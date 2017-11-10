#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "osm/globalLeader/component_balancing.h"
#include "communication/message_type_enum.pb.h"

//using namespace std::tr1;

namespace ComponentManagerTest
{
std::string service_name = "container-server";

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


TEST(ComponentManagerTest, ComponentManager_all_Test)
{
	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) )
	{
		//std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	std::cout<<username<<std::endl;


        boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager_new (new component_manager::ComponentMapManager(parser_ptr));
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

	std::vector<std::string> node_add;
	std::vector<std::string> node_del;
	std::vector<std::string> node_rec;

	node_add.clear();
	node_add.push_back("HN0101_60123_container-server");
        comp_balancer->get_new_planned_map(service_name, node_add, node_del, node_rec);
	node_add.clear();
	node_add.push_back("HN0101_60123_account-server");
        comp_balancer->get_new_planned_map("account-server", node_add, node_del, node_rec);
	node_add.clear();
	node_add.push_back("HN0101_60123_account-updater-server");
        comp_balancer->get_new_planned_map("account-updater-server", node_add, node_del, node_rec);

	ASSERT_EQ(4, map_manager->get_global_map_version()); 	//Initially 1, add 1 for every service >> 1+1+1+1 = 4.
	map_manager->set_global_map_version(11);
	ASSERT_EQ(11, map_manager->get_global_map_version());

	std::vector<uint32_t> my_components = map_manager->get_component_ownership("HN0101_60123_container-server");
	ASSERT_EQ(512, int(my_components.size()));

	map_manager_new->recover_from_component_info_file(); 
	ASSERT_EQ(4, map_manager_new->get_global_map_version());
	
	boost::shared_ptr<recordStructure::ComponentInfoRecord> serv_info_rec;
	serv_info_rec = map_manager->get_service_object("HN0102_60123_container-server");
	serv_info_rec = map_manager->get_service_object("HN0101_60123_container-server");
	if (serv_info_rec)
	{
		ASSERT_EQ("HN0101_60123_container-server", serv_info_rec->get_service_id());
		ASSERT_EQ("192.168.131.123", serv_info_rec->get_service_ip());
		ASSERT_EQ(61007, serv_info_rec->get_service_port());
	}

	recordStructure::ComponentInfoRecord comp_rec1 = map_manager->get_service_info("HN0101_60123_container-server");

	ASSERT_EQ("HN0101_60123_container-server", comp_rec1.get_service_id());
	//ASSERT_EQ(nifrecord1->get_containerService_port(), comp_rec1.get_service_port());
	ASSERT_EQ("192.168.131.123", comp_rec1.get_service_ip());
	

	std::vector<recordStructure::NodeInfoFileRecord> nifrecords;
	nifrecords.push_back(*nifrecord1);
	nifrecords.push_back(*nifrecord2);
	nifrecords.push_back(*nifrecord3);
	nifrecords.push_back(*nifrecord4);
	
	recordStructure::ComponentInfoRecord comp_rec2 = map_manager->get_service_info("HN0101_60123_container-server", nifrecords);

	ASSERT_EQ("HN0101_60123_container-server", comp_rec2.get_service_id());
	ASSERT_EQ(nifrecord1->get_containerService_port(), comp_rec2.get_service_port());
	ASSERT_EQ("192.168.131.123", comp_rec2.get_service_ip());

	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > b_map = map_manager->get_basic_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it_b_map;
	it_b_map = b_map.find("container-server");
	ASSERT_EQ(3, int (b_map.size()));
	ASSERT_EQ(512, int (std::tr1::get<0>(it_b_map->second).size()));

	
	node_add.push_back("HN0102_60123_container-server");
	std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> new_planned_map_pair;
	new_planned_map_pair = comp_balancer->get_new_planned_map(service_name, node_add, node_del, node_rec);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map = new_planned_map_pair.first;
	node_add.clear();
	node_add.push_back("HN0102_60123_account-server");
	comp_balancer->get_new_planned_map("account-server", node_add, node_del, node_rec);
	node_add.clear();
	node_add.push_back("HN0102_60123_account-updater-server");
	comp_balancer->get_new_planned_map("account-updater-server", node_add, node_del, node_rec);
	
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map = new_planned_map.find("HN0101_60123_container-server");
        std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2;
        arg2.clear();
        arg2 = it_internal_planned_map->second;
        int internal_planned_map_size = arg2.size();

	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > p_map = map_manager->get_planned_map();
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_p_map;
	it_p_map = p_map.find("container-server");
	ASSERT_EQ(3, int (p_map.size()));
	ASSERT_EQ(internal_planned_map_size, int ((it_p_map->second).size()));

	map_manager->update_transient_map(service_name, arg2);
	map_manager->update_transient_map("account-server", arg2);
	map_manager->update_transient_map("account-updater-server", arg2);
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > t_map = map_manager->get_transient_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator it_t_map;
	it_t_map = t_map.find("container-server");
	ASSERT_EQ(3, int (t_map.size()));
	ASSERT_EQ(internal_planned_map_size, int (std::tr1::get<0>(it_t_map->second).size()));

	map_manager_new->update_transient_map_during_recovery(service_name, arg2);
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > t_map_new = map_manager->get_transient_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator it_t_map_new;
	it_t_map_new = t_map_new.find("container-server");
	ASSERT_EQ(3, int (t_map_new.size()));
	ASSERT_EQ(internal_planned_map_size, int (std::tr1::get<0>(it_t_map_new->second).size()));


	std::vector<boost::shared_ptr<recordStructure::Record> > Serv_comp_coup = map_manager->convert_full_transient_map_into_journal_record();
	//ASSERT_EQ(std::tr1::get<0>(it_t_map->second).size(), Serv_comp_coup.size());
	ASSERT_EQ(3, int (Serv_comp_coup.size())); //expected only 1 entry for container-server

	std::vector<boost::shared_ptr<recordStructure::Record> > tmap_journal_rec;
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator it = std::tr1::get<0>(it_t_map->second).begin();

	for (; it != std::tr1::get<0>(it_t_map->second).end(); ++it)
	{
		tmap_journal_rec.push_back(map_manager->convert_tmap_into_journal_record(it_t_map->first, 2)); //type
	}

	ASSERT_EQ(256, int (tmap_journal_rec.size()));

	map_manager->merge_basic_and_transient_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > t_map_new_1 = map_manager->get_transient_map();
	ASSERT_EQ(0, int (t_map_new_1.size()));

	ASSERT_TRUE(map_manager->write_in_component_file(CONTAINER_SERVICE, osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE));
	
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >& b_map_2 = map_manager->get_basic_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it_b_map_2;
	it_b_map_2 = b_map_2.find("container-server");

	std::vector<recordStructure::ComponentInfoRecord> comp_rec = std::tr1::get<0>(it_b_map_2->second);
	map_manager->prepare_basic_map(ACCOUNT_SERVICE, comp_rec);
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it_b_map_3;
        it_b_map_3 = b_map_2.find(ACCOUNT_SERVICE);
	ASSERT_EQ(3, int (b_map_2.size()));
	ASSERT_EQ(512, int (std::tr1::get<0>(it_b_map_3->second).size()));

	std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> get_map_full;
	get_map_full = map_manager->get_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > get_map = get_map_full.first;

	ASSERT_EQ(3, int (get_map.size()));

	map_manager->clear_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > b_map_1 = map_manager->get_basic_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > t_map_1 = map_manager->get_transient_map();
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > p_map_1 = map_manager->get_planned_map();
	ASSERT_EQ(0, int (b_map_1.size()));
	ASSERT_EQ(0, int (p_map_1.size()));
	ASSERT_EQ(0, int (t_map_1.size()));

	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord2(new recordStructure::ComponentInfoRecord("HN0102_60123_container-server", "192.168.131.12", 61007));

	map_manager->add_entry_in_global_planned_map("account-server", 0, *cifrecord2);
	ASSERT_EQ(3, int (p_map.size()));
	ASSERT_TRUE(map_manager->remove_entry_from_global_planned_map("account-server", 0));
	ASSERT_TRUE(map_manager->add_object_entry_in_basic_map("object-server", "HN0101_60123_object-server"));

	node_add.clear();
	node_del.clear();
	node_rec.clear();
	node_add.push_back("HN0101_60123_account-server");
	std::string acc_serv_name = "account-server";
	node_add.push_back("HN0102_60123_account-server");
	comp_balancer->get_new_planned_map(acc_serv_name, node_add, node_del, node_rec);
	map_manager->merge_basic_and_transient_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > t_map_account = map_manager->get_transient_map();
	ASSERT_EQ(0, int(t_map_account.size()));

	node_add.clear();
	node_del.clear();
	node_rec.clear();
	node_add.push_back("HN0101_60123_account-updater-server");
	std::string accup_serv_name = "account-updater-server";
	comp_balancer->get_new_planned_map(accup_serv_name, node_add, node_del, node_rec);
	map_manager->merge_basic_and_transient_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > t_map_accountUpdater = map_manager->get_transient_map();
	ASSERT_EQ(0, int(t_map_accountUpdater.size()));
}

/*TEST(ComponentManagerTest, ComponentManager_get_global_map_Test)
{
	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) )
	{
		//std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	std::cout<<username<<std::endl;


	boost::shared_ptr<component_manager::ComponentMapManager> map_manager_new (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<component_balancer::ComponentBalancer> comp_balancer (new component_balancer::ComponentBalancer(map_manager_new, parser_ptr));
	
	std::vector<std::string> node_add;
	std::vector<std::string> node_del;
	std::vector<std::string> node_rec;

	node_add.clear();
	node_del.clear();
	node_rec.clear();
	node_add.push_back("HN0101_60123_account-updater-server");
	std::string accup_serv_name = "account-updater-server";
	comp_balancer->get_new_planned_map(accup_serv_name, node_add, node_del, node_rec);
	node_add.push_back("HN0102_60123_account-updater-server");
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map_AU;
	new_planned_map_AU = comp_balancer->get_new_planned_map(accup_serv_name, node_add, node_del, node_rec);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map_AU = new_planned_map_AU.find("HN0101_60123_account-updater-server");
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2_AU;
	arg2_AU.clear();
	arg2_AU = it_internal_planned_map_AU->second;
	map_manager_new->update_transient_map(accup_serv_name, arg2_AU);
	
	std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> get_gbl_map_accountUpdater, get_gbl_map_account, get_gbl_map_container;
	get_gbl_map_accountUpdater = map_manager_new->get_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > get_map_accountUpdater = get_gbl_map_accountUpdater.first;
	ASSERT_EQ(1, int (get_map_accountUpdater.size()));
	
	node_add.clear();
	node_del.clear();
	node_rec.clear();
	node_add.push_back("HN0101_60123_account-server");
	std::string acc_serv_name = "account-server";
	comp_balancer->get_new_planned_map(acc_serv_name, node_add, node_del, node_rec);
	node_add.push_back("HN0102_60123_account-server");
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map_AS;
	new_planned_map_AS = comp_balancer->get_new_planned_map(acc_serv_name, node_add, node_del, node_rec);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map_AS = new_planned_map_AS.find("HN0101_60123_account-server");
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2_AS;
	arg2_AS.clear();
	arg2_AS = it_internal_planned_map_AS->second;
	map_manager_new->update_transient_map(acc_serv_name, arg2_AS);

	get_gbl_map_account = map_manager_new->get_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > get_map_account = get_gbl_map_account.first;
	ASSERT_EQ(2, int (get_map_account.size()));

	node_add.clear();
	node_del.clear();
	node_rec.clear();
	node_add.push_back("HN0101_60123_container-server");
	std::string cont_serv_name = "container-server";
	comp_balancer->get_new_planned_map(cont_serv_name, node_add, node_del, node_rec);
	node_add.push_back("HN0102_60123_container-server");
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map_CS;
	new_planned_map_CS = comp_balancer->get_new_planned_map(cont_serv_name, node_add, node_del, node_rec);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map_CS = new_planned_map_CS.find("HN0101_60123_container-server");
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2_CS;
	arg2_CS.clear();
	arg2_CS = it_internal_planned_map_CS->second;
	map_manager_new->update_transient_map(cont_serv_name, arg2_CS);

	get_gbl_map_container = map_manager_new->get_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > get_map_container = get_gbl_map_container.first;
	ASSERT_EQ(3, int (get_map_container.size()));

	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/object_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/account_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/container_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/accountUpdater_component_info_file.info"));

}*/

TEST(ComponentManagerTest, ComponentManager_service_component_ack_Test)
{

	boost::shared_ptr<comm_messages::GetServiceComponentAck> ownership_ack_ptr(new comm_messages::GetServiceComponentAck());

	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 60123));

	boost::shared_ptr<component_manager::ComponentMapManager> map_manager_new (new component_manager::ComponentMapManager(parser_ptr));

	map_manager_new->send_get_service_component_ack(ownership_ack_ptr, comm);
}

TEST(ComponentManagerTest, ComponentManager_send_ack_for_comp_trnsfr_msg_Test)
{
	boost::shared_ptr<comm_messages::GetServiceComponentAck> ownership_ack_ptr(new comm_messages::GetServiceComponentAck());

	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 60123));

	boost::shared_ptr<component_manager::ComponentMapManager> map_manager_new (new component_manager::ComponentMapManager(parser_ptr));

	map_manager_new->send_ack_for_comp_trnsfr_msg(comm, messageTypeEnum::typeEnums::COMP_TRANSFER_INFO_ACK);
	map_manager_new->send_ack_for_comp_trnsfr_msg(comm, messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT_ACK);

}

TEST(ComponentManagerTest, ComponentManager_send_get_global_map_ack_Test)
{
	boost::shared_ptr<comm_messages::GetServiceComponentAck> ownership_ack_ptr(new comm_messages::GetServiceComponentAck());

	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 60123));

	boost::shared_ptr<component_manager::ComponentMapManager> map_manager_new (new component_manager::ComponentMapManager(parser_ptr));

	map_manager_new->clear_global_map();
	map_manager_new->send_get_global_map_ack(comm);
}
} //namespace ComponentManagerTest
