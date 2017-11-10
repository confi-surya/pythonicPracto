#ifndef __OSM_UTILS_H__
#define __OSM_UTILS_H__

#include <set>
#include <list>
#include <vector>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/shared_ptr.hpp>	
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/lexical_cast.hpp>

#include "communication/message_binding.pb.h"
#include "communication/communication.h"
#include "libraryUtils/config_file_parser.h"
#include "osm/localLeader/recovery_handler.h"	
#include "libraryUtils/file_handler.h"
#define BOOST_FILESYSTEM_NO_DEPRECATED

#define HFS_BUSY 10
#define HFS_SERVER_BUSY 28
#define HFS_NOT_READY	9
#define SLEEP_TIME 10

static const std::string CONTAINER_SERVICE = "container-server";
static const std::string ACCOUNT_SERVICE = "account-server";
static const std::string ACCOUNT_UPDATER = "account-updater-server";
static const std::string OBJECT_SERVICE = "object-server";
static const std::string PROXY_SERVICE = "proxy-server";

#ifndef OBJECT_STORAGE_CONFIG
	const static std::string NODE_INFO_FILE_PATH = "/export/.osd_meta_config";
	const static std::string CONFIG_FILESYSTEM = "/export/.osd_meta_config";
#else
	const static std::string NODE_INFO_FILE_PATH = "/tmp/";
	const static std::string CONFIG_FILESYSTEM = "/tmp/";
#endif


const static uint64_t FAX_FD_LIMIT = 500;
const static std::string node_name_delimiter = "_";
const static std::string lock_file_delimeter = "-";
//const static std::string GLOBAL_LEADER_INFORMATION_FILE = "/export/osd_meta_config/osd_global_leader_information";
//const static std::string GLOBAL_LEADER_INFORMATION_FILE = "/remote/hydra042/gmishra/gl_config/osd_global_leader_information";
const static std::string GLOBAL_LEADER_INFORMATION_FILE = "osd_global_leader_information";
const static std::string GLOBAL_LEADER_STATUS_FILE = "osd_global_leader_status";

class GLUtils
{
		bool is_mount(std::string path);
	public:
		GLUtils();
		~GLUtils();
		std::string get_node_name(std::string service_id);
		cool::SharedPtr<config_file::OSMConfigFileParser> get_config();
		cool::SharedPtr<config_file::OsdServiceConfigFileParser> get_osm_timeout_config();
		cool::SharedPtr<config_file::ConfigFileParser> get_journal_config();
		uint64_t get_osm_port_from_id(std::string node_id);
		std::string get_node_ip(std::string node_id);
		std::string get_service_name(std::string service_id);
		bool verify_if_single_hydra();
		bool verify_if_alternate();
		std::string get_my_node_id();
		std::string get_my_service_id(int32_t port);
		bool verify_if_gl(bool if_cluster, int32_t port);
		enum network_messages::NodeStatusEnum verify_if_part_of_cluster(std::string node_id);
		bool verify_cluster_existence(std::string node_info_file_path, std::string config_fs_name);
		std::string read_GL_file();
		std::string get_node_id(std::string);
		std::string get_node_id(std::string node_name, int32_t port);
		bool license_check();
		bool hfs_availability_check(std::string path = ""); //[TODO}
		std::string get_osd_service_id(int port, std::string service_name);
		bool check_environment(std::string);
		std::set<int64_t> selectOnSockets(std::set<int64_t> fd_list, int & error, int timeout = 0);
		boost::tuple<std::string, bool> find_file(std::string dir_path, std::string file_name);
		time_t parse_file_name(std::string);
		uint16_t get_osm_port();
		uint8_t get_start_election_count();
		uint16_t get_thread_wait_timeout();
		bool rename_gl_info_file(std::string version, int32_t gl_port);
		std::pair<bool, std::string> check_file_existense_and_rename(std::string file_pattern);
		std::pair<std::string,bool> create_and_rename_GL_specific_file(std::string file_pattern);
		void remove_GL_specific_file(std::string file_pattern);
		bool update_gl_info_file(int32_t gl_port);
		std::vector<std::string> get_service_list();
		std::string get_service_id(std::string, std::string);
		std::string get_mount_path(std::string, std::string, bool create = false );
		std::string get_mount_command(std::string, std::string, std::string, std::string);	
		std::vector<std::string> get_global_file_system_list();
		std::vector<std::string> create_file_system_list();
		std::string get_unmount_command(std::string, std::string file_system, std::string options ="");
		std::string get_version_from_file_name(std::string);
		bool mount_file_system(std::string, std::string cmd, uint32_t);
		bool umount_file_system(std::string, std::string cmd, uint32_t);
		bool create_and_mount_file_system(std::string path, std::string cmd, std::string mkfs, std::string fsexist, uint32_t timeout);
		std::string get_log_file_name();
};

class Bucket
{
	public:
		Bucket();
		bool add_fd(int64_t fd, boost::shared_ptr<Communication::SynchronousCommunication> sync_comm);
		void remove_fd(int64_t fd);
		void set_fd_limit(int64_t limit);
		int64_t get_fd_limit();
		std::set<int64_t> get_fd_list();
		boost::shared_ptr<Communication::SynchronousCommunication> get_comm_obj(int64_t fd);
	private:
		int64_t limit;
		boost::mutex mtx;
		std::map<int64_t, boost::shared_ptr<Communication::SynchronousCommunication> > fd_comm_map;
};

class ServiceToRecordTypeMapper
{
	public:
		void register_service(std::string service, int id);
		void unregister_service(std::string service);
		int get_type_for_service(std::string service);
		std::string get_name_for_type(int type);
	private:
		std::map<std::string, int> service_to_record_map;
};

class ServiceToRecordTypeBuilder
{
	public:
		void register_services(boost::shared_ptr<ServiceToRecordTypeMapper> mapper_ptr);
		void unregister_services(boost::shared_ptr<ServiceToRecordTypeMapper> mapper_ptr);
};

class CommandExecutor
{
	public:
		int execute_command(std::string command);
		int execute_command_in_bg(std::string command);
		//void execute_command_with_condition(std::string command, boost::condition_variable &cond);
		int execute_cmd_in_thread(std::string command, boost::system_time const timeout);
		int get_status() const;
	private:
		int status;
		boost::condition_variable cond;
};

class OsmUtils
{
	public:
		std::string ItoStr(int);
		int createFile(const char*, int);
		int find_index(std::list<std::string>, std::string);
		std::string split(const std::string&, std::string);
		std::list<std::string> list_files( const char*, const char*);
		std::vector<std::string> list_files_in_creation_order(std::string path, std::string fileStartWith);
		boost::tuple<std::string, uint32_t> get_ip_and_port_of_gl();
};
#endif
