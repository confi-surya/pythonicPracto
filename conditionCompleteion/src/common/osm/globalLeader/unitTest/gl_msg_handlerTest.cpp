#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "libraryUtils/object_storage_log_setup.h"
#include "osm/globalLeader/gl_msg_handler.h"
#include "communication/message_type_enum.pb.h"
#include "osm/osmServer/msg_handler.h"
#include "communication/message.h"

namespace GlMsgHandlerTest
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

void global_leader_handler(boost::shared_ptr<MessageCommunicationWrapper> comm_wrap)
{
}

void state_msg_handler(boost::shared_ptr<MessageCommunicationWrapper> comm_wrap)
{
}

void monitor_service(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> llstrm , boost::shared_ptr<Communication::SynchronousCommunication> sync_comm)
{
}

void retrieve_node_status(boost::shared_ptr<comm_messages::NodeStatus> node_stat, boost::shared_ptr<Communication::SynchronousCommunication> sync_msg)
{
}

TEST(GlMsgHandlerTest, GlMsgHandler_Message_push_pop_Test)
{	
	boost::shared_ptr<gl_parser::GLMsgHandler> gl_msg_handler(new gl_parser::GLMsgHandler(boost::bind(&global_leader_handler, _1), boost::bind(&state_msg_handler, _1), boost::bind(&monitor_service, _1, _2), boost::bind(&retrieve_node_status, _1, _2)));
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr(new MessageResultWrap(1));
	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 60123));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj1(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm));
	gl_msg_handler->push(msg_comm_obj1);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj2 = gl_msg_handler->pop();
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr2 = msg_comm_obj2->get_msg_result_wrap();
	ASSERT_EQ(1, int (msg_result_wrap_ptr2->get_type()));
	gl_msg_handler->abort();
}

TEST(GlMsgHandlerTest, GlMsgHandler_handle_msg_Test)
{
	boost::shared_ptr<gl_parser::GLMsgHandler> gl_msg_handler2(new gl_parser::GLMsgHandler(boost::bind(&global_leader_handler, _1), boost::bind(&state_msg_handler, _1), boost::bind(&monitor_service, _1, _2), boost::bind(&retrieve_node_status, _1, _2)));
	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 60123));

	//Message type 1
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr(new MessageResultWrap(1));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj1(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm));

	//Message type 50
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr50(new MessageResultWrap(50));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj50(new MessageCommunicationWrapper(msg_result_wrap_ptr50, comm));
	//Message type 54
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr54(new MessageResultWrap(54));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj54(new MessageCommunicationWrapper(msg_result_wrap_ptr54, comm));
	
	//Message Type 9
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr9(new MessageResultWrap(9));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj9(new MessageCommunicationWrapper(msg_result_wrap_ptr9, comm));

	//Message type 15
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr15(new MessageResultWrap(15));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj15(new MessageCommunicationWrapper(msg_result_wrap_ptr15, comm));

	//Message type 11
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr11(new MessageResultWrap(11));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj11(new MessageCommunicationWrapper(msg_result_wrap_ptr11, comm));

	//Message type 7
	comm_messages::LocalLeaderStartMonitoring llstrm;
	llstrm.set_service_id("HN0101_61014_container-server");
	std::string ser_str;
	ASSERT_TRUE(llstrm.serialize(ser_str));

	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr7(new MessageResultWrap(7));
	char *ptr = new char[ser_str.length() + 1];
	strcpy(ptr, ser_str.c_str());
	uint32_t size = strlen((char *)ptr);
	ASSERT_TRUE(llstrm.deserialize(ser_str.c_str(), size));
	msg_result_wrap_ptr7->set_payload(ptr, size);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj7(new MessageCommunicationWrapper(msg_result_wrap_ptr7, comm));

	//Message type 19
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr19(new MessageResultWrap(19));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj19(new MessageCommunicationWrapper(msg_result_wrap_ptr19, comm));

	//Message type 65
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr65(new MessageResultWrap(65));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj65(new MessageCommunicationWrapper(msg_result_wrap_ptr65, comm));

	//Message type 
	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr5(new MessageResultWrap(5));
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj5(new MessageCommunicationWrapper(msg_result_wrap_ptr5, comm));

	//Message type 59
	boost::shared_ptr<ServiceObject> node (new ServiceObject());
	node->set_service_id("HN0101_61014_container-server");
	node->set_ip("192.168.101.1");
	node->set_port(61014);
	comm_messages::NodeStatus node_status(node);
	std::string ser_str1;
	ASSERT_TRUE(node_status.serialize(ser_str1));

	boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr59(new MessageResultWrap(59));
	char *ptr1 = new char[ser_str1.length() + 1];
	strcpy(ptr1, ser_str1.c_str());
	uint32_t size1 = strlen((char *)ptr1);
	ASSERT_TRUE(node_status.deserialize(ser_str1.c_str(), size1));
	msg_result_wrap_ptr59->set_payload(ptr1, size1);
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj59(new MessageCommunicationWrapper(msg_result_wrap_ptr59, comm));
	
	//Message type NULL
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj3;
	
	//gl_msg_handler2->push(msg_comm_obj3); //NULL. Remove this to pop below message objs. 
	gl_msg_handler2->push(msg_comm_obj1);
	gl_msg_handler2->push(msg_comm_obj50);
	gl_msg_handler2->push(msg_comm_obj54);
	gl_msg_handler2->push(msg_comm_obj9);
	gl_msg_handler2->push(msg_comm_obj15);
	gl_msg_handler2->push(msg_comm_obj11);
	gl_msg_handler2->push(msg_comm_obj19);
	gl_msg_handler2->push(msg_comm_obj7);
	gl_msg_handler2->push(msg_comm_obj65);
	gl_msg_handler2->push(msg_comm_obj5);
	gl_msg_handler2->push(msg_comm_obj59);
	boost::thread bthread(boost::bind(&gl_parser::GLMsgHandler::handle_msg, gl_msg_handler2));
	gl_msg_handler2->abort();
	bthread.join();

}

} //namespace GlMsgHandlerTest
