#include <list>
#include <vector>
#include <string>
#include <stdio.h>
#include <sstream>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <signal.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "cool/shellCommandsExecution.h"
#include "osm/osmServer/osm_info_file.h"
#include "osm/osmServer/utils.h"
#include "osm/globalLeader/journal_writer.h"
#include <boost/algorithm/string.hpp>

using namespace std;

/*const static std::string node_name_delimiter = "_";
//const static std::string GLOBAL_LEADER_INFORMATION_FILE = "/export/osd_meta_config/osd_global_leader_information";
const static std::string GLOBAL_LEADER_INFORMATION_FILE = "/remote/hydra042/gmishra/gl_config/osd_global_leader_information";
*/
const static int MAX_RETRY_COUNT_FS_CREATE = 3;

GLUtils::GLUtils()
{
	
}

GLUtils::~GLUtils()
{
	
}

std::string GLUtils::get_node_id(std::string node_name, int32_t port)
{
	return (node_name + node_name_delimiter + boost::lexical_cast<std::string>(port));
}

uint64_t GLUtils::get_osm_port_from_id(std::string node_id)     //HN0101_64014
{
	return boost::lexical_cast<uint64_t>(node_id.substr(node_id.find(node_name_delimiter)+1, node_id.size()));
}

std::string GLUtils::get_node_name(std::string service_id)      //Will return HN0101
{
	return service_id.substr(0, service_id.find_last_of(node_name_delimiter));
}

std::string GLUtils::get_service_id(std::string node_id, std::string service_name)
{
	return (node_id + "_" + service_name);
}

std::string GLUtils::get_node_id(std::string service_id)
{
	return service_id.substr(0, service_id.find_last_of(node_name_delimiter));
}

std::string GLUtils::get_service_name(std::string service_id)
{
	return service_id.substr(service_id.find_last_of(node_name_delimiter)+1, service_id.size());
}

std::set<int64_t> GLUtils::selectOnSockets(std::set<int64_t> fd_list, int & error, int timeout)
{
	fd_set sockets;
	FD_ZERO(&sockets);
	int64_t maxfd = 0;
	struct timeval t;
	if(timeout > 0)
	{
		t.tv_sec = timeout;
	}

	for (std::set<int64_t>::iterator it = fd_list.begin(); it != fd_list.end(); ++it)
	{
		FD_SET(*it, &sockets);
		maxfd = std::max(maxfd, *it);
	}
	std::set<int64_t> return_set;
	int active_fd_count = ::select(maxfd + 1, &sockets, NULL, NULL, &t);
	int32_t const errnoTmp = errno;
	if ( active_fd_count < 0 )
	{
		error = errnoTmp;
		return return_set;
	}else if(active_fd_count == 0)
	{
		OSDLOG(DEBUG,"Select timeout");
	}
	BOOST_FOREACH(int64_t const fd, fd_list)
	{
		if (FD_ISSET(fd, &sockets))
		{
			return_set.insert(fd);
		}
	}
	return return_set;
}

std::string GLUtils::get_node_ip(std::string node_id)
{
	std::string command = "grep -w " + node_id + " /etc/hosts";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) )
	{
		//std::cout << buf << std::endl;
	}
	::pclose(proc);
	std::string ip(buf);
	return ip.substr(0, ip.find("\t"));	//TODO: Verify if space or tab
}

bool GLUtils::verify_if_single_hydra()
{
	std::string command = "sudo equipment get sw -c HYDRA_IS_SINGLE";
	FILE *proc = popen(command.c_str(),"r");
	char buf[10];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) );
	::pclose(proc);
	if(strncmp(buf, "NO", 2) == 0)
		return false;
	else
		return true;
}

bool GLUtils::verify_if_alternate()
{
	std::string my_node_id = this->get_my_node_id();
	std::string command = "sudo /opt/HYDRAstor/cluster_management/Migrate/GetClusterNode_Current.sh | grep -w Alternative | cut -d'=' -f2 $1";
	FILE *proc = popen(command.c_str(),"r");
	char buf[10];
	memset(buf, '\0', 10);
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) );
	::pclose(proc);
	std::string str(buf);
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
//	return strncmp(buf, my_node_id.c_str(), my_node_id.size());
	if(str == my_node_id)
		return true;
	else
		return false;
}

std::string GLUtils::read_GL_file()	//GL file will contain ID IP PORT tab separated in a single line
{
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	std::string cmd;
	if (osd_config)
		cmd = "find " + std::string(osd_config) + " -name " + GLOBAL_LEADER_INFORMATION_FILE + "* | xargs cat $1";
	else
		cmd = "find " + CONFIG_FILESYSTEM + " -name " + GLOBAL_LEADER_INFORMATION_FILE + "* | xargs cat $1";
/*	char buf[50];
	memset(buf, '\0', 50);
	FILE *proc = popen(cmd.c_str(),"r");
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) );
	std::string str(buf);
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
	::pclose(proc); */
	int32 ret = -1;
	cool::ShellCommand command(cmd.c_str());
        cool::Option<cool::CommandResult> result  = command.run();
	ret = result.getRefValue().getExitCode();
	
	std::string str;
	if(ret == 0) {	
	      str = result.getRefValue().getOutputString();
	}

	return str;
}

std::string GLUtils::get_my_node_id()
{
	FILE *proc = popen("hostname","r");;
	char buf[10];
	memset(buf, '\0', 10);
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) );
	::pclose(proc);
	std::string node(buf);
	node.erase(std::remove(node.begin(), node.end(), '\n'), node.end());
	return node;
}

std::string GLUtils::get_my_service_id(int32_t port)
{
	//std::cout << "My service id: " << (this->get_my_node_id() + "_" + boost::lexical_cast<std::string>(port)) << std::endl;
	return (this->get_my_node_id() + "_" + boost::lexical_cast<std::string>(port));
}

std::string GLUtils::get_osd_service_id(int port, std::string service_name)
{
	return ( this->get_my_service_id(port) + "_" + service_name + "-" + "server");
}

cool::SharedPtr<config_file::ConfigFileParser> GLUtils::get_journal_config()
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config_dir)
		//config_file_builder.readFromFile(std::string(osd_config_dir).append("/osm.conf"));
		config_file_builder.readFromFile(std::string(osd_config_dir).append("/gl_journal.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/gl_journal.conf");
	return config_file_builder.buildConfiguration();

}

cool::SharedPtr<config_file::OsdServiceConfigFileParser> GLUtils::get_osm_timeout_config()
{
	cool::config::ConfigurationBuilder<config_file::OsdServiceConfigFileParser> config_file_builder;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config_dir)
		config_file_builder.readFromFile(std::string(osd_config_dir).append("/OsdServiceConfigFile.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/OsdServiceConfigFile.conf");
	return config_file_builder.buildConfiguration();
}

cool::SharedPtr<config_file::OSMConfigFileParser> GLUtils::get_config()
{
	cool::config::ConfigurationBuilder<config_file::OSMConfigFileParser> config_file_builder;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config_dir)
		config_file_builder.readFromFile(std::string(osd_config_dir).append("/osm.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/osm.conf");
	return config_file_builder.buildConfiguration();			
}

bool GLUtils::verify_if_gl(bool if_cluster, int32_t port)
{
	if (if_cluster)
	{
		std::string str = read_GL_file();
		if (str.substr(0, str.find("\t")) == this->get_my_service_id(port))
		{
			OSDLOG(DEBUG, "I am GL according to GL Info");
			return true;
		}
	}
	else
	{
		if(this->verify_if_single_hydra())
		{
			OSDLOG(DEBUG, "I am GL since this is mini hydra");
			this->remove_GL_specific_file(GLOBAL_LEADER_INFORMATION_FILE);
			return true;
		}
		else if (this->verify_if_alternate())
		{
			OSDLOG(DEBUG, "I am GL since this is alternate node");
			this->remove_GL_specific_file(GLOBAL_LEADER_INFORMATION_FILE);
			return true;
		}
	}
	return false;
}

enum network_messages::NodeStatusEnum GLUtils::verify_if_part_of_cluster(std::string node_id)
{
	enum network_messages::NodeStatusEnum node_stat = network_messages::NEW;
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = this->get_config();
	std::string node_info_file_path = parser_ptr->node_info_file_pathValue();
	
	osm_info_file_manager::NodeInfoFileReadHandler nodeInfoFileReadObj(node_info_file_path);
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> recordPtr = nodeInfoFileReadObj.get_record_from_id(node_id);
        if(recordPtr)   
		node_stat = static_cast<network_messages::NodeStatusEnum>(recordPtr->get_node_status());
	return node_stat;
}

/*
void register_gl_msg_handler()
{
	std::cout << "Registering GL Msg Handler\n";
}

void register_ll_full_functionality()
{
	std::cout << "Registering LL full functionality\n";
}
*/
bool GLUtils::verify_cluster_existence(std::string node_info_file_path, std::string config_fs_name)
{
	std::string cmd;
	if(node_info_file_path.substr(node_info_file_path.size()-1, node_info_file_path.size()) == "/")
		cmd = "sudo cat /proc/mounts | grep -w " + node_info_file_path.substr(0, node_info_file_path.size()-1) + " | cut -d' ' -f2 $1 | cut -d'/' -f3 $1";
	
	char buf[50];
	memset(buf, '\0', 50);
	FILE *proc = popen(cmd.c_str(),"r");
	fgets(buf,sizeof(buf),proc);
	::pclose(proc);
	std::string str(buf);
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
	if ( str == config_fs_name )
	{
		if ( boost::filesystem::exists( node_info_file_path + osm_info_file_manager::NODE_INFO_FILE_NAME) )
			return true;
	}
	
	return false;
}

bool GLUtils::license_check()
{
	return true;
}

std::string GLUtils::get_mount_path(std::string path, std::string file_system, bool create)
{
	std::string mount_path = path + "/" + file_system;
	if( create )
	{
		if( !boost::filesystem::exists(mount_path))
		{
			if (::mkdir(mount_path.c_str(),S_IRWXU | S_IRWXG) == -1) 
			{
				 OSDLOG(DEBUG, "Could not create Log Dir: " << mount_path.c_str() << " system error #" << errno);
			}
		}
	}
	return mount_path;		
}

std::string GLUtils::get_mount_command(std::string cmd, std::string file_system, std::string options, std::string mount_path)
{
	std::string mount_command = cmd;
	mount_command = mount_command + " " + options + " " + file_system + " " + mount_path; 
	return mount_command;
}

std::string GLUtils::get_unmount_command(std::string cmd, std::string file_system, std::string options)
{
	std::string umount_command = cmd;
	umount_command = umount_command + " -f " + options + file_system;
	return umount_command;
}
/*start*/
bool GLUtils::create_and_mount_file_system(std::string path, std::string cmd, std::string mkfs_command, 
		std:: string fsexist_command, uint32_t timeout)
{
	bool flag = true;
	std::string filesystem_name = ".osd_meta_config";
	OSDLOG(INFO,"Mount the file system osd_meta_config");  
	std::string mount_path = this->get_mount_path(path, filesystem_name);
	if( !this->is_mount(mount_path))
	{
		std::string mount_command = this->get_mount_command(cmd, filesystem_name, "-d", mount_path);
		OSDLOG(INFO, "Mount command = "<<mount_command);
		int status = 20;
		FILE *fp;

		/*special handling for meta_config_filesystem*/
		if( !boost::filesystem::exists(mount_path))
		{
			if (::mkdir(mount_path.c_str(),S_IRWXU | S_IRWXG) == -1) 
			{
				if(errno != EEXIST)
				{
					OSDLOG(DEBUG, "Could not create osd_meta_config Dir: " << mount_path.c_str() << " system error #" << errno);
					return false;
				}
			}
		}
		std::string cmd = fsexist_command + " .osd_meta_config";
		CommandExecuter cmd_executor;
		OSDLOG(DEBUG, "file system check command : " << cmd);
		cmd_executor.exec_command(cmd.c_str(), &fp, &status);
		status = status>>8;
		OSDLOG(DEBUG, "Status of command : " << status);
		uint32_t timeout_left = timeout;
		if(0 != status)
		{
			int retry_count = 0;
			//while(timeout_left > 0)
			while(1)
			{
				CommandExecuter cmd_exec;
				string my_node_id = this->get_my_node_id();
				string command = "chmod -R 777 /export/.osd_meta_config;";
				//command = command + "clido fs create name=osd_meta_config  node=" + my_node_id + ";";
				command = command + mkfs_command + " -r copy .osd_meta_config";
				cmd_exec.exec_command(command.c_str(), &fp, &status);
				OSDLOG(DEBUG, "file system create command : " << command);
				status = status>>8;
				/*Error check for already created filesystem TBD*/
				if(status != 0 && status != 3) //0 for success and 3 for fs already created
				{
				//	if (status == HFS_NOT_READY)
					if (status == HFS_NOT_READY or status == HFS_BUSY or status == HFS_SERVER_BUSY)
					{
						OSDLOG(INFO, "Filesystem " << filesystem_name << " status is " << status);
						//timeout_left = timeout_left - SLEEP_TIME;
						boost::this_thread::sleep(boost::posix_time::seconds(SLEEP_TIME));
						flag = false;
					}
					else
					{
						OSDLOG(INFO, "osd_meta_config filesystem couldn't be created due to err code: " << status);
						return false;
					}

				}
				else
				{
					flag = true;
					break;
				}
			}
		}
		if(!flag)
		{
			OSDLOG(ERROR, "File system osd_meta_config could not be created");
			return  flag;
		}
		/*Special handling for meta_config_filesystem ends*/

		status =20;
		//while( timeout > 0)
		while(1)
		{
			CommandExecuter cmd_exec;
			cmd_exec.exec_command(mount_command.c_str(), &fp, &status);
			status = status>>8;
			OSDLOG(INFO, "Status of mount command "<< mount_command<<" is "<<status);
			//if( status != HFS_BUSY )
			if( status == 11 or status == 0)	//11 is for already mounted and 0 for success
			{
				flag = true;
				break;
			}
			else
			{
				if (status == HFS_BUSY or status == HFS_NOT_READY or status == HFS_SERVER_BUSY)
				{
					OSDLOG(INFO, "Filesystem " << filesystem_name << " status is " << status);
					//timeout = timeout - SLEEP_TIME;
					boost::this_thread::sleep(boost::posix_time::seconds(SLEEP_TIME));
					flag = false;
				}
				else
				{
					return false;
				}
			}
		}
	}
	OSDLOG(INFO, "mount_file_system return status " << flag);
	return  flag;
}
/*end*/
bool GLUtils::mount_file_system(std::string path, std::string cmd, uint32_t timeout)
{
	std::vector<std::string>list = this->get_global_file_system_list();
	std::vector<std::string>::iterator it;
	bool flag = true;
	for( it = list.begin(); it != list.end(); it++)
	{
		flag = true;
		std::string mount_path = this->get_mount_path(path, *it, true);
		if( !this->is_mount(mount_path))
		{
			std::string mount_command = this->get_mount_command(cmd, *it, "-d", mount_path);
			OSDLOG(INFO, "Mount command = "<<mount_command);
			int status = 20;
			FILE *fp;
			//while( timeout > 0)
			while(1)
			{
				CommandExecuter cmd_exec;
				cmd_exec.exec_command(mount_command.c_str(), &fp, &status);
				status = status>>8;
				OSDLOG(INFO, "Status of mount command "<< mount_command<<" is "<<status);
				//if( status != HFS_BUSY )
				if( status == 11 or status == 0)	//11 is for already mounted and 0 for success
				{
					flag = true;
					break;
				}
				else
				{
					//if (status == HFS_BUSY)
					if (status == HFS_BUSY or status == HFS_NOT_READY or status == HFS_SERVER_BUSY)
					{
						OSDLOG(INFO, "Filesystem "<<*it<<" status is "<< status);
						//timeout = timeout - SLEEP_TIME;
						//boost::this_thread::sleep(boost::posix_time::seconds(timeout));
						boost::this_thread::sleep(boost::posix_time::seconds(SLEEP_TIME));
						flag = false;
					}
					else
					{
						//flag = false;
						//break;
						return false;
					}
				}
			}
		}
		if(flag == false)
		{
			return flag;
		}
	}
	OSDLOG(INFO, "mount_file_system return status "<<flag);
	return  flag;
}

std::string GLUtils::get_version_from_file_name(std::string file_name)
{
	std::string str2 = file_name.substr(file_name.find_last_of(node_name_delimiter) + 1, file_name.size());
	std::string str3 = str2.substr(0, str2.find_first_of(lock_file_delimeter));
	return str3;
}

bool GLUtils::umount_file_system(std::string path, std::string cmd, uint32_t timeout)
{
	std::vector<std::string>list = this->get_global_file_system_list();
	std::vector<std::string>::iterator it;
	bool flag = false;
	for( it = list.begin(); it != list.end(); it++)
	{
		OSDLOG(INFO, "Unmount the filesytem "<<*it);
		std::string mount_path = this->get_mount_path(path, *it);
		try
		{
			std::string umount_command = this->get_unmount_command(cmd, *it, "");
			OSDLOG(INFO, "Unmount command = "<<umount_command);
			int status = 20;
			FILE *fp;
			//while( timeout > 0)
			while(1)
			{
				CommandExecuter cmd_exec;
				cmd_exec.exec_command(umount_command.c_str(), &fp, &status);
				status = status>>8;
				OSDLOG(INFO, "Status of unmount command "<< umount_command<<" is "<<status);
				//if( status != HFS_BUSY )
				if (status != HFS_BUSY and status != HFS_NOT_READY and status != HFS_SERVER_BUSY)
				{
					flag = true;
					break;
				}
				else
				{
					OSDLOG(INFO, "Filesystem "<<*it<<" status is "<< status);
					//timeout = timeout - SLEEP_TIME;
					boost::this_thread::sleep(boost::posix_time::seconds(SLEEP_TIME));
					flag = false;
				}
			}
		}
		catch(...)
		{
			OSDLOG(INFO, "Exception accure during hfs unmount");
			flag = false;
		}
	}	
	OSDLOG(INFO, "Unmount_file_system return status "<<flag);
	return flag;
}

bool GLUtils::rename_gl_info_file(std::string version, int32_t gl_port)
{
	namespace fs = boost::filesystem;
	std::string dir = CONFIG_FILESYSTEM;
	fs::path someDir(dir);
	fs::directory_iterator end_iter;
	for( fs::directory_iterator dir_iter(someDir) ; dir_iter != end_iter ; ++dir_iter)
	{
		size_t pos = dir_iter->path().string().find(GLOBAL_LEADER_INFORMATION_FILE);
		if (fs::is_regular_file(dir_iter->path()) && pos != std::string::npos)
		{
			std::string current_version = dir_iter->path().string().substr(pos + GLOBAL_LEADER_INFORMATION_FILE.size() + 1);
			if(current_version == version)
			{
				OSDLOG(INFO, "GL Info file not changed since election started.");
				float new_gl_version = boost::lexical_cast<float>(current_version) + 1.0;
				std::string str_gl_version = boost::str(boost::format("%.1f") % new_gl_version);
				std::string gl_file = dir + "/" + GLOBAL_LEADER_INFORMATION_FILE + node_name_delimiter + str_gl_version;
				int ret = ::rename(dir_iter->path().string().c_str(), gl_file.c_str());
				if(ret == -1)
				{
					return false;
				}
				OSDLOG(INFO, "Renamed Info file from " << dir_iter->path().string() << " to " << gl_file);
				std::string node_id = this->get_my_service_id(gl_port);	//eg HN0101_61014
				std::string node_name = this->get_my_node_id();			//eg HN0101
				std::string node_ip = this->get_node_ip(node_name);
				std::string data = node_id + "\t" + node_ip + "\t" + boost::lexical_cast<std::string>(gl_port);
				int fd = ::open(gl_file.c_str(), O_RDWR | O_TRUNC, 0777);
				int64_t ret_value = ::write(fd, data.c_str(), data.size());
				if (ret_value == -1)
				{
					OSDLOG(INFO, "Unable to write in " << gl_file << " : " << strerror(errno));
					return false;
				}
				::close(fd);

				return true;
			}
			else
			{
				OSDLOG(INFO, "GL Info file changed since election started. Aborting this rename process");
			}
		}
	}
	return false;
}

void GLUtils::remove_GL_specific_file(std::string file_pattern)
{
	std::string dir = CONFIG_FILESYSTEM;
	std::string gl_file;
	namespace fs = boost::filesystem;
	fs::path someDir(dir);
	fs::directory_iterator end_iter;
	for( fs::directory_iterator dir_iter(someDir) ; dir_iter != end_iter ; ++dir_iter)
	{
		size_t pos = dir_iter->path().string().find(file_pattern);
		if (fs::is_regular_file(dir_iter->path()) && pos != std::string::npos)
		{
			OSDLOG(INFO, "Deleting " << dir_iter->path().string());
			int ret = ::unlink(dir_iter->path().string().c_str());
			return;
		}
	}

}

//std::pair<bool, std::string> GLUtils::check_file_existense_and_rename(std::string file_pattern)
std::pair<bool, std::string> GLUtils::check_file_existense_and_rename(std::string file_pattern)
{
	std::string dir;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config)
		dir = std::string(osd_config);
	else
		dir = CONFIG_FILESYSTEM;
	std::string gl_file;
	namespace fs = boost::filesystem;
	fs::path someDir(dir);
	fs::directory_iterator end_iter;
	for( fs::directory_iterator dir_iter(someDir) ; dir_iter != end_iter ; ++dir_iter)
	{
		size_t pos = dir_iter->path().string().find(file_pattern);
		if (fs::is_regular_file(dir_iter->path()) && pos != std::string::npos)
		{
			string suffix = dir_iter->path().string().substr(pos + file_pattern.size() + 1);
			float new_gl_version = boost::lexical_cast<float>(suffix) + 1.0;
			std::string str_gl_version = boost::str(boost::format("%.1f") % new_gl_version);
			gl_file = dir + "/" + file_pattern + node_name_delimiter + str_gl_version;
			int ret = ::rename(dir_iter->path().string().c_str(), gl_file.c_str());
			OSDLOG(INFO, "Renamed old status from " << dir_iter->path().string() << " to " << gl_file);
			return std::make_pair(true, gl_file);
		}
	}
	OSDLOG(INFO, "File with pattern " << file_pattern << " doesn't exist");
	return std::make_pair(false, gl_file);
}

/* Check GL Info or Status File existence and create/rename it */
std::pair<std::string,bool> GLUtils::create_and_rename_GL_specific_file(std::string file_pattern)
{
	std::string dir;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config)
		dir = std::string(osd_config);
	else
		dir = CONFIG_FILESYSTEM;
	std::string gl_file;
	std::pair<bool, std::string> result = this->check_file_existense_and_rename(file_pattern);
	gl_file = result.second;
	bool if_new = false;
	if(!result.first)
	{
		gl_file = dir + "/" + file_pattern + node_name_delimiter + "1.0";
		if_new = true;
		OSDLOG(INFO, "Creating new GL file: " << gl_file);
		int fd = open(gl_file.c_str(), O_CREAT|O_EXCL, 0777);
		::close(fd);
	}
	return std::make_pair(gl_file, if_new);
}

bool GLUtils::update_gl_info_file(int32_t gl_port)
{
	std::pair<std::string, bool> gl_info_file_details = this->create_and_rename_GL_specific_file(GLOBAL_LEADER_INFORMATION_FILE);

	bool if_created = gl_info_file_details.second;
	std::string gl_info_file = gl_info_file_details.first;
	
	GLUtils util_obj;
	std::string node_id = util_obj.get_my_service_id(gl_port);	//eg HN0101_61014
	std::string node_name = util_obj.get_my_node_id();			//eg HN0101
	std::string node_ip = util_obj.get_node_ip(node_name);
	std::string data = node_id + "\t" + node_ip + "\t" + boost::lexical_cast<std::string>(gl_port);
	if(!if_created)
	{
		std::string read_cmd = "cat " + gl_info_file;
		cool::ShellCommand command(read_cmd.c_str());
		cool::Option<cool::CommandResult> result  = command.run();
		std::string str = result.getRefValue().getOutputString();
		if (str.substr(0, str.find("\t")) != this->get_my_service_id(gl_port))
		{
			OSDLOG(ERROR, "I am not GL anymore");
			return false;
		}
	}
	int fd = ::open(gl_info_file.c_str(), O_RDWR | O_TRUNC, 0777);
	::write(fd, data.c_str(), data.size());
	::close(fd);
	return true;
}


std::vector<std::string> GLUtils::get_service_list()
{
	std::vector<std::string> service_list;
	service_list.push_back("proxy-server");
	service_list.push_back("object-server");	
	service_list.push_back("container-server");
	service_list.push_back("account-server");
	service_list.push_back("account-updater");
	return service_list;	
}

std::vector<std::string> GLUtils::create_file_system_list()
{
	std::vector<std::string> gfs_list;
	gfs_list.push_back("OSP_01");
	gfs_list.push_back("OSP_02");
	gfs_list.push_back("OSP_03");
	gfs_list.push_back("OSP_04");
	gfs_list.push_back("OSP_05");
	gfs_list.push_back("OSP_06");
	gfs_list.push_back("OSP_07");
	gfs_list.push_back("OSP_08");
	gfs_list.push_back("OSP_09");
	gfs_list.push_back("OSP_10");
	gfs_list.push_back(".osd_meta_config");
	return gfs_list;
}

std::vector<std::string> GLUtils::get_global_file_system_list()
{
	return this->create_file_system_list();
}

bool GLUtils::hfs_availability_check(std::string path)
{
	return this->is_mount( path);
}

bool GLUtils::is_mount(std::string path)
{
	struct stat statbuf, statbuf1;	
	int r = lstat( path.c_str(), &statbuf);
	if( r < 0)
	{
		if( errno == ENOENT)
			return false; 
	}
	if((statbuf.st_mode & S_IFMT) == S_IFLNK)
	{
		OSDLOG(INFO, "Unmount file never symbolic link");
		return false;
	}
	std::string path1 = path + "/" + "..";
	if(lstat( path1.c_str(), &statbuf1) < 0)
	{
		OSDLOG(INFO, "Mount failed "<<path1);
		return false;
	}
	if( statbuf.st_dev != statbuf1.st_dev )
	{
		OSDLOG(DEBUG, "File system is mounted "<<path1<<" "<<statbuf.st_dev<<" "<<statbuf1.st_dev );
		return true;
	}
	if( statbuf.st_ino == statbuf1.st_ino) 
		return true;
	return false;
}

boost::tuple<std::string, bool> GLUtils::find_file(std::string dir_path, std::string file_name)
{
	boost::filesystem::directory_iterator end_itr;
	try
	{
		for (  boost::filesystem::directory_iterator itr( dir_path ); itr != end_itr; ++itr)
		{
			if (  boost::filesystem::is_directory(itr->status()) )
				continue;
			else if (  boost::lexical_cast<std::string>(itr->path().filename()).find(file_name) != std::string::npos )
					return boost::make_tuple(boost::lexical_cast<std::string>(itr->path().filename()), true);
		}
	}
	catch(boost::filesystem::filesystem_error &ex)
	{
		OSDLOG(ERROR, "Filesystem not accessible: " << ex.what());
		return boost::make_tuple("", false);
	}
	return boost::make_tuple("", false);	
}

time_t GLUtils::parse_file_name(std::string file_name)
{
	const char* name = NULL;
	name = file_name.c_str();
	if( !name)
		return 0;
	name = file_name.c_str() + std::strlen(file_name.c_str()) - 1;
	while( name != file_name.c_str() && *name != '_')
		--name;
	if( name == file_name.c_str() )
		return 0;
	return boost::lexical_cast<time_t>(++name);
}

bool GLUtils::check_environment(std::string path)
{
	bool if_hfs_available = false;
	while(!if_hfs_available)
	{
		if_hfs_available = this->hfs_availability_check(path);
	}
	OSDLOG(INFO, "HFS is available");
	bool license = this->license_check();
	if (license)
	{
		//TODO: Start all functionality if part of cluster
		return true;
	}
	else
	{
		// Start only basic functionality
		return false;
	}
}

Bucket::Bucket()
{
}

void Bucket::set_fd_limit(int64_t limit)
{
	this->limit = limit;
}

bool Bucket::add_fd(
	int64_t fd,
	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm
)
{
	boost::mutex::scoped_lock lock(this->mtx);
//	this->list_of_fds.insert(fd);
	this->fd_comm_map.insert(std::make_pair(fd, sync_comm));
	return true;
}

boost::shared_ptr<Communication::SynchronousCommunication> Bucket::get_comm_obj(int64_t fd)
{
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj;
	std::map<int64_t, boost::shared_ptr<Communication::SynchronousCommunication> >::iterator it = this->fd_comm_map.find(fd);
	if(it != this->fd_comm_map.end())
		comm_obj = it->second;
	return comm_obj;
}

std::set<int64_t> Bucket::get_fd_list()
{
	boost::mutex::scoped_lock lock(this->mtx);
	std::set<int64_t> fd_list;
	std::map<int64_t, boost::shared_ptr<Communication::SynchronousCommunication> >::iterator it = this->fd_comm_map.begin();
	for(; it != this->fd_comm_map.end(); ++it)
	{
		fd_list.insert(it->first);
	}
	return fd_list;
}

void Bucket::remove_fd(int64_t fd)
{
	boost::mutex::scoped_lock lock(this->mtx);
	this->fd_comm_map.erase(fd);
//	this->list_of_fds.erase(fd);
}

int64_t Bucket::get_fd_limit()
{
	return this->limit;
}

void ServiceToRecordTypeBuilder::register_services(boost::shared_ptr<ServiceToRecordTypeMapper> mapper_ptr)
{
	mapper_ptr->register_service(CONTAINER_SERVICE, osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE);
	mapper_ptr->register_service(ACCOUNT_SERVICE, osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE);
	mapper_ptr->register_service(ACCOUNT_UPDATER, osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE);
	mapper_ptr->register_service(OBJECT_SERVICE, osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE);
}

void ServiceToRecordTypeBuilder::unregister_services(boost::shared_ptr<ServiceToRecordTypeMapper> mapper_ptr)
{
	mapper_ptr->unregister_service(CONTAINER_SERVICE);
	mapper_ptr->unregister_service(ACCOUNT_SERVICE);
	mapper_ptr->unregister_service(ACCOUNT_UPDATER);
	mapper_ptr->unregister_service(OBJECT_SERVICE);
}

void ServiceToRecordTypeMapper::register_service(std::string service, int id)
{
	this->service_to_record_map.insert(std::make_pair(service, id));
}

void ServiceToRecordTypeMapper::unregister_service(std::string service)
{
	std::map<std::string, int>::iterator it = this->service_to_record_map.find(service);
	if ( it != this->service_to_record_map.end() )
		this->service_to_record_map.erase(it);
}

std::string ServiceToRecordTypeMapper::get_name_for_type(int type)
{
	std::map<std::string, int>::iterator it = this->service_to_record_map.begin();
	for(; it != this->service_to_record_map.end(); ++it)
	{
		if(it->second == type)
			return it->first;
	}
	return "ERROR";
}

int ServiceToRecordTypeMapper::get_type_for_service(std::string service)
{
	std::map<std::string, int>::iterator it = this->service_to_record_map.find(service);
	if ( it != this->service_to_record_map.end() )
		return it->second;
	return -1;
}

std::vector<std::string> OsmUtils::list_files_in_creation_order(std::string path, std::string fileStartWith)
{
	std::string cmd = "ls -tr " + path + " | grep " + fileStartWith;
	cool::ShellCommand command(cmd);
	cool::Option<cool::CommandResult> result  = command.run();
	std::vector<std::string> str = result.getRefValue().getOutputLines();
	std::vector<std::string>::iterator it = str.begin();
	for(; it != str.end(); ++it)
		OSDLOG(INFO, "Listed file include: " << *it);
	return str;
}

std::list<std::string> OsmUtils::list_files( const char* path, const char* fileStartWith)
{
	DIR* dirFile = opendir( path );
	std::list<std::string> files;
	if (dirFile)
	{
		struct dirent* hFile;
		while ((hFile = readdir(dirFile)) != NULL )
		{
			//Ignore to current & parrent directories.
			if(!strcmp( hFile->d_name, "."  )) continue;
			if(!strcmp( hFile->d_name, ".." )) continue;

			// Hidden files ko bhi ignore maar...
			if(hFile->d_name[0] == '.') continue;

			if (strstr(hFile->d_name, fileStartWith))
				files.push_back(hFile->d_name);
		}
		closedir( dirFile );
	}
	return files;
}

int OsmUtils::createFile(const char* path, int mode)
{
	return creat(path, mode);

}

std::string OsmUtils::ItoStr(int x)
{
	stringstream ss;
	ss << x;
	return ss.str();
}



string OsmUtils::split(const std::string& s, std::string c) //split a string on the basis of a delimiter c.
{
/*	vector<string> v;
	string::size_type i = 0;
	string::size_type j = s.find(c);

	while (j != string::npos)
	{
		v.push_back(s.substr(i, j-i));
		i = ++j;
		j = s.find(c, j);

		if (j == string::npos)
			v.push_back(s.substr(i, s.length()));
	}
	return v;
*/
	return s.substr(s.find(c)+c.size(), s.size());
}

int OsmUtils::find_index(std::list<std::string> data_list, std::string data)
{
	bool flag = false;
	int i = 0;
	std::list<std::string>::iterator it = data_list.begin();
	for ( ; it != data_list.end(); ++it)
	{
		if (*it == data)
		{
			flag = true;
			break;
		}
		i++;
	}
	if(!flag)
	{return -1;}
	return i;

}

boost::tuple<std::string, uint32_t> OsmUtils::get_ip_and_port_of_gl()
{

	GLUtils utils;	
	std::vector<std::string> v;
/*	std::string token;
	std::string delimeter = "\t";
	std::size_t pos = 0;
	std::string str = utils.read_GL_file();
	while ((pos = str.find(delimeter)) != std::string::npos) 
	{
		token = str.substr(0, pos);
		v.push_back(token);
		str.erase(0, pos + delimeter.length());
	}
	*/
	int retry_count = 0;
	uint32_t port = 0;

	while(1) {
		std::string str = utils.read_GL_file();
		if(str.size() == 0) {
			OSDLOG(ERROR, "Unable to read gl info file in retry attempt " << retry_count);
			boost::this_thread::sleep(boost::posix_time::seconds(30));
			retry_count++;
			continue;
		}else {
			boost::split(v, str, boost::is_any_of("\t "));
			try {
				port = boost::lexical_cast<uint32_t>(v[2]);
			}catch(const boost::bad_lexical_cast& e) {
				OSDLOG(ERROR, "CORRUPTED ENTRIES IN GL INFO FILE");
				return boost::make_tuple(std::string(""), uint32_t(0)); 
			}
			return boost::make_tuple(v[1], port);
		}	
	}
	
	OSDLOG(ERROR, "FAILED TO READ GL INFO FILE IN 3 RETRY ATTEMPTS");
	return boost::make_tuple(std::string(""), uint32_t(0));	
}

int CommandExecutor::execute_command_in_bg(std::string command)
{
	FILE *proc = ::popen(command.c_str(),"r");
	if(proc == NULL)
		return -1;	//failure
	return 0;
}

void signal_callback_handler(int i)
{
	return;
}

int CommandExecutor::get_status() const
{
	return this->status;
}

int CommandExecutor::execute_command(std::string command)
{
	this->status = -1;
	signal(SIGUSR1, signal_callback_handler);
	FILE *proc = ::popen(command.c_str(),"r");
	if(proc == NULL)
		return -1;	//failure
	char buf[50];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) );
	int i = ::pclose(proc);
	this->status = i>>8;
	OSDLOG(INFO, "Return value after executing command is " << this->status);
	this->cond.notify_all();
	return this->status;		//success
}
/*
int CommandExecutor::execute_command_with_condition(std::string command, boost::condition_variable &cond)
{
	signal(SIGUSR1, signal_callback_handler);
	FILE *proc = ::popen(command.c_str(),"r");
	if(proc == NULL)
		return -1;	//failure
	char buf[50];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) );
	int i = ::pclose(proc);
	this->status = i>>8;
	OSDLOG(INFO, "Return value after executing command is " << this->status);
	cond.notify_all();
	return this->status;		//success
}*/

int CommandExecutor::execute_cmd_in_thread(std::string command, boost::system_time const timeout)
{
	boost::mutex mtx;
	boost::mutex::scoped_lock lock(mtx);
	boost::thread helper_thread(boost::bind(&CommandExecutor::execute_command, this, command));
	if(!this->cond.timed_wait(lock, timeout))
	{
		OSDLOG(ERROR, "Could not complete operation within timeout limit, killing thread and exiting");
		pthread_kill(helper_thread.native_handle(), SIGUSR1);
	}
	helper_thread.join();
	return this->status;

}
