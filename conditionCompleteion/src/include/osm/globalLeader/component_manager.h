#ifndef __GLOBAL_MAP_COMPONENT_MANAGER__
#define __GLOBAL_MAP_COMPONENT_MANAGER__

#include <map>
#include <vector>
#include <utility>

#include "communication/message.h"
#include "osm/osmServer/utils.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"

//using namespace std::tr1;

namespace component_manager
{

//static const std::string COMPONENT_FILE_PATH = "/tmp/";
//static const std::string COMPONENT_FILE_PATH = "/export/osd_meta_config/";

class ComponentMapManager
{
	public:
		ComponentMapManager(cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr);
/*		void update_transferred_comp_in_transient_map(
			std::string service_name,
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > transferred_comp
		);
*/
		std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >& get_transient_map();
		std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >& get_basic_map();

		std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >& get_planned_map();

		void get_map(
			std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > &orig_basic_map,
			std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > &orig_transient_map,
			std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > &orig_planned_map
			);

		bool write_in_component_file(std::string service_name, int info_file);
		
		std::pair<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, bool> update_transient_map(
			std::string service_name,
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > transferred_comp
		);

		void update_transient_map_during_recovery(
			std::string service_name,
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > transferred_comp
		);

		boost::shared_ptr<recordStructure::ComponentInfoRecord> get_service_object(std::string service_id);

		bool add_object_entry_in_basic_map(
			std::string service_type,
			std::string service_id	
		);

		void update_object_component_info_file(
			std::vector<std::string> nodes_registered,
			std::vector<std::string> nodes_failed
		);

		void prepare_basic_map(
			std::string service_type, 
			std::vector<recordStructure::ComponentInfoRecord> comp_info_records
		);

		void clear_global_map();

		void clear_global_planned_map();

		bool remove_entry_from_global_planned_map(
			std::string service_type,
			int32_t component_number
		);
		size_t get_transient_map_size();
		void add_entry_in_global_planned_map(
			std::string service_type,
			int32_t component_number,
			recordStructure::ComponentInfoRecord
		);
		void merge_basic_and_transient_map();

		std::vector<boost::shared_ptr<recordStructure::Record> > convert_full_transient_map_into_journal_record();

		boost::shared_ptr<recordStructure::Record> convert_tmap_into_journal_record(std::string service_name, int type);

		std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> get_global_map();
				// it will merge basic map and transient map, and then will return the map

		recordStructure::ComponentInfoRecord get_service_info(std::string service_id);

		recordStructure::ComponentInfoRecord get_service_info(
			std::string service_id,
			std::vector<recordStructure::NodeInfoFileRecord> node_info_file_rcrd
		);

/*		std::string get_target_node_for_recovery(
			std::vector<std::string> unhealthy_nodes,
			std::vector<std::string> nodes_as_rec_destination
		);
*/
		float get_global_map_version();
		void set_global_map_version(float);
		void send_get_global_map_ack(boost::shared_ptr<Communication::SynchronousCommunication> comm);
		void recover_from_component_info_file();
		void recover_components_for_service(std::string service_name);
		bool send_ack_for_comp_trnsfr_msg(boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr, int ack_type);
		void send_get_service_component_ack(
			boost::shared_ptr<comm_messages::GetServiceComponentAck> serv_comp_ptr,
			boost::shared_ptr<Communication::SynchronousCommunication> comm
		);
		std::vector<uint32_t> get_component_ownership(std::string service_id);
		void set_transient_map_version(float version);
		float get_transient_map_version();
		void remove_object_entry_from_map(std::string service_id);
		bool is_global_planned_empty(std::string service_name);


	private:
		boost::recursive_mutex mtx;
		float global_map_version;
		float transient_map_version;
		std::string node_info_file_path;
		std::string osm_info_file_path;

		std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > transient_map;							//Key will be service-name and value will be vector of tuples
		
		std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > basic_map;
		
		std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > planned_map;
		
		boost::shared_ptr<ServiceToRecordTypeMapper> service_to_record_type_map_ptr;
};

} // namespace component_manager

#endif
