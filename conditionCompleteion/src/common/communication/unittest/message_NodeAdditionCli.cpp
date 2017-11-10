#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include <list>

namespace NodeAdditionCli_message_test
{
TEST(NodeAdditionCliTest, explicit_constructor_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
        std::string ip = "192.168.131.64";
	uint64_t port= 61005;
	boost::shared_ptr<ServiceObject> service_obj(new ServiceObject());
        service_obj->set_service_id(service_id);
        service_obj->set_ip(ip);
	service_obj->set_port(port);
	std::list<boost::shared_ptr<ServiceObject> > node_list;
	node_list.push_back(service_obj);
	comm_messages::NodeAdditionCli nac(node_list);
        ASSERT_STREQ(service_id.c_str(), service_obj->get_service_id().c_str());
        ASSERT_STREQ(ip.c_str(), service_obj->get_ip().c_str());
	ASSERT_EQ(port,service_obj->get_port());
}

TEST(NodeAdditionCliTest, explicit_constructor_serialization_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
        std::string ip = "192.168.131.64";
	uint64_t port= 61005;
        boost::shared_ptr<ServiceObject> service_obj(new ServiceObject());
        service_obj->set_service_id(service_id);
        service_obj->set_ip(ip);
	service_obj->set_port(port);
        std::list<boost::shared_ptr<ServiceObject> > node_list;
        node_list.push_back(service_obj);
        comm_messages::NodeAdditionCli nac1(node_list);
	std::string serialized;
        ASSERT_TRUE(nac1.serialize(serialized));
        uint64_t size = serialized.size();

	comm_messages::NodeAdditionCli nac2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(nac2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::NODE_ADDITION_CLI, nac2.get_type());
        ASSERT_STREQ(service_id.c_str(), service_obj->get_service_id().c_str());
        ASSERT_STREQ(ip.c_str(), service_obj->get_ip().c_str());
	ASSERT_EQ(port,service_obj->get_port());
}

}

	

