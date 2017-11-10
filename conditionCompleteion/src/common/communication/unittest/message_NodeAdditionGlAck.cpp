#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include <list>

namespace NodeAdditionGlAck_message_test
{
TEST(NodeAdditionGlAckTest, explicit_constructor_check)
{
	int32_t code = 123;
        std::string msg = "Ack msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
	std::string service_id = "HN0101_proxy-server_61005";
	std::string ip = "192.168.131.64";
	uint64_t port = 61005;
	boost::shared_ptr<ServiceObject> service_obj(new ServiceObject());
	service_obj->set_service_id(service_id);
	service_obj->set_ip(ip);
	service_obj->set_port(port);
	std::list<boost::shared_ptr<ServiceObject> > service_obj_list;
	service_obj_list.push_back(service_obj);	
	comm_messages::NodeAdditionGlAck naga(service_obj_list,error_status);
        ASSERT_STREQ(service_id.c_str(), service_obj->get_service_id().c_str());
	ASSERT_STREQ(ip.c_str(), service_obj->get_ip().c_str());
	ASSERT_EQ(port,service_obj->get_port());
	ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

TEST(NodeAdditionGlAckTest, explicit_constructor_serialization_check)
{
	int32_t code = 123;
        std::string msg = "Ack msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
        std::string service_id = "HN0101_proxy-server_61005";
        std::string ip = "192.168.131.64";
	uint64_t port = 61005;
        boost::shared_ptr<ServiceObject> service_obj(new ServiceObject());
        service_obj->set_service_id(service_id);
        service_obj->set_ip(ip);
	service_obj->set_port(port);
	std::list<boost::shared_ptr<ServiceObject> > service_obj_list;
        service_obj_list.push_back(service_obj);
	comm_messages::NodeAdditionGlAck naga1(service_obj_list,error_status);
	std::string serialized;
        ASSERT_TRUE(naga1.serialize(serialized));
        uint64_t size = serialized.size();

        comm_messages::NodeAdditionGlAck naga2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(naga2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::NODE_ADDITION_GL_ACK, naga2.get_type());
	ASSERT_STREQ(service_id.c_str(), service_obj->get_service_id().c_str());
        ASSERT_STREQ(ip.c_str(), service_obj->get_ip().c_str());
        ASSERT_EQ(port,service_obj->get_port());
	ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

}

        

	
