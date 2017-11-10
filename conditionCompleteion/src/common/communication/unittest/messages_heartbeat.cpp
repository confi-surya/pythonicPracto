#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace heartbeat_message_test
{

TEST(HeartBeatTest, default_constructor_msg_string_check)
{
	comm_messages::HeartBeat hb;
	std::string heartbeat_string = "HEARTBEAT";
	std::string msg = hb.get_msg();
	ASSERT_STREQ(heartbeat_string.c_str(), msg.c_str());
}

TEST(HeartBeatTest, default_constructor_msg_type_check)
{
	comm_messages::HeartBeat hb;
	ASSERT_EQ(messageTypeEnum::typeEnums::HEART_BEAT, hb.get_type());
}

TEST(HeartBeatTest, explicit_constructor_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
	uint32_t sequence = 124;
	std::string heartbeat_string = "HEARTBEAT";
	comm_messages::HeartBeat hb(service_id, sequence);
	ASSERT_STREQ(heartbeat_string.c_str(), hb.get_msg().c_str());
	ASSERT_STREQ(service_id.c_str(), hb.get_service_id().c_str());
	ASSERT_EQ(sequence, hb.get_sequence());
}

TEST(HeartBeatTest, default_constructor_fd_check)
{
	comm_messages::HeartBeat hb;
	uint64_t fd = 2;
	hb.set_fd(fd);
	ASSERT_EQ(fd, hb.get_fd());
}

TEST(HeartBeatTest, explicit_constructor_serialization_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
	uint32_t sequence = 124;
	std::string heartbeat_string = "HEARTBEAT";
	comm_messages::HeartBeat hb(service_id, sequence);
	std::string serialized;
	ASSERT_TRUE(hb.serialize(serialized));
	uint64_t size = serialized.size();

	for (int i = 0 ; i < size; i++){
	//	std::cout << "|" << int(serialized[i]) << ", " << serialized[i];
	}
	//std::cout << "\n Finished\n";

	comm_messages::HeartBeat hb2;
	char * msg_string = new char[size + 1];
	memset(msg_string, '\0', size + 1);
	strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(hb2.deserialize(msg_string, size));
	ASSERT_EQ(messageTypeEnum::typeEnums::HEART_BEAT, hb2.get_type());
	ASSERT_EQ(sequence, hb2.get_sequence());
	ASSERT_STREQ(heartbeat_string.c_str(), hb2.get_msg().c_str());
	ASSERT_STREQ(service_id.c_str(), hb2.get_service_id().c_str());
}

}
