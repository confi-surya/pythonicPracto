#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "libraryUtils/object_storage_log_setup.h"
#include "osm/globalLeader/global_leader.h"
#include "communication/message_type_enum.pb.h"
#include "osm/osmServer/msg_handler.h"


namespace GlobalLeaderTest
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

TEST(GlobalLeaderTest, GlobalLeader_all_Test)
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
	std::string journal_path = "/export/home/" + username;
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));

	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_61014", "192.168.101.1", 61014, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_61014", "192.168.101.1", 61014, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord3(new recordStructure::NodeInfoFileRecord("HN0103_61014", "192.168.101.1", 61014, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord4(new recordStructure::NodeInfoFileRecord("HN0104_61014", "192.168.101.1", 61014, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	std::list<boost::shared_ptr<recordStructure::Record> > record_list;
	record_list.push_back(nifrecord1);
	record_list.push_back(nifrecord2);
	record_list.push_back(nifrecord3);
	record_list.push_back(nifrecord4);
	
	boost::shared_ptr<global_leader::GlobalLeader> gl(new global_leader::GlobalLeader());
	boost::thread bthread(boost::bind(&global_leader::GlobalLeader::start, gl));
	sleep(5);
	gl->abort();
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr(new MessageResultWrap(2));
	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 61014));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj1(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm));
	gl->push(msg_comm_obj1);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj2 = gl->pop();
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr2 = msg_comm_obj2->get_msg_result_wrap();
	ASSERT_EQ(2, int (msg_result_wrap_ptr2->get_type()));

	std::string dir;
	dir = std::string(osd_config);
	boost::filesystem::remove(boost::filesystem::path(dir + "/osd_global_leader_information_1.0"));
	boost::filesystem::remove(boost::filesystem::path(dir + "/osd_global_leader_status_1.0"));

	//Message type 50
	boost::shared_ptr<ServiceObject> node (new ServiceObject());
	node->set_service_id("HN0101");
	node->set_ip("192.168.101.1");
	node->set_port(61014);
	std::list<boost::shared_ptr<ServiceObject> > node_list;
	node_list.push_back(node);
	comm_messages::NodeAdditionCli node_add(node_list);
	std::string ser_str1;
	ASSERT_TRUE(node_add.serialize(ser_str1));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr50(new MessageResultWrap(50));
	char *ptr1 = new char[ser_str1.length() + 1];
	strcpy(ptr1, ser_str1.c_str());
	uint32_t size1 = strlen((char *)ptr1);
	ASSERT_TRUE(node_add.deserialize(ser_str1.c_str(), size1));
	msg_result_wrap_ptr50->set_payload(ptr1, size1);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj50(new MessageCommunicationWrapper(msg_result_wrap_ptr50, comm));

	//Message Type 71
	boost::shared_ptr<ServiceObject> node2 (new ServiceObject());
	node2->set_service_id("HN0101");
	node2->set_ip("192.168.101.1");
	node2->set_port(61014);
	comm_messages::NodeDeleteCli node_del(node2);
	std::string ser_str2;
	ASSERT_TRUE(node_del.serialize(ser_str2));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr71(new MessageResultWrap(71));
	char *ptr2 = new char[ser_str2.length() + 1];
	strcpy(ptr2, ser_str2.c_str());
	uint32_t size2 = strlen((char *)ptr2);
	ASSERT_TRUE(node_del.deserialize(ser_str2.c_str(), size2));
	msg_result_wrap_ptr71->set_payload(ptr2, size2);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj71(new MessageCommunicationWrapper(msg_result_wrap_ptr71, comm));
	
	//GET_OBJECT_VERSION message type
	comm_messages::GetVersionID get_version("HN0101_61014_container-server");
	std::string ser_str;
	ASSERT_TRUE(get_version.serialize(ser_str));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr19(new MessageResultWrap(19));
	char *ptr = new char[ser_str.length() + 1];
	strcpy(ptr, ser_str.c_str());
	uint32_t size = strlen((char *)ptr);
	ASSERT_TRUE(get_version.deserialize(ser_str.c_str(), size));
	msg_result_wrap_ptr19->set_payload(ptr, size);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj19(new MessageCommunicationWrapper(msg_result_wrap_ptr19, comm));

	//Message Type 73
	comm_messages::NodeRejoinAfterRecovery node_rejoin("HN0101_61014", "192.168.101.1", 61014);
	std::string ser_str3;
	ASSERT_TRUE(node_rejoin.serialize(ser_str3));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr73(new MessageResultWrap(73));
	char *ptr3 = new char[ser_str3.length() + 1];
	strcpy(ptr3, ser_str3.c_str());
	uint32_t size3 = strlen((char *)ptr3);
	ASSERT_TRUE(node_rejoin.deserialize(ser_str3.c_str(), size3));
	msg_result_wrap_ptr73->set_payload(ptr3, size3);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj73(new MessageCommunicationWrapper(msg_result_wrap_ptr73, comm));

	
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj0; 
	gl->push(msg_comm_obj50);
	gl->push(msg_comm_obj71);
	gl->push(msg_comm_obj19);
	gl->push(msg_comm_obj73);
	gl->push(msg_comm_obj0);

	gl->handle_main_queue_msg();

	//gl->success_handler(msg_result_wrap_ptr);
	//gl->failure_handler();
	
	sleep(5);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3( new osm_info_file_manager::NodeInfoFileReadHandler(path));
        std::list<std::string> list_of_node_ids_received = rhandler3->list_node_ids();
	ASSERT_EQ(1, int (list_of_node_ids_received.size()));
	sleep(5);

	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
}

} //namespace GlMsgHandlerTest
