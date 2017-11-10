#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "osm/globalLeader/state_change_initiator.h"

namespace SystemStateTest
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


cool::SharedPtr<config_file::ConfigFileParser> const get_journal_config_parser()
{
        cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
        if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/gl_journal.conf"));
        else
                config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/gl_journal.conf");
        return config_file_builder.buildConfiguration();
}

TEST(SystemStateTest, SystemState_StateChangeInitiatorMap_Test)
{
	boost::shared_ptr<recordStructure::RecoveryRecord> rec_record (new recordStructure::RecoveryRecord(system_state::NODE_DELETION, "HN0101_60123" , system_state::BALANCING_PLANNED));
	time_t timer;
	time(&timer);
	int64_t fd =11;
	int timeout = 2;
	boost::shared_ptr<system_state::StateChangeInitiatorMap> state_change_recovery(new system_state::StateChangeInitiatorMap(rec_record, fd, timer, 30, false, 0));
	boost::shared_ptr<system_state::StateChangeInitiatorMap> state_change_balancing(new system_state::StateChangeInitiatorMap(rec_record, fd, false));

	ASSERT_EQ(0, state_change_recovery->get_timeout());
	state_change_recovery->set_timeout(timeout);
	//ASSERT_EQ(0, state_change_recovery->get_timeout()); //get hbrt_failure_count from config and multiply with set value

	state_change_recovery->set_expected_msg_type(system_state::BALANCING_NEW);
	ASSERT_EQ(system_state::BALANCING_NEW, state_change_recovery->get_expected_msg_type());

	state_change_recovery->set_fd(fd);
	ASSERT_EQ(fd, state_change_recovery->get_fd());

	time_t timer1;
        time(&timer1);
	state_change_recovery->set_last_updated_time(timer1);
	ASSERT_EQ(timer1, state_change_recovery->get_last_updated_time());

	state_change_recovery->set_planned();
	ASSERT_TRUE(state_change_recovery->if_planned());

	state_change_recovery->reset_planned();
        ASSERT_FALSE(state_change_recovery->if_planned());
	boost::shared_ptr<recordStructure::RecoveryRecord> record = state_change_recovery->get_recovery_record();
	ASSERT_EQ(system_state::NODE_DELETION, record->get_operation());
}

TEST(StateChangeMonitorThreadTest, StateChangeMonitorThread_update_planned_components_Test)
{
        boost::shared_ptr<Bucket> bucket_ptr (new Bucket());
}

bool change_status_in_monitoring_component(std::string abc, int xyz)
{
	return true;
}

TEST(StateChangeThreadManagerTest, StateChangeThreadManager_update_state_table_Test)
{
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();

	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<system_state::StateChangeThreadManager> system_state_thread_manager (new system_state::StateChangeThreadManager(parser_ptr, map_manager, boost::bind(&change_status_in_monitoring_component, "HN0112", network_messages::NEW))); //TODO:pass shared function

	std::vector<std::string> state_change_ids = system_state_thread_manager->get_state_change_ids();
	ASSERT_EQ(0, int(state_change_ids.size()));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr(new MessageResultWrap(1));
        boost::shared_ptr<Communication::SynchronousCommunication> sync_comm1 (new Communication::SynchronousCommunication("192.168.101.1", 61014));
	boost::shared_ptr<MessageCommunicationWrapper> msg(new MessageCommunicationWrapper(msg_result_wrap_ptr, sync_comm1));
	ASSERT_FALSE(system_state_thread_manager->update_state_table(msg));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr1(new MessageResultWrap(1)); //TODO: Change to type 11 and prepare message and ASSERT_TRUE for update_state_table
        boost::shared_ptr<Communication::SynchronousCommunication> sync_comm2 (new Communication::SynchronousCommunication("192.168.101.1", 61014));
	boost::shared_ptr<MessageCommunicationWrapper> msg1(new MessageCommunicationWrapper(msg_result_wrap_ptr1, sync_comm2));
	ASSERT_FALSE(system_state_thread_manager->update_state_table(msg1)); //TODO: ASSERT_TREU for message type 11
}

TEST(StateChangeThreadManagerTest, StateChangeThreadManager_prepare_and_add_entry_Test)
{
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<system_state::StateChangeThreadManager> system_state_thread_manager (new system_state::StateChangeThreadManager(parser_ptr, map_manager, boost::bind(&change_status_in_monitoring_component, "HN0112", network_messages::NEW))); //TODO:pass shared function
	std::string node_id = "HN0101_61014";
	int operation = system_state::NODE_ADDITION;
	ASSERT_TRUE(system_state_thread_manager->prepare_and_add_entry(node_id, operation));
}

TEST(StateChangeMessageHandlerTest, StateChangeMessageHandler_handle_msg_Test)
{
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
	boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr(new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<system_state::StateChangeThreadManager> system_state_thread_manager (new system_state::StateChangeThreadManager(parser_ptr, component_map_mgr, boost::bind(&change_status_in_monitoring_component, "HN0112", network_messages::NEW)));
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));

	boost::shared_ptr<system_state::StateChangeMessageHandler> state_change_message_handler(new system_state::StateChangeMessageHandler(system_state_thread_manager, component_map_mgr, monitoring));

        boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 60123));

	//Message Type 1
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr1(new MessageResultWrap(1));
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj1(new MessageCommunicationWrapper(msg_result_wrap_ptr1, comm));

	//Message Type 9
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr9(new MessageResultWrap(9));
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj9(new MessageCommunicationWrapper(msg_result_wrap_ptr9, comm));

	//Message Type 15
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
        std::string node_id = "HN0101_61014";
        msg->set_service_id(node_id);
        msg->set_ip("192.168.101.1");
        msg->set_port(61014);
        //boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	ASSERT_FALSE(monitoring->if_node_already_exists(node_id));
	ASSERT_TRUE(monitoring->add_entry_in_map(msg));
	ASSERT_TRUE(monitoring->add_entries_while_recovery("HN0102_61014", gl_monitoring::HRBT, network_messages::FAILED));
	ASSERT_TRUE(monitoring->add_entries_while_recovery("HN0103_61014", gl_monitoring::HRBT, network_messages::RECOVERED));
	ASSERT_TRUE(monitoring->add_entries_while_recovery("HN0104_61014", gl_monitoring::HRBT, network_messages::RETIRED));

	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr15a(new MessageResultWrap(15));
	comm_messages::GetServiceComponent servcom("HN0101_61014_container-server");
	std::string ser_str1;
	ASSERT_TRUE(servcom.serialize(ser_str1));
	char *ptr1 = new char[ser_str1.length() + 1];
        strcpy(ptr1, ser_str1.c_str());
        uint32_t size1 = strlen((char *)ptr1);
	ASSERT_TRUE(servcom.deserialize(ser_str1.c_str(), size1));
	msg_result_wrap_ptr15a->set_payload(ptr1, size1);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj15a(new MessageCommunicationWrapper(msg_result_wrap_ptr15a, comm));
	
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr15b(new MessageResultWrap(15));
	comm_messages::GetServiceComponent servcomb("HN0113_61014_container-server");
	std::string ser_str2;
	ASSERT_TRUE(servcomb.serialize(ser_str2));
	char *ptr2 = new char[ser_str2.length() + 1];
        strcpy(ptr2, ser_str2.c_str());
        uint32_t size2 = strlen((char *)ptr2);
	ASSERT_TRUE(servcomb.deserialize(ser_str2.c_str(), size2));
	msg_result_wrap_ptr15b->set_payload(ptr2, size2);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj15b(new MessageCommunicationWrapper(msg_result_wrap_ptr15b, comm));
	
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr15c(new MessageResultWrap(15));
	comm_messages::GetServiceComponent servcomc("HN0102_61014_container-server");
	std::string ser_str3;
	ASSERT_TRUE(servcomc.serialize(ser_str3));
	char *ptr3 = new char[ser_str3.length() + 1];
        strcpy(ptr3, ser_str3.c_str());
        uint32_t size3 = strlen((char *)ptr3);
	ASSERT_TRUE(servcomc.deserialize(ser_str3.c_str(), size3));
	msg_result_wrap_ptr15c->set_payload(ptr3, size3);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj15c(new MessageCommunicationWrapper(msg_result_wrap_ptr15c, comm));
	
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr15d(new MessageResultWrap(15));
	comm_messages::GetServiceComponent servcomd("HN0103_61014_container-server");
	std::string ser_str4;
	ASSERT_TRUE(servcomd.serialize(ser_str4));
	char *ptr4 = new char[ser_str4.length() + 1];
        strcpy(ptr4, ser_str4.c_str());
        uint32_t size4 = strlen((char *)ptr4);
	ASSERT_TRUE(servcomd.deserialize(ser_str4.c_str(), size4));
	msg_result_wrap_ptr15d->set_payload(ptr4, size4);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj15d(new MessageCommunicationWrapper(msg_result_wrap_ptr15d, comm));
	

	//Message Type 5
	comm_messages::RecvProcStartMonitoring recvprocstrm("1");
	std::string ser_str6;
	ASSERT_TRUE(recvprocstrm.serialize(ser_str6));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr5(new MessageResultWrap(5));
	char *ptr6 = new char[ser_str6.length() + 1];
	strcpy(ptr6, ser_str6.c_str());
	uint32_t size6 = strlen((char *)ptr6);
	ASSERT_TRUE(recvprocstrm.deserialize(ser_str6.c_str(), size6));
	msg_result_wrap_ptr5->set_payload(ptr6, size6);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj5(new MessageCommunicationWrapper(msg_result_wrap_ptr5, comm));

	//Message Type 11
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr11(new MessageResultWrap(11));
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj11(new MessageCommunicationWrapper(msg_result_wrap_ptr11, comm));

	//Message Type 13
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr13(new MessageResultWrap(13));
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj13(new MessageCommunicationWrapper(msg_result_wrap_ptr13, comm));

	//Message Type 65
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr65a(new MessageResultWrap(65));
	boost::shared_ptr<ErrorStatus> err_stata (new ErrorStatus);
	err_stata->set_code(1); //TODO:Pass valid code
	err_stata->set_msg("OSDException::OSDSuccessCode::NODE_FAILOVER");
	comm_messages::NodeFailover nodefailovera("HN0102_61014", err_stata);
	std::string ser_str7;
	ASSERT_TRUE(nodefailovera.serialize(ser_str7));
	char *ptr7 = new char[ser_str7.length() + 1];
	strcpy(ptr7, ser_str7.c_str());
	uint32_t size7 = strlen((char *)ptr7);
	ASSERT_TRUE(nodefailovera.deserialize(ser_str7.c_str(), size7));
	msg_result_wrap_ptr65a->set_payload(ptr7, size7);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj65a(new MessageCommunicationWrapper(msg_result_wrap_ptr65a, comm));
	
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr65b(new MessageResultWrap(65));
	boost::shared_ptr<ErrorStatus> err_statb (new ErrorStatus);
	err_statb->set_code(1); //TODO:Pass valid code
	err_statb->set_msg("OSDException::OSDSuccessCode::NODE_FAILOVER");
	comm_messages::NodeFailover nodefailoverb("HN0112_61014", err_statb);
	std::string ser_str9;
	ASSERT_TRUE(nodefailoverb.serialize(ser_str9));
	char *ptr9 = new char[ser_str9.length() + 1];
	strcpy(ptr9, ser_str9.c_str());
	uint32_t size9 = strlen((char *)ptr9);
	ASSERT_TRUE(nodefailoverb.deserialize(ser_str9.c_str(), size9));
	msg_result_wrap_ptr65b->set_payload(ptr9, size9);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj65b(new MessageCommunicationWrapper(msg_result_wrap_ptr65b, comm));

	//Message Type 54
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr54a(new MessageResultWrap(54));
	boost::shared_ptr<ServiceObject> msg1 (new ServiceObject());
        std::string node_id_54a = "HN0101_61014"; //Registered
        msg1->set_service_id(node_id_54a);
        msg1->set_ip("192.168.101.1");
        msg1->set_port(61014);
	comm_messages::NodeRetire noderetirea(msg1);
	std::string ser_str10;
	ASSERT_TRUE(noderetirea.serialize(ser_str10));
	char *ptr10 = new char[ser_str10.length() + 1];
	strcpy(ptr10, ser_str10.c_str());
	uint32_t size10 = strlen((char *)ptr10);
	ASSERT_TRUE(noderetirea.deserialize(ser_str10.c_str(), size10));
	msg_result_wrap_ptr54a->set_payload(ptr10, size10);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj54a(new MessageCommunicationWrapper(msg_result_wrap_ptr54a, comm));

	boost::shared_ptr<ServiceObject> msg2 (new ServiceObject());
        std::string node_id_54b = "HN0104_61014"; //Retired
        msg2->set_service_id(node_id_54b);
        msg2->set_ip("192.168.101.1");
        msg2->set_port(61014);
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr54b(new MessageResultWrap(54));
	comm_messages::NodeRetire noderetireb(msg2);
	std::string ser_str12;
	ASSERT_TRUE(noderetireb.serialize(ser_str12));
	char *ptr12 = new char[ser_str12.length() + 1];
	strcpy(ptr12, ser_str12.c_str());
	uint32_t size12 = strlen((char *)ptr12);
	ASSERT_TRUE(noderetireb.deserialize(ser_str12.c_str(), size12));
	msg_result_wrap_ptr54b->set_payload(ptr12, size12);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj54b(new MessageCommunicationWrapper(msg_result_wrap_ptr54b, comm));

	boost::shared_ptr<ServiceObject> msg3 (new ServiceObject());
        std::string node_id_54c = "HN0114_61014"; //Invalid
        msg3->set_service_id(node_id_54c);
        msg3->set_ip("192.168.101.1");
        msg3->set_port(61014);
	//ASSERT_TRUE(monitoring->add_entry_in_map(msg3));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr54c(new MessageResultWrap(54));
	comm_messages::NodeRetire noderetirec(msg3);
	std::string ser_str13;
	ASSERT_TRUE(noderetirec.serialize(ser_str13));
	char *ptr13 = new char[ser_str13.length() + 1];
	strcpy(ptr13, ser_str13.c_str());
	uint32_t size13 = strlen((char *)ptr13);
	ASSERT_TRUE(noderetirec.deserialize(ser_str13.c_str(), size13));
	msg_result_wrap_ptr54c->set_payload(ptr13, size13);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj54c(new MessageCommunicationWrapper(msg_result_wrap_ptr54c, comm));

	boost::shared_ptr<ServiceObject> msg4 (new ServiceObject());
        std::string node_id_54d = "HN0103_61014"; //Recovered
        msg4->set_service_id(node_id_54d);
        msg4->set_ip("192.168.101.1");
        msg4->set_port(61014);
	//ASSERT_TRUE(monitoring->add_entry_in_map(msg4));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr54d(new MessageResultWrap(54));
	comm_messages::NodeRetire noderetired(msg4);
	std::string ser_str14;
	ASSERT_TRUE(noderetired.serialize(ser_str14));
	char *ptr14 = new char[ser_str14.length() + 1];
	strcpy(ptr14, ser_str14.c_str());
	uint32_t size14 = strlen((char *)ptr14);
	ASSERT_TRUE(noderetired.deserialize(ser_str14.c_str(), size14));
	msg_result_wrap_ptr54d->set_payload(ptr14, size14);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj54d(new MessageCommunicationWrapper(msg_result_wrap_ptr54d, comm));


	//Message Type 77
	/*boost::shared_ptr<ServiceObject> msg1 (new ServiceObject());
        std::string node_id_77 = "HN0101_61014"; //Registered
        msg1->set_service_id(node_id_77);
        msg1->set_ip("192.168.101.1");
        msg1->set_port(61014);
	//ASSERT_TRUE(monitoring->add_entry_in_map(msg1));
	*/
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr77a(new MessageResultWrap(77));
	comm_messages::NodeStopLL nodestopll(msg1);
	std::string ser_str10_77a;
	ASSERT_TRUE(nodestopll.serialize(ser_str10_77a));
	char *ptr10_77a = new char[ser_str10_77a.length() + 1];
	strcpy(ptr10_77a, ser_str10_77a.c_str());
	uint32_t size10_77a = strlen((char *)ptr10_77a);
	ASSERT_TRUE(nodestopll.deserialize(ser_str10_77a.c_str(), size10_77a));
	msg_result_wrap_ptr77a->set_payload(ptr10_77a, size10_77a);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj77a(new MessageCommunicationWrapper(msg_result_wrap_ptr77a, comm));

	
	/*boost::shared_ptr<ServiceObject> msg3 (new ServiceObject());
        std::string node_id_77c = "HN0114_61014"; //Invalid
        msg3->set_service_id(node_id_77c);
        msg3->set_ip("192.168.101.1");
        msg3->set_port(61014);
	//ASSERT_TRUE(monitoring->add_entry_in_map(msg3));
	*/
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr77c(new MessageResultWrap(77));
	comm_messages::NodeStopLL nodestopll3(msg3);
	std::string ser_str13_77c;
	ASSERT_TRUE(nodestopll3.serialize(ser_str13_77c));
	char *ptr13_77c = new char[ser_str13_77c.length() + 1];
	strcpy(ptr13_77c, ser_str13_77c.c_str());
	uint32_t size13_77c = strlen((char *)ptr13_77c);
	ASSERT_TRUE(nodestopll3.deserialize(ser_str13_77c.c_str(), size13_77c));
	msg_result_wrap_ptr77c->set_payload(ptr13_77c, size13_77c);
        boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj77c(new MessageCommunicationWrapper(msg_result_wrap_ptr77c, comm));




	state_change_message_handler->push(msg_comm_obj1);
	state_change_message_handler->push(msg_comm_obj9);
	state_change_message_handler->push(msg_comm_obj15a);
	state_change_message_handler->push(msg_comm_obj15b);
	state_change_message_handler->push(msg_comm_obj15c);
	state_change_message_handler->push(msg_comm_obj15d);
	//state_change_message_handler->push(msg_comm_obj5); //Segmentation fault in send_ack_for_rec_strm()
	//state_change_message_handler->push(msg_comm_obj11);
	//state_change_message_handler->push(msg_comm_obj13);
	state_change_message_handler->push(msg_comm_obj65a);
	state_change_message_handler->push(msg_comm_obj65b);
	state_change_message_handler->push(msg_comm_obj54a);
	state_change_message_handler->push(msg_comm_obj54b);
	state_change_message_handler->push(msg_comm_obj54c);
	state_change_message_handler->push(msg_comm_obj54d);
	state_change_message_handler->push(msg_comm_obj77a);
	state_change_message_handler->push(msg_comm_obj77c);
	boost::thread bthread(boost::bind(&system_state::StateChangeMessageHandler::handle_msg, state_change_message_handler));
	state_change_message_handler->abort();
	bthread.join();
	std::cout<<"state_change_message_handler thread terminated"<<std::endl;
}

TEST(StateChangeThreadManagerTest, StateChangeThreadManager_monitor_table_Test)
{
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<system_state::StateChangeThreadManager> system_state_thread_manager (new system_state::StateChangeThreadManager(parser_ptr, map_manager, boost::bind(&change_status_in_monitoring_component, "HN0112", network_messages::NEW))); //TODO:pass shared function
	
}

TEST(StateChangeThreadManagerTest, StateChangeThreadManager_start_recovery_Test)
{
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
	cool::SharedPtr<config_file::ConfigFileParser> journal_parser_ptr = get_journal_config_parser();
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<system_state::StateChangeThreadManager> system_state_thread_manager (new system_state::StateChangeThreadManager(parser_ptr, map_manager, boost::bind(&change_status_in_monitoring_component, "HN0112", network_messages::NEW))); //TODO:pass shared function
	std::vector<std::string> nodes_registered;
	std::vector<std::string> nodes_failed;
	std::vector<std::string> nodes_retired;
	//nodes_registered.push_back();

	std::string command = "whoami";
        FILE *proc = popen(command.c_str(),"r");
        char buf[1024];
        while ( !feof(proc) && fgets(buf,sizeof(buf),proc) )
        {
                //std::cout << buf << std::endl;
        }
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
        boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));

}


} //namespace SystemStateTest
