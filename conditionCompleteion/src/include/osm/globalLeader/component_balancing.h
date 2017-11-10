#ifndef __COMPONENT_BALANCING_H__
#define __COMPONENT_BALANCING_H__

#include <iostream>
#include "osm/osmServer/osm_info_file.h"
#include "osm/globalLeader/component_manager.h"
#include "osm/osmServer/utils.h"

//using namespace std::tr1;

namespace component_balancer
{

class ComponentBalancer
{
	public:
		//ComponentBalancer();
		ComponentBalancer(
			boost::shared_ptr<component_manager::ComponentMapManager> &map_manager,
			cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr
		);

		/*	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > &basic_map,
			std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > &transient_map, 
			std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > &planned_map
		*/
		virtual ~ComponentBalancer();
		void reinstantiate_balancer();
		std::string get_target_node_for_recovery(std::string);
		void update_recovery_dest_map(std::string, std::string);
		std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> get_new_planned_map(
				std::string service_type, 
				std::vector<std::string> node_addition, 
				std::vector<std::string> node_deletion, 
				std::vector<std::string> node_recovery,
				bool flag = false
				);
		std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> balance_osd_cluster(
				std::string service_type, 
				std::vector<std::string> node_addition, 
				std::vector<std::string> node_deletion, 
				std::vector<std::string> node_recovery
				);
		int set_healthy_node_list(std::vector<std::string> healthy_node_list);
		int set_unhealthy_node_list(std::vector<std::string> unhealthy_node_list);
		int set_revovered_node_list(std::vector<std::string> recovered_node_list);
		void remove_entry_from_unhealthy_node_list(std::string node_id_to_be_removed);

		void push_node_info_file_records_into_memory(recordStructure::NodeInfoFileRecord node_info_file_record);
		bool check_if_nodes_already_recovered(std::vector<std::string> aborted_service_list);
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > get_new_balancing_for_source(
				std::string service_type,
				std::string source_service_name,
				std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > planned_components 
				);
		int set_recovered_node_list(std::vector<std::string> recovered_node_list);

	private:
	        //boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> node_info_file_reader;
	        boost::shared_ptr<component_manager::ComponentMapManager> map_manager;
		std::string node_info_file_path;
		std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > orig_basic_map;
		//TODO: Get it verified from Sanchit
		std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, 
						recordStructure::ComponentInfoRecord> >, float> > orig_transient_map;
		std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > orig_planned_map;
		std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > internal_planned_map;
		boost::shared_ptr<ServiceToRecordTypeMapper> service_to_record_type_map_ptr;
                //boost::mutex mtx;
		boost::recursive_mutex mtx;
		std::vector<recordStructure::NodeInfoFileRecord> node_info_file_records;
		std::vector<std::string> all_node_list, unhealthy_node_list, healthy_node_list, recovered_node_list;
		std::vector<std::string> list_of_services_for_addition, list_of_services_for_deletion, list_of_services_for_recovery;
		std::vector<std::tr1::tuple<std::string, int32_t, int32_t> > source_services, destination_services;
		std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> > internal_basic_map;
	        std::map<std::string, std::vector<std::string> > node_recovery_dest_map;
		int32_t total_no_of_components;
		void prepare_internal_basic_map(std::string service_type);
		void merge_transient_into_internal_basic_map(std::string service_name);
		void merge_planned_into_internal_basic_map(std::string service_name);
		void remove_value_from_internal_basic_map(int32_t component_number);
		void remove_planned_from_internal_basic_map(int32_t component_number);
		bool add_entry_in_internal_basic_map(int32_t component_number, std::string service_id);
		void transfer_components(std::string service_name);
		bool balance_components(std::string service_name);
};

}
#endif
