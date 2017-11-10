#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace NodeStopCliAck_message_test
{
TEST(NodeStopCliAckTest, explicit_constructor_check)
{
	int32_t code = 123;
        std::string msg = "NodeStopCliAck msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
        comm_messages::NodeStopCliAck nsca(error_status);
        ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

TEST(NodeStopCliAckTest, explicit_constructor_serialization_check)
{
        int32_t code = 123;
        std::string msg = "NodeStopCliAck msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
        comm_messages::NodeStopCliAck nsca1(error_status);
        std::string serialized;
        ASSERT_TRUE(nsca1.serialize(serialized));
        uint64_t size = serialized.size();

        comm_messages::NodeStopCliAck nsca2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

        ASSERT_TRUE(nsca2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::NODE_STOP_CLI_ACK, nsca2.get_type());
        ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

}


