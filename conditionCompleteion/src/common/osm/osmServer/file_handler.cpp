#include <list>
#include <string>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/filesystem/operations.hpp>

#include "osm/osmServer/utils.h"
#include "osm/osmServer/file_handler.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/object_storage_exception.h"
#include "libraryUtils/config_file_parser.h"

using namespace std;

LockFile::LockFile(std::string path, boost::function<int(void)> check_hfs_stat):gfs_path(path), check_hfs_stat(check_hfs_stat)
{
	this->lock_file_base = this->gfs_path + "/LOCK";
}

LockFile::~LockFile()
{

}

int LockFile::get_lock(std::string & election_version)
{
	std::string lock_file = this->get_lock_file();
	
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
        GLUtils gl_utils_obj;
        config_parser = gl_utils_obj.get_config();
	uint16_t t_notify_plus_delta = config_parser->t_el_timeout_deltaValue();

	if(strlen(lock_file.c_str()) != 0)
	{
		/*If election is already running,
		update own version and return false.*/
		OSDLOG(DEBUG, "Lock file already exist");
		OsmUtils osm_utils;
		string existing_version = osm_utils.split(lock_file, "_");
		OSDLOG(INFO, "Election Version : " << election_version << ", Existing Version : " << existing_version);
		float existing_v = ::atof(existing_version.c_str());
		float election_v = ::atof(election_version.c_str());
		//if(strcmp(existing_version.c_str(), election_version.c_str()) >= 0)
		if(existing_v > election_v)
		{
			/*
			if(election_temp_file is older than timeout_value)
			{
				remove the election_temp_file;
			}*/
			OSDLOG(INFO, "Newer version of election already in progress");
			election_version = existing_version;
			return LOCK_REJECTED;
		}
		else if(existing_v == election_v)
		{
			std::string existing_sub_ver = existing_version.substr(existing_version.find_last_of("-")+1, existing_version.size());
			std::string election_sub_ver = election_version.substr(election_version.find_last_of("-")+1, election_version.size());
			if(existing_sub_ver >= election_sub_ver)
			{
				OSDLOG(INFO, "Election with newer sub-version is already in progress");
				election_version = existing_version;
				return LOCK_REJECTED;
			}
			else
			{
				boost::this_thread::sleep(boost::posix_time::seconds(t_notify_plus_delta));
				if(this->check_hfs_stat() != OSDException::OSDHfsStatus::NOT_WRITABILITY)
				{
					return LOCK_NOT_NEEDED;
				}
				std::string complete_lock_file_path = this->gfs_path + "/" + lock_file;
				::remove(complete_lock_file_path.c_str());
				OSDLOG(INFO, "Removed obsolete lock file " << complete_lock_file_path << " with errno " << errno);

			}
		}
		else
		{
			boost::this_thread::sleep(boost::posix_time::seconds(t_notify_plus_delta)); //t-notify timeout plus delta
			if(this->check_hfs_stat() != OSDException::OSDHfsStatus::NOT_WRITABILITY)
			{
				return LOCK_NOT_NEEDED;
			}
			std::string complete_lock_file_path = this->gfs_path + "/" + lock_file;
			::remove(complete_lock_file_path.c_str());
			OSDLOG(INFO, "Removed obsolete lock file " << complete_lock_file_path << " with errno " << errno);
		}
	}
	try
	{
		std::string lock_file_name = this->lock_file_base + "_" + election_version;
		OSDLOG(INFO, "Creating election lock: " << lock_file_name);
		int ret = ::open(lock_file_name.c_str(), O_CREAT | O_EXCL, S_IRUSR|S_IWUSR);
		if(ret >= 0)
		{
			OSDLOG(DEBUG, "Election lock file created");
			::close(ret);
			return LOCK_ACQUIRED;
		}
		else if((ret < 0) & (errno == EEXIST))
		{
			OSDLOG(INFO, "Election lock file already created");
			return LOCK_REJECTED;
		}
		else
		{
			OSDLOG(ERROR, "While creating lock file, failed with error no: " << errno);
			throw OSDException::CreateFileException();
		}
		
	}
	catch(std::string msg)
	{
		OSDLOG(ERROR, "Error while getting lock. Errno: " << errno);
		throw OSDException::CreateFileException();
	}
	return LOCK_ACQUIRED;
}

int LockFile::release_lock(std::string & election_version)
{
	std::string lock_file_name = this->lock_file_base + "_" + election_version;
	OSDLOG(DEBUG, "Removing lock file : " << lock_file_name);
	return ::remove(lock_file_name.c_str());
}

std::string LockFile::get_lock_file()
{
	OsmUtils osm_utils;
	std::string latest_lock = "";
//	std::list<std::string> fileList = osm_utils.list_files(this->gfs_path.c_str(), "LOCK");
	std::vector<std::string> fileList = osm_utils.list_files_in_creation_order(this->gfs_path, "LOCK");
	int fileCount = fileList.size();
	OSDLOG(DEBUG, "Existing Election lock file count: " << fileCount);
//	fileList.sort();
	if(!fileList.empty())
	{
		latest_lock = fileList.back();
		std::vector<std::string>::iterator it;
		it = fileList.begin();
		for(int i=0;i<fileCount-1;i++)
		{
			OSDLOG(INFO, "Multiple lock files exist, removing: " << (*it).c_str());
			int ret = ::remove((this->gfs_path + "/" + *it).c_str());
			OSDLOG(DEBUG, "Remove Status: " << errno)
			++it;
		}
	}
	return latest_lock;
}


ElectionFile::ElectionFile(std::string path)
{
	gfs_path = path;
	elect_file_name = gfs_path + "/" + "ELECT_FILE";

}

int ElectionFile::create_file(std::string path_name)
{
	cool::unused(path_name);
	return 1;
}

int ElectionFile::read(int const fd, char * buffer, uint64_t const bufferLength)
{
	OSDLOG(DEBUG, "Going to read " << bufferLength << " bytes.");
	return ::read(fd, buffer, bufferLength);
}

int ElectionFile::write(int const fd, char const * buffer, uint64_t const bufferLength)
{
	uint64_t const writeBytes = ::write(fd, buffer, bufferLength);
	if (writeBytes  < 0)
	{return -1;}
	else if(writeBytes == bufferLength)
	{return 0;}
	else
	{return writeBytes;}
}

int ElectionFile::remove_file(std::string path_name)
{
	return ::remove(path_name.c_str());
}

int ElectionFile::open(std::string path_name, int const mode)
{
	return ::open(path_name.c_str(), mode, S_IWUSR|S_IRUSR|S_IRGRP);
}

int ElectionFile::close(int const fd, bool const sync)
{
	if (sync)
	{
		int ret = ::fsync(fd);
		if (ret < 0)
		{
			OSDLOG(ERROR, "Failed to sync: error code: " << errno);
			return ret;
		}
	}
	return ::close(fd);
}
int ElectionFile::rename(std::string oldpath, std::string newpath)
{
	OSDLOG(DEBUG, "Rename file " << oldpath << " to " << newpath);
	int const ret = ::rename(oldpath.c_str(), newpath.c_str());
	if (ret != 0)
	{
		return errno;
	}
	else
	{
		return 0;
	}
}

ElectionFileHandler::ElectionFileHandler(std::string path)
{
	this->gfs_path = path;
	this->elect_file_name = this->gfs_path + "/" + "ELECT_FILE";
	this->temp_path = this->elect_file_name + "_TEMP";
	this->elect_file.reset(new ElectionFile(this->gfs_path));
	this->buffer_size = 8192;
}

bool ElectionFileHandler::election_file_exist()
{
	return boost::filesystem::exists(this->elect_file_name);
}
/* Temp path clean up code starts */
int ElectionFileHandler::temp_file_cleaner(int temp_cleanup_timeout)
{
	if(boost::filesystem::exists(this->temp_path))
	{
		struct stat temp_path_attr;
		stat(this->temp_path.c_str(), &temp_path_attr);
		time_t timer;
		time(&timer);
		double seconds = difftime(timer,temp_path_attr.st_ctime);
		if(seconds > temp_cleanup_timeout)
		{
			/*remove temp file */
			OSDLOG(DEBUG, "Removing temp file");
			return this->elect_file->remove_file(this->temp_path);
		}
	}
	return 0;
}

bool ElectionFileHandler::post_election_file_exist()
{
	OsmUtils osm_utils;
	std::list<std::string> file_list = osm_utils.list_files(this->gfs_path.c_str(), "LOCK");
	std::string lock_file = "";
	if(!file_list.empty())
	{
		file_list.sort();
		lock_file = file_list.back();
	}
	struct stat temp_path_attr, lock_file_attr;
	if(boost::filesystem::exists(this->temp_path))
	{
		stat(lock_file.c_str(), &lock_file_attr);
		stat(this->temp_path.c_str(), &temp_path_attr);
		if(temp_path_attr.st_ctime > lock_file_attr.st_ctime)
		{return true;}
		return false;
		
	}
	return false;
}

int ElectionFileHandler::create_election_file(int fileSize)
{
	std::string tmp_elect_file = this->elect_file_name + "_tmp";
	OSDLOG(DEBUG, "Creating election file: " << tmp_elect_file);
	int const fd = this->elect_file->open(tmp_elect_file, O_CREAT|O_WRONLY);
	if (fd < 0)
	{
		OSDLOG(ERROR, "Failed to open election file, Errorno: " << errno);
		return fd;
	}
	char buffer[this->buffer_size];
	memset(buffer, '\0', fileSize);
	OSDLOG(DEBUG, "Fill election file with null bytes of size: " << fileSize);
	if(this->elect_file->write(fd, buffer, fileSize) == 0)
	{
		this->elect_file->close(fd, 1);
	}
	return this->elect_file->rename(tmp_elect_file, this->elect_file_name);
}

int ElectionFileHandler::write_on_offset(const char * nodeInfo, int node_info_size, int const offset )
{
	OSDLOG(DEBUG, "Open election file: " << this->elect_file_name << " to write node info on offset:" << offset);
	int const fd = this->elect_file->open(this->elect_file_name, O_RDWR);
	if (fd < 0)
	{
		OSDLOG(ERROR, "Failed to open election file, Errorno: " << errno);
		throw OSDException::OpenFileException();
	}
	if(::lseek(fd, offset, SEEK_SET) < 0)
	{
		OSDLOG(ERROR, "Failed to jump on position in election file, Errorno: " << errno);
		return -1;
	}
	if(this->elect_file->write(fd, nodeInfo, node_info_size) == 0)
	{
		OSDLOG(DEBUG, "election file write successful");
		return this->elect_file->close(fd, 1);
	}
	return -1;
}

std::string ElectionFileHandler::getGL(int record_size, int file_size)
{
	OSDLOG(DEBUG, "Block participant, by moving election file: " << this->elect_file_name << " to " << this->temp_path);
	this->elect_file->rename(this->elect_file_name, this->temp_path);//Rename elect file, to disable write.
	int const fd = this->elect_file->open(this->temp_path, O_RDONLY);
	if (fd < 0)
	{
		OSDLOG(ERROR, "Failed to open election file, Errorno: " << errno);
		throw OSDException::OpenFileException();
	}
	char *buffer = new char[this->buffer_size];
	memset(buffer, '\0', this->buffer_size);
	if(this->elect_file->read(fd, buffer, buffer_size) != file_size)
	{
		OSDLOG(ERROR, "Failed to read election file, Errorno: " << errno);
		throw OSDException::ReadFileException();
	}
	OsmUtils osm_utils;
	//std::list<recordStructure::ElectionRecord *> record_list;
	std::list<std::string> record_list;
	for(int i=0; i < file_size; i += record_size)
	{
		recordStructure::ElectionRecord *record;
		record = dynamic_cast<recordStructure::ElectionRecord *>(recordStructure::deserialize_record(buffer, record_size));
		if(record == NULL)
		{
			OSDLOG(DEBUG, "Record is null");
		}
		else
		{
			OSDLOG(DEBUG, "Node name is :" << record->get_node_id());
			//record_list.push_back(record);
			record_list.push_back(record->get_node_id());
		}
		buffer += record_size;
	}
	record_list.sort();
	OSDLOG(DEBUG, "No of eligible candidates: " << record_list.size());
	//OSDLOG(DEBUG, "Elected node Id: " << record_list.back()->get_node_id());
	OSDLOG(INFO, "Elected node Id: " << record_list.back());
	//return record_list.back()->get_node_id();
	return record_list.back();
}


int ElectionFileHandler::sweep_election_file()
{
	OSDLOG(DEBUG, "Remove temp election file: " << this->temp_path);
	return this->elect_file->remove_file(this->temp_path);
}
