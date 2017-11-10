#ifndef __OSMINFOFILE_H_2424__
#define __OSMINFOFILE_H_2424__

#include <boost/thread/mutex.hpp>

#include "libraryUtils/file_handler.h"
#include "libraryUtils/serializor.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/config_file_parser.h"

#include "osm/globalLeader/monitoring.h"
#include "osm/globalLeader/component_balancing.h"

#define MAX_READ_CHUNK_SIZE 65536

namespace component_manager
{
	class ComponentMapManager;
}

namespace osm_info_file_manager
{

const static std::string NODE_INFO_FILE_NAME = "node_info_file.info";
const static std::string ACCOUNT_COMPONENT_INFO_FILE_NAME = "account_component_info_file.info";
const static std::string CONTAINER_COMPONENT_INFO_FILE_NAME = "container_component_info_file.info";
const static std::string ACCOUNTUPDATER_COMPONENT_INFO_FILE_NAME = "accountUpdater_component_info_file.info";
const static std::string OBJECT_COMPONENT_INFO_FILE_NAME = "object_component_info_file.info";

enum infoFileType
{
        NODE_INFO_FILE = 0,
        ACCOUNT_COMPONENT_INFO_FILE = 1,
        CONTAINER_COMPONENT_INFO_FILE = 2,
        ACCOUNTUPDATER_COMPONENT_INFO_FILE = 3,
        OBJECT_COMPONENT_INFO_FILE = 4
};

class NodeInfoFileReadHandler
{
	public:
		NodeInfoFileReadHandler(std::string path, boost::shared_ptr<gl_monitoring::Monitoring> monitoring);
		NodeInfoFileReadHandler(std::string path, boost::function<void(
					recordStructure::NodeInfoFileRecord)> push_node_info_file_records_into_memory);
		NodeInfoFileReadHandler(std::string path);
		//NodeInfoFileReadHandler(std::string path, cool::SharedPtr<config_file::ConfigFileParser> parser_ptr);
		virtual ~NodeInfoFileReadHandler();
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> get_record_from_id(std::string id);
		std::list<std::string> list_node_ids();
                bool recover_record();
		std::list<std::pair<std::string, int> > get_node_status_map();

	private:
		std::string path;
		boost::function<void(recordStructure::NodeInfoFileRecord)> push_node_info_file_records_into_memory;
		boost::shared_ptr<gl_monitoring::Monitoring> monitoring;
                std::vector<char> buffer;
                char * cur_buffer_ptr;
                boost::shared_ptr<serializor_manager::Deserializor> deserializor;
                boost::shared_ptr<fileHandler::FileHandler> file_handler;
		std::string recovery_initializer;

		bool init();
		void read_record();
		bool prepare_read_record();
                void recover_info_file_record(boost::shared_ptr<recordStructure::Record> record);
		void prepare_list(
			std::list<std::string> &return_list,
			std::list<std::pair<std::string, int> > &node_status_map
		);
		boost::shared_ptr<recordStructure::Record> get_next_record();
};

class ComponentInfoFileReadHandler
{
	public:
		ComponentInfoFileReadHandler(std::string path);
		virtual ~ComponentInfoFileReadHandler();
		bool recover_distributed_service_record(int infoFileType, boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd);
		bool recover_object_service_record(int infoFileType, std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list);
	private:
		std::string path;
                std::vector<char> buffer;
                char * cur_buffer_ptr;
                boost::shared_ptr<serializor_manager::Deserializor> deserializor;
                boost::shared_ptr<fileHandler::FileHandler> file_handler;

		bool init(int infoFileType);
		void read_record(int infoFileType, boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd, std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list);
		bool prepare_read_record(int infoFileType);
		boost::shared_ptr<recordStructure::Record> get_next_record();
                bool recover_record(int infoFileType, boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd, std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list);
                void recover_info_file_record(int infoFileType, boost::shared_ptr<recordStructure::Record> record, boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd, std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list);

};
	
class NodeInfoFileWriteHandler
{
	public:
		//NodeInfoFileWriteHandler(std::string path, cool::SharedPtr<config_file::ConfigFileParser> parser_ptr);
		NodeInfoFileWriteHandler(std::string path);
		virtual ~NodeInfoFileWriteHandler();
		bool init();
                bool add_record(std::list<boost::shared_ptr<recordStructure::Record> > record_list);
                bool update_record(std::string node_id, int node_status);
                bool delete_record(std::string node_id);

	protected:
		std::string path;
                boost::mutex mtx;
                std::vector<char> buffer, readbuff;
		int64_t max_file_length;
		boost::shared_ptr<fileHandler::FileHandler> file_handler, read_file_handler;
                serializor_manager::Serializor * serializor;
                boost::shared_ptr<serializor_manager::Deserializor> deserializor;
		char * cur_read_buffer_ptr;
		bool add(boost::shared_ptr<recordStructure::Record> record);
                void process_write(recordStructure::Record * record, int64_t record_size);
                bool delete_update_helper(boost::shared_ptr<recordStructure::Record> record);
                void rename_temp_file();

};

class ComponentInfoFileWriteHandler
{
	public:
		ComponentInfoFileWriteHandler(std::string path);
		virtual ~ComponentInfoFileWriteHandler();
		bool init(int infoFileType);
                bool add_record(int infoFileType, boost::shared_ptr<recordStructure::Record> record);
		bool update_record(int infoFileType, boost::shared_ptr<recordStructure::Record> record);
		bool delete_record(int infoFileType, std::string deleted_service_id);
		void update_basic_map();
	protected:
		std::string path;
                boost::mutex mtx;
                std::vector<char> buffer, readbuff;
		int64_t max_file_length;
                boost::shared_ptr<fileHandler::FileHandler> file_handler, read_file_handler;
                serializor_manager::Serializor * serializor;
                boost::shared_ptr<serializor_manager::Deserializor> deserializor;
		boost::shared_ptr<component_manager::ComponentMapManager> comp_map_ptr;
		char * cur_read_buffer_ptr;
		bool add(boost::shared_ptr<recordStructure::Record> record);
                void process_write(recordStructure::Record * record, int64_t record_size);
                bool delete_update_helper(int infoFileType, boost::shared_ptr<recordStructure::Record> record);
                void rename_temp_file(int infoFileType);

};

std::string const make_info_file_path(int infoFileType, std::string const path);
std::string const make_temp_file_path(int infoFileType, std::string const path);

}// osm_info_file_manager namespace ends
#endif
