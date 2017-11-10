#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace NodeRetireAck_message_test
{
TEST(NodeRetireAckTest, explicit_constructor_check)
{
	std::string node_id="HN0101";
	int32_t code = 123;
        std::string msg = "NodeRetireAck msg";
	boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
	comm_messages::NodeRetireAck nra(node_id,error_status);
	ASSERT_STREQ(node_id.c_str(),nra.get_node_id().c_str());
	ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

TEST(NodeRetireAckTest, explicit_constructor_serialization_check)
{
	std::string node_id="HN0101";
        int32_t code = 123;
        std::string msg = "NodeRetireAck msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
        comm_messages::NodeRetireAck nra1(node_id,error_status);
	std::string serialized;
        ASSERT_TRUE(nra1.serialize(serialized));
        uint64_t size = serialized.size();

	comm_messages::NodeRetireAck nra2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(nra2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::NODE_RETIRE_ACK, nra2.get_type());
	ASSERT_STREQ(node_id.c_str(),nra2.get_node_id().c_str());
        ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

}



