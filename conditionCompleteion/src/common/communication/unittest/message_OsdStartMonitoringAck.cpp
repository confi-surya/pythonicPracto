#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace OsdStartMonitoringAck_message_test
{

TEST(OsdStartMonitoringAckTest, explicit_constructor_check)
{
        std::string service_id = "HN0101_proxy-server_61005";
        int32_t code = 123;
        std::string msg = "OsdStartMonitoringAck msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
	comm_messages::OsdStartMonitoringAck osma(service_id,error_status);
	ASSERT_STREQ(service_id.c_str(), osma.get_service_id().c_str());
	ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

TEST(OsdStartMonitoringAckTest, explicit_constructor_serialization_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
	int32_t code = 123;
        std::string msg = "OsdStartMonitoringAck msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
        comm_messages::OsdStartMonitoringAck osma1(service_id,error_status);
        std::string serialized;
        ASSERT_TRUE(osma1.serialize(serialized));
        uint64_t size = serialized.size();

	comm_messages::OsdStartMonitoringAck osma2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(osma2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::OSD_START_MONITORING_ACK, osma2.get_type());
        ASSERT_EQ(code,error_status->get_code());
        ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
	ASSERT_STREQ(service_id.c_str(), osma2.get_service_id().c_str());

}

}


