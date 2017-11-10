#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace getglobalmap_message_test
{

TEST(GetGlobalMapTest, default_constructor_msg_type_check)
{
	comm_messages::GetGlobalMap msg;
	ASSERT_EQ(messageTypeEnum::typeEnums::GET_GLOBAL_MAP, msg.get_type());
}

TEST(GetGlobalMapTest, explicit_constructor_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
	comm_messages::GetGlobalMap msg(service_id);
	ASSERT_STREQ(service_id.c_str(), msg.get_service_id().c_str());
}

TEST(GetGlobalMapTest, default_constructor_fd_check)
{
	comm_messages::GetGlobalMap msg;
	uint64_t fd = 2;
	msg.set_fd(fd);
	ASSERT_EQ(fd, msg.get_fd());
}

TEST(GetGlobalMapTest, explicit_constructor_serialization_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
	comm_messages::GetGlobalMap msg(service_id);
	std::string serialized;
	ASSERT_TRUE(msg.serialize(serialized));
	uint64_t size = serialized.size();

	comm_messages::GetGlobalMap msg2;
	char * msg_string = new char[size + 1];
	memset(msg_string, '\0', size + 1);
	strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(msg2.deserialize(msg_string, size));
	ASSERT_EQ(messageTypeEnum::typeEnums::GET_GLOBAL_MAP, msg2.get_type());
	ASSERT_STREQ(service_id.c_str(), msg2.get_service_id().c_str());
}

}
