#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace StopServicesAck_message_test
{
TEST(StopServicesAckTest, explicit_constructor_check)
{
	int32_t code = 123;
        std::string msg = "StopServicesAck msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
	comm_messages::StopServicesAck ssa(error_status);
        ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

TEST(StopServicesAckTest, explicit_constructor_serialization_check)
{
	int32_t code = 123;
        std::string msg = "StopServicesAck msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
        comm_messages::StopServicesAck ssa1(error_status);
	std::string serialized;
        ASSERT_TRUE(ssa1.serialize(serialized));
        uint64_t size = serialized.size();

        comm_messages::StopServicesAck ssa2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

        ASSERT_TRUE(ssa2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::STOP_SERVICES_ACK, ssa2.get_type());
        ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

}



