#include <iostream>
#include <boost/shared_ptr.hpp>
#include "osm/osmServer/osm_info_file.h"
#include "libraryUtils/record_structure.h"
#include "osm/globalLeader/global_leader.h"
#include "osm/globalLeader/component_balancing.h"
#include "libraryUtils/osd_logger_iniliazer.h"
#include "libraryUtils/object_storage_exception.h"
#include <sys/resource.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

cool::SharedPtr<config_file::OSMConfigFileParser> const get_config_parser()
{
	cool::config::ConfigurationBuilder<config_file::OSMConfigFileParser> config_file_builder;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/osm.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/osm.conf");
	return config_file_builder.buildConfiguration();
}


int main(int argc, char **argv)
{
        ::testing::InitGoogleTest(&argc, argv);
	
	
	boost::shared_ptr<OSD_Logger::OSDLoggerImpl>logger;
	GLUtils gl_utils_obj;
	logger.reset(new OSD_Logger::OSDLoggerImpl("osm"));
	try
	{
		logger->initialize_logger();
		OSDLOG(INFO, "logger initiallization succussfuly");
	}
	catch(...)
	{
	//	shutDown.signal();
	}														        
cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
std::string path = parser_ptr->osm_info_file_pathValue();

		//Prepare Node Info File
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord3(new recordStructure::NodeInfoFileRecord("HN0103_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord4(new recordStructure::NodeInfoFileRecord("HN0104_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        //std::string path = "/export/home/smalhotra/";
        
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
        boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler( new osm_info_file_manager::NodeInfoFileReadHandler(path));

	std::list<boost::shared_ptr<recordStructure::Record> >record_list;
        record_list.push_back(nifrecord1);
        record_list.push_back(nifrecord2);
        record_list.push_back(nifrecord3);
        record_list.push_back(nifrecord4);

       	whandler->add_record(record_list);
	
	//------------------------------Component Balancing Tests

	std::string service_name = "container-server";

	//component_balancer::ComponentBalancer cmp_bal(basic_map, transient_map, planned_map);
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager(new component_manager::ComponentMapManager(parser_ptr));
	component_balancer::ComponentBalancer *cmp_bal = new component_balancer::ComponentBalancer(map_manager, parser_ptr);
	std::vector<std::string> node_add;
	std::vector<std::string> node_del;
	std::vector<std::string> node_rec;
	
	std::string nid = "HN0102_60123";
	std::list<std::string> list1;
	list1.push_back(nid);
	//cmp_bal.set_unhealthy_node_list(list1);
	
	node_add.clear();
	node_add.push_back("HN0101_60123_container-server");
	cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	//node_add.push_back("HN0102_60123_container-server");

	//node_rec.push_back("HN0103_60123_container-server");
	//node_del.push_back("HN0102_60123_container-server");
	
	//cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	
	node_add.clear();
	node_add.push_back("HN0102_60123_container-server");
	//node_add.push_back("HN0103_60123_container-server");
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map;
	new_planned_map = cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	//cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 

	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map = new_planned_map.find("HN0101_60123_container-server");
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2;
	arg2.clear();
	arg2 = it_internal_planned_map->second;

	map_manager->update_transient_map(service_name, arg2);
	
	
	node_add.clear();
	node_del.clear();
	node_del.push_back("HN0101_60123_container-server");
	node_add.push_back("HN0102_60123_container-server");
	node_add.push_back("HN0103_60123_container-server");
	node_add.push_back("HN0104_60123_container-server");
	cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	

	//---------------------------------------------------
	return RUN_ALL_TESTS();
}

