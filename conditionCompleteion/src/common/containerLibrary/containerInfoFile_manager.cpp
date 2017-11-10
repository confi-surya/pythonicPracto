#include <poll.h>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/function.hpp>
#include "libraryUtils/object_storage_log_setup.h"
#include "containerLibrary/containerInfoFile_manager.h"
#include "libraryUtils/object_storage_exception.h"
#include <iostream>
#include <errno.h>
namespace containerInfoFile
{

ContainerInfoFileManager::ContainerInfoFileManager(uint8_t node_id):node_id(node_id)
{
	OSDLOG(DEBUG, "ContainerInfoFileManager::ContainerInfoFileManager called");
	this->container_infoFile_builder.reset(new ContainerInfoFileBuilder());
	this->lock_manager.reset(new lockManager::LockManager(1024));	//TODO: discussion required to change this value from 1024 to 10
									// currently 1024 lock is set to avoid threads getting blocked.
}

void ContainerInfoFileManager::get_container_stat(
	std::string const & path,
	recordStructure::ContainerStat* containter_stat,
	boost::function<recordStructure::ContainerStat(void)> const & get_stat
)
{
	OSDLOG(DEBUG, "ContainerInfoFileManager::get_container_stat called for " << path);
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::ReadLock lock(mutex);
	boost::shared_ptr<ContainerInfoFile> container_info_file = 
			this->container_infoFile_builder->create_container_for_reference(this->get_file_name(path), mutex);
//	container_info_file->get_container_stat(containter_stat);
	container_info_file->get_container_stat(path, containter_stat, get_stat);
	return;
}

void ContainerInfoFileManager::get_container_stat_after_compaction(
	std::string const & path,
	recordStructure::ContainerStat* container_stat,
	RecordList * record_list,
	boost::function<void(void)> const & release_flush_records,
	boost::function<bool(void)> mem_delete
)
{
	OSDLOG(DEBUG, "ContainerInfoFileManager::get_container_stat_after_compaction called for " << path);
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	// file should be opened in r/w mode
	boost::shared_ptr<ContainerInfoFile> container_info_file = this->container_infoFile_builder->create_container_for_modify(this->get_file_name(path), mutex);
	container_info_file->get_container_stat_after_compaction(path, container_stat, record_list, release_flush_records,this->node_id);
	return;
}

void ContainerInfoFileManager::set_container_stat(
			std::string const & path, 
			recordStructure::ContainerStat  container_stat)
{
	OSDLOG(DEBUG, "ContainerInfoFileManager::set_container_stat called for " << path);
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	boost::shared_ptr<ContainerInfoFile> container_info_file = 
				this->container_infoFile_builder->create_container_for_modify(this->get_file_name(path), mutex);
	container_info_file->set_container_stat(container_stat);

}


Status ContainerInfoFileManager::flush_container(
        std::string path,
        RecordList * record_list,
        bool unknown_status,
        boost::function<void(void)> const & release_flush_records,
        boost::function<bool(void)> mem_delete
)
{
        boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
        lockManager::WriteLock lock(mutex);
        OSDLOG(DEBUG, "Record list size: "<< record_list->size());

        char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
        std::string copy_command, rename_command, tmp_file;
        std::string file_name = this->get_file_name(path);
        tmp_file = file_name + "_tmp";

	if(!(boost::filesystem::exists(file_name)))
        {
                OSDLOG(INFO, "Info File Not Found " << file_name);
		release_flush_records();
                return Status(OSDException::OSDErrorCode::INFO_FILE_NOT_FOUND);
        }

        if (!osd_config)
        {
                //Create Copy Command
                copy_command = "sudo /opt/HYDRAstor/hfs/bin/release_prod_sym/filecopy.hydrafs " +
                               file_name + " " + tmp_file;
        }
        else
        {
                copy_command = "cp " + file_name + " " + tmp_file;
        }
        rename_command = "mv " + tmp_file + " " + file_name;
        OSDLOG(DEBUG, "tmp filename: "<< tmp_file << " filename: "<< file_name);

	try
        {
                int ret = -1;
                OSDLOG(DEBUG, "Copy Command : " << copy_command );
                cool::ShellCommand command(copy_command.c_str());
                cool::Option<cool::CommandResult> result  = command.run();
                ret = result.getRefValue().getExitCode();
                OSDLOG(DEBUG, "Return value of copy command execution : " << ret);
                OSDLOG(DEBUG, "Errno : " << strerror(errno) );
                if ( ret == 0)
                {
                        OSDLOG(DEBUG, "Info File copied to : "<< path);
                        boost::shared_ptr<ContainerInfoFile> container_info_file =
                                this->container_infoFile_builder->create_container_for_modify(tmp_file, mutex);

                        container_info_file->flush_container(record_list, unknown_status, release_flush_records,this->node_id);
                }
                else
                {
			//TODO following passert should be removed
	                PASSERT(false, "Could not copy InfoFile to Temp file during flushing. Aborting.");
        	        //return Status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
                }
        }
        catch(...)
        {

                //TODO following passert should be removed
                PASSERT(false, "Error in Info File Operation during flushing. Aborting.");
                //return Status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
        }
	OSDLOG(DEBUG, "Renaming Temp Info file to Actual Info File");
        cool::ShellCommand command(rename_command.c_str());
        cool::Option<cool::CommandResult> result  = command.run();
        int ret = result.getRefValue().getExitCode();
        if ( ret == 0 )
        {
                OSDLOG(DEBUG, "Rename command passed");
                return Status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
        }
        else
        {
                //TODO following passert should be removed
                PASSERT(false, "Error in Renaming Temp file to original info file during flushing. Aborting.");
                //OSDLOG(DEBUG, "Rename command failed. Error in Info file operation");
                //OSDLOG(DEBUG, "Error in Info File Operation");
                //return Status(OSDException::OSDErrorCode::INFO_FILE_OPERATION_ERROR);
        }
}


void ContainerInfoFileManager::get_object_list(
		std::string const & path,
		std::list<recordStructure::ObjectRecord> * list,
		boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records,
		uint64_t const count,
		std::string const & marker, std::string const & end_marker,
		std::string const & prefix, std::string const & delimiter)
{
	OSDLOG(DEBUG, "ContainerInfoFileManager::get_object_list called for " << path <<" with count, marker and end marker "<<
		count << "," << marker << "," << end_marker);
	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::ReadLock lock(mutex);
	boost::shared_ptr<ContainerInfoFile> container_info_file =
				this->container_infoFile_builder->create_container_for_reference(this->get_file_name(path), mutex);
	container_info_file->get_object_list(list, records, count, marker, end_marker, prefix, delimiter);
}


void ContainerInfoFileManager::create_dir_if_not_exist(std::string path)
{
	boost::filesystem::path dir_path(path);
	if(!(boost::filesystem::exists(dir_path)))
	{
		boost::filesystem::create_directories(dir_path);
	}
}


void ContainerInfoFileManager::move_container_info_file(std::string source_file, std::string destination_file)
{
	//move file TODO:boost exception handling
	boost::filesystem::rename(source_file, destination_file);
	OSDLOG(DEBUG, "Temporary file renamed");
}	

std::string ContainerInfoFileManager::get_temp_file_path(std::string temp_path, std::string path)
{
	std::vector<std::string> path_tokens;
	boost::split(path_tokens, path, boost::is_any_of("/"));
	if(path_tokens.size() > 0)
	{
 		std::string temp_file_parent_dir;
	 	std::string temp_file_name;
		temp_file_parent_dir = temp_path + "/" + "temp" + "/";
 		this->create_dir_if_not_exist(temp_file_parent_dir);
		temp_file_name = temp_file_parent_dir + path_tokens[ path_tokens.size()- 1 ] + ".infoFile";//create tempfile
		if(boost::filesystem::exists(boost::filesystem::path(temp_file_name)))
		{
			OSDLOG(DEBUG, "Removing info file, info-file path "<<temp_file_name);
			boost::filesystem::remove(boost::filesystem::path(temp_file_name));
		}
		OSDLOG(DEBUG, "Temporary container info-file path "<<temp_file_name);
		return temp_file_name;
	}
	else
	{
		OSDLOG(ERROR, "ContainerInfoFileManager::get_temp_file_path Invalid File Path "<<path);
		return "";
	}
}

Status ContainerInfoFileManager::add_container(
	std::string const & temp_path,
	std::string const & path,
	recordStructure::ContainerStat const container_stat,
	JournalOffset const offset
)
{
	OSDLOG(DEBUG, "Executing add container");

	boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
	lockManager::WriteLock lock(mutex);
	std::string file_path = this->get_file_name(path);
	std::string temp_file_path = this->get_temp_file_path(temp_path, path);
	if(!boost::filesystem::exists(boost::filesystem::path(file_path)))
	{
		OSDLOG(INFO, "Container " << path << " does not exist, adding new container");
		boost::filesystem::create_directories(boost::filesystem::path(path));
		boost::shared_ptr<ContainerInfoFile> container_info_file =
				this->container_infoFile_builder->create_container_for_create(temp_file_path, mutex);
		container_info_file->add_container(container_stat, offset);
		this->move_container_info_file(temp_file_path, file_path);
		return Status(OSDException::OSDSuccessCode::INFO_FILE_OPERATION_SUCCESS);
	}
	else
	{
		OSDLOG(DEBUG, "Container " << path << " exists, updating stat of container");
		boost::shared_ptr<ContainerInfoFile> container_info_file =
				this->container_infoFile_builder->create_container_for_modify(this->get_file_name(path), mutex);
		container_info_file->set_container_stat(container_stat, true);
		return Status(OSDException::OSDSuccessCode::INFO_FILE_ACCEPTED);
	}
}

bool ContainerInfoFileManager::delete_container(
	std::string const & path,
	boost::function<bool(void)> mem_delete
)
{
	OSDLOG(INFO, "ContainerInfoFileManager::delete_container called for " << path);
	
	while (true)
	{
		{
			// Remove from memory. If flush or compaction is in progress, wait for it.
			boost::shared_ptr<lockManager::Mutex> mutex = this->lock_manager->get_lock(path);
			lockManager::WriteLock lock(mutex);
			
			if(mem_delete())
			{
			       OSDLOG(INFO, "Memory deleted for container " << path << " while delete_container");
			       break;
			}
		}
		poll(NULL, 0, 1);
	}

	if(boost::filesystem::exists(boost::filesystem::path(this->get_file_name(path))))
	{
		boost::filesystem::remove(boost::filesystem::path(this->get_file_name(path)));	
		return true;
	}
	OSDLOG(INFO, "While deleting container path doesn't exist" << path);
	return false;
}

std::string ContainerInfoFileManager::get_file_name(std::string path)
{
	std::vector<std::string> path_tokens;
	boost::split(path_tokens, path, boost::is_any_of("/"));

	std::string file_name;
	if(path_tokens.size() > 0){
		file_name = path + "/" + path_tokens[ path_tokens.size()- 1 ] + ".infoFile"; // create file name
		return file_name;
	}
	else{
		OSDLOG(ERROR, "ContainerInfoFileManager::get_file_name Path is not valid " << path);
		return "";
	}
}

ContainerInfoFileManager::~ContainerInfoFileManager()
{
}

}
