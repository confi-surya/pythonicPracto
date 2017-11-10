#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "osm/globalLeader/component_balancing.h"

//using namespace std::tr1;

namespace ComponentBalancingTest
{
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

cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");

std::string path = parser_ptr->osm_info_file_pathValue();
std::string service_name = "container-server";


TEST(ComponentBalancingTest, ComponentBalancing_get_target_node_for_recovery_Test)
{
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_balancer::ComponentBalancer> comp_balancer (new component_balancer::ComponentBalancer(map_manager, parser_ptr));
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));

	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord3(new recordStructure::NodeInfoFileRecord("HN0103_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord4(new recordStructure::NodeInfoFileRecord("HN0104_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));

	std::list<boost::shared_ptr<recordStructure::Record> > record_list;
	record_list.push_back(nifrecord1);
	record_list.push_back(nifrecord2);
	record_list.push_back(nifrecord3);
	record_list.push_back(nifrecord4);
	whandler3->add_record(record_list);
	
	std::vector<std::string> unhealty_node_list;
	unhealty_node_list.push_back("HN0101_60123");
	comp_balancer->set_unhealthy_node_list(unhealty_node_list);
	std::vector<std::string> healty_node_list;
	healty_node_list.push_back("HN0102_60123");
	healty_node_list.push_back("HN0103_60123");
	healty_node_list.push_back("HN0104_60123");
	comp_balancer->set_healthy_node_list(healty_node_list);
	ASSERT_EQ("HN0102_60123", comp_balancer->get_target_node_for_recovery("HN0103_60123_container-server"));
	ASSERT_EQ("HN0102_60123", comp_balancer->get_target_node_for_recovery("HN0103_60123_account-server"));
	ASSERT_EQ("HN0102_60123", comp_balancer->get_target_node_for_recovery("HN0103_60123_account-updater-server"));
	ASSERT_NE("HN0103_60123", comp_balancer->get_target_node_for_recovery("HN0103_60123_container-server")); //Would return HN0104

	
	comp_balancer->update_recovery_dest_map("HN0102_60123", "container-server");
	comp_balancer->update_recovery_dest_map("HN0102_60123", "account-server");
	comp_balancer->update_recovery_dest_map("HN0102_60123", "account-updater-server");

	//unhealty_node_list.push_back("HN0102_60123");
	//comp_balancer->set_unhealthy_node_list(unhealty_node_list);
	//healty_node_list.push_back("HN0102_60123");
	healty_node_list.clear();
	healty_node_list.push_back("HN0103_60123");
	healty_node_list.push_back("HN0104_60123");
	comp_balancer->set_healthy_node_list(healty_node_list);


	ASSERT_EQ("HN0103_60123", comp_balancer->get_target_node_for_recovery("HN0104_60123_container-server")); //Would return HN0103 
	ASSERT_EQ("HN0103_60123", comp_balancer->get_target_node_for_recovery("HN0104_60123_account-server")); //Would return HN0103 
	ASSERT_EQ("HN0103_60123", comp_balancer->get_target_node_for_recovery("HN0104_60123_account-updater-server")); //Would return HN0103 

	comp_balancer->update_recovery_dest_map("HN0104_60123", "container-server");
	ASSERT_EQ("NO_NODE_FOUND", comp_balancer->get_target_node_for_recovery("HN0104_60123_container-server"));
	
	comp_balancer->remove_entry_from_unhealthy_node_list("HN0102_60123"); //Would be added in recovered_node_list

	ASSERT_EQ("NO_NODE_FOUND", comp_balancer->get_target_node_for_recovery("HN0104_60123_container-server"));

	comp_balancer->update_recovery_dest_map("HN0103_60123", "container-server");
	comp_balancer->update_recovery_dest_map("HN0103_60123", "account-server");
	comp_balancer->update_recovery_dest_map("HN0103_60123", "account-updater-server");

	ASSERT_EQ("HN0103_60123", comp_balancer->get_target_node_for_recovery("HN0104_60123_container-server"));

	comp_balancer->update_recovery_dest_map("HN0103_60123", "container-server");
}

TEST(ComponentBalancingTest, ComponentBalancing_set_healthy_node_list_Test)
{
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_balancer::ComponentBalancer> comp_balancer (new component_balancer::ComponentBalancer(map_manager, parser_ptr));
	std::vector<std::string> healty_node_list;
	healty_node_list.push_back("HN0101_60123");
	healty_node_list.push_back("HN0102_60123");
	ASSERT_EQ(2, comp_balancer->set_healthy_node_list(healty_node_list)); 
	ASSERT_NE(4, comp_balancer->set_healthy_node_list(healty_node_list)); 
}

TEST(ComponentBalancingTest, ComponentBalancing_set_recovered_node_list_Test)
{
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_balancer::ComponentBalancer> comp_balancer (new component_balancer::ComponentBalancer(map_manager, parser_ptr));
	std::vector<std::string> recovered_node_list;
	recovered_node_list.push_back("HN0101_60123");
	recovered_node_list.push_back("HN0102_60123");
	ASSERT_EQ(2, comp_balancer->set_recovered_node_list(recovered_node_list)); 
	ASSERT_NE(4, comp_balancer->set_recovered_node_list(recovered_node_list)); 

}

TEST(ComponentBalancingTest, ComponentBalancing_set_unhealthy_node_list_Test)
{
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_balancer::ComponentBalancer> comp_balancer (new component_balancer::ComponentBalancer(map_manager, parser_ptr));
	std::vector<std::string> unhealty_node_list;
	unhealty_node_list.push_back("HN0101_60123");
	unhealty_node_list.push_back("HN0102_60123");

	ASSERT_EQ(2, comp_balancer->set_unhealthy_node_list(unhealty_node_list)); 
	ASSERT_NE(4, comp_balancer->set_unhealthy_node_list(unhealty_node_list)); 
	
	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while(!feof(proc) && fgets(buf,sizeof(buf),proc))
	{
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());

	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
}

TEST(ComponentBalancingTest, ComponentBalancing_check_if_nodes_already_recovered_Test)
{
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_balancer::ComponentBalancer> comp_balancer (new component_balancer::ComponentBalancer(map_manager, parser_ptr));
	std::vector<std::string> aborted_service_list;
	aborted_service_list.push_back("HN0103_60123_container-server");
	aborted_service_list.push_back("HN0101_60123_container-server");
	ASSERT_FALSE(comp_balancer->check_if_nodes_already_recovered(aborted_service_list));
	
	std::vector<std::string> unhealty_node_list;
	unhealty_node_list.push_back("HN0101_60123");
	unhealty_node_list.push_back("HN0102_60123");

	ASSERT_EQ(2, comp_balancer->set_unhealthy_node_list(unhealty_node_list)); 

	ASSERT_TRUE(comp_balancer->check_if_nodes_already_recovered(aborted_service_list));
	
	comp_balancer->remove_entry_from_unhealthy_node_list("HN0101_60123"); //Would be added in recovered_node_list
	
	ASSERT_TRUE(comp_balancer->check_if_nodes_already_recovered(aborted_service_list));
}

TEST(ComponentBalancingTest, ComponentBalancing_balance_osd_cluster_Test)
{
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord3(new recordStructure::NodeInfoFileRecord("HN0103_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord4(new recordStructure::NodeInfoFileRecord("HN0104_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        
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

	boost::shared_ptr<component_manager::ComponentMapManager> map_manager(new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_balancer::ComponentBalancer> cmp_bal (new component_balancer::ComponentBalancer(map_manager, parser_ptr));

	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > orig_basic_map;
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > orig_transient_map;
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > orig_planned_map;
	
	std::vector<std::string> node_add;
	std::vector<std::string> node_del;
	std::vector<std::string> node_rec;
	
	node_add.clear();
	node_add.push_back("HN0101_60123_container-server");
	
	std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> balancing_planned_pair;
	balancing_planned_pair = cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > balancing_planned = balancing_planned_pair.first;

	ASSERT_EQ(0, int(balancing_planned.size()));

	std::vector<std::string> healthy_nodes;
	healthy_nodes.push_back("HN0101_60123_container-server");
	healthy_nodes.push_back("HN0102_60123_container-server");


	balancing_planned_pair = cmp_bal->balance_osd_cluster(service_name, healthy_nodes, node_del, node_rec);
	ASSERT_EQ(1, int(balancing_planned_pair.first.size())); //Transfer 256 from HN0101 to HN0102
	
	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	ASSERT_EQ(1, int(orig_planned_map.size())); //Size is 1 as it contains container-server as key and 256 components in vector.
	ASSERT_EQ(0, int(orig_transient_map.size()));

	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map = balancing_planned_pair.first.find("HN0101_60123_container-server");
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2;
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	map_manager->update_transient_map(service_name, arg2);

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	ASSERT_EQ(0, int(orig_planned_map.size())); //Size is 0 as components are moved from planned to transient.
	ASSERT_EQ(1, int(orig_transient_map.size()));

	healthy_nodes.push_back("HN0103_60123_container-server");
	balancing_planned_pair = cmp_bal->balance_osd_cluster(service_name, healthy_nodes, node_del, node_rec);
	ASSERT_EQ(2, int(balancing_planned_pair.first.size())); //Transfer 85 from HN0101 and HN0102 to HN0103

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_global_planned_map = orig_planned_map.find(service_name);
	ASSERT_EQ(170, int(it_global_planned_map->second.size()));

	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator it_orig_transient_map = orig_transient_map.find(service_name);
	ASSERT_EQ(256, int(std::tr1::get<0>(it_orig_transient_map->second).size()));

	
	//Move components from Planned Map to Transient Map
	it_internal_planned_map = balancing_planned_pair.first.find("HN0101_60123_container-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	map_manager->update_transient_map(service_name, arg2);
	
	it_internal_planned_map = balancing_planned_pair.first.find("HN0102_60123_container-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	map_manager->update_transient_map(service_name, arg2);
	
	healthy_nodes.push_back("HN0104_60123_container-server");
	balancing_planned_pair = cmp_bal->balance_osd_cluster(service_name, healthy_nodes, node_del, node_rec);
	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	ASSERT_EQ(1, int(orig_planned_map.size())); //Size is 1 as it contains container-server as key and 128 components in vector.

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_global_planned_map = orig_planned_map.find(service_name);
	ASSERT_EQ(128, int(it_global_planned_map->second.size()));

	it_orig_transient_map = orig_transient_map.find(service_name);
	ASSERT_EQ(426, int(std::tr1::get<0>(it_orig_transient_map->second).size()));

	//Move components from Planned Map to Transient Map
	it_internal_planned_map = balancing_planned_pair.first.find("HN0101_60123_container-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	map_manager->update_transient_map(service_name, arg2);
	
	it_internal_planned_map = balancing_planned_pair.first.find("HN0102_60123_container-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	map_manager->update_transient_map(service_name, arg2);
	
	it_internal_planned_map = balancing_planned_pair.first.find("HN0103_60123_container-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	map_manager->update_transient_map(service_name, arg2);

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_orig_transient_map = orig_transient_map.find(service_name);
	ASSERT_EQ(554, int(std::tr1::get<0>(it_orig_transient_map->second).size()));

	map_manager->merge_basic_and_transient_map(); // Prepare new global map. 
	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_global_planned_map = orig_planned_map.find(service_name);
	ASSERT_EQ(0, int(orig_planned_map.size()));

	node_add.clear();
	node_add.push_back("HN0101_60123_account-server");
	node_add.push_back("HN0102_60123_account-server");
	node_add.push_back("HN0103_60123_account-server");
	node_add.push_back("HN0104_60123_account-server");

	service_name = "account-server";
	balancing_planned_pair = cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 


	node_add.clear();
	node_rec.clear();
	node_rec.push_back("HN0101_60123_account-server");
	node_rec.push_back("HN0102_60123_account-server");
	balancing_planned_pair = cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	ASSERT_EQ(2, int(balancing_planned_pair.first.size()));
	
	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_global_planned_map = orig_planned_map.find(service_name);
	ASSERT_EQ(256, int(it_global_planned_map->second.size()));
	
	it_internal_planned_map = balancing_planned_pair.first.find("HN0101_60123_account-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	ASSERT_EQ(128, int(arg2.size()));
	map_manager->update_transient_map(service_name, arg2);

	it_internal_planned_map = balancing_planned_pair.first.find("HN0102_60123_account-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	ASSERT_EQ(128, int(arg2.size()));
	map_manager->update_transient_map(service_name, arg2);

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_orig_transient_map = orig_transient_map.find(service_name);
	ASSERT_EQ(256, int(std::tr1::get<0>(it_orig_transient_map->second).size()));

	node_rec.push_back("HN0103_60123_account-server");
	balancing_planned_pair = cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	ASSERT_EQ(1, int(balancing_planned_pair.first.size()));

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_global_planned_map = orig_planned_map.find(service_name);
	ASSERT_EQ(256, int(it_global_planned_map->second.size()));
	
	it_internal_planned_map = balancing_planned_pair.first.find("HN0103_60123_account-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	ASSERT_EQ(256, int(arg2.size()));
	map_manager->update_transient_map(service_name, arg2);

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_orig_transient_map = orig_transient_map.find(service_name);
	ASSERT_EQ(512, int(std::tr1::get<0>(it_orig_transient_map->second).size()));

	map_manager->merge_basic_and_transient_map(); // Prepare new global map. 

	node_rec.clear();
	node_add.clear();
	node_add.push_back("HN0102_60123_account-server");
	node_add.push_back("HN0103_60123_account-server");
	balancing_planned_pair = cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	ASSERT_EQ(1, int(balancing_planned_pair.first.size())); //Entry for 1 source, HN0104
	
	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	it_global_planned_map = orig_planned_map.find(service_name);
	ASSERT_EQ(341, int(it_global_planned_map->second.size())); //171 on HN0102 and 170 on HN0103
	
	it_internal_planned_map = balancing_planned_pair.first.find("HN0104_60123_account-server");
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	ASSERT_EQ(341, int(arg2.size()));
	map_manager->update_transient_map(service_name, arg2);
	
	map_manager->merge_basic_and_transient_map(); // Prepare new global map. 
	
	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	ASSERT_EQ(0, int(orig_planned_map.size()));
	ASSERT_EQ(0, int(orig_transient_map.size()));
	
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > balancing_for_source;
	balancing_for_source.clear();
	arg2.clear();
	balancing_for_source = cmp_bal->get_new_balancing_for_source(service_name, "HN0104_60123_account-server", arg2);
	ASSERT_EQ(1, int(balancing_for_source.size()));

	map_manager->get_map(orig_basic_map, orig_transient_map, orig_planned_map);
	ASSERT_EQ(1, int(orig_planned_map.size()));
	it_global_planned_map = orig_planned_map.find(service_name);
	ASSERT_EQ(1, int(it_global_planned_map->second.size()));

	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while(!feof(proc) && fgets(buf,sizeof(buf),proc))
	{
	        std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	std::cout<<username<<std::endl;
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/object_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/account_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/container_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/accountUpdater_component_info_file.info"));


}

} //namespace ComponentBalancingTest
