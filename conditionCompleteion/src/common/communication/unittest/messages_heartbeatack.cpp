#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace heartbeatack_message_test
{

TEST(HeartBeatAckTest, default_constructor_msg_type_check)
{
	comm_messages::HeartBeatAck hb;
	ASSERT_EQ(messageTypeEnum::typeEnums::HEART_BEAT_ACK, hb.get_type());
}

TEST(HeartBeatAckTest, explicit_constructor_check)
{
	uint32_t sequence = 124;
	std::string heartbeat_string = "HEARTBEAT";
	comm_messages::HeartBeatAck hb(sequence);
	ASSERT_STREQ(heartbeat_string.c_str(), hb.get_msg().c_str());
	ASSERT_EQ(sequence, hb.get_sequence());
}

TEST(HeartBeatAckTest, default_constructor_fd_check)
{
	comm_messages::HeartBeatAck hb;
	uint64_t fd = 2;
	hb.set_fd(fd);
	ASSERT_EQ(fd, hb.get_fd());
}

TEST(HeartBeatAckTest, explicit_constructor_serialization_check)
{
	uint32_t sequence = 124;
	std::string heartbeat_string = "HEARTBEAT";
	comm_messages::HeartBeatAck hb(sequence);
	std::string serialized;
	ASSERT_TRUE(hb.serialize(serialized));
	uint64_t size = serialized.size();

	comm_messages::HeartBeatAck hb2;
	char * msg_string = new char[size + 1];
	memset(msg_string, '\0', size + 1);
	strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(hb2.deserialize(msg_string, size));
	ASSERT_EQ(messageTypeEnum::typeEnums::HEART_BEAT_ACK, hb2.get_type());
	ASSERT_EQ(sequence, hb2.get_sequence());
	ASSERT_STREQ(heartbeat_string.c_str(), hb2.get_msg().c_str());
}

}
