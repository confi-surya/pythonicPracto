#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace node_addition_Gl_Test
{

TEST(NodeAdditionGlTest, default_constructor_msg_type_check)
{
  	 comm_messages::NodeAdditionGl na;
  	 ASSERT_EQ(messageTypeEnum::typeEnums::NODE_ADDITION_GL,na.get_type());
}

TEST(NodeAdditionGlTest,explicit_constructor_check)
{
	std::string service_id="HN0101_proxyserver";
	comm_messages::NodeAdditionGl na(service_id);
	ASSERT_EQ(messageTypeEnum::typeEnums::NODE_ADDITION_GL,na.get_type());
	ASSERT_STREQ(service_id.c_str(),na.get_service_id().c_str());
}

TEST(NodeAdditionGlTest, default_constructor_fd_check)
{
	comm_messages::NodeAdditionGl na;
	uint64_t fd=2;
	na.set_fd(fd);
	ASSERT_EQ(fd,na.get_fd());
}

TEST(NodeAdditionGlTest, explicit_constructor_serialization_check)
{
	std::string service_id="HN0101_proxyserver";
	comm_messages::NodeAdditionGl na(service_id);
	std::string serialized;
	ASSERT_TRUE(na.serialize(serialized));
	uint64_t size= serialized.size();

	comm_messages::NodeAdditionGl na2;
	char * msg_string=new char[size+1];
	memset(msg_string,'\0',size+1);
	strncpy(msg_string,serialized.c_str(),size);

	ASSERT_TRUE(na2.deserialize(msg_string,size));
	ASSERT_EQ(messageTypeEnum::typeEnums::NODE_ADDITION_GL,na2.get_type());
	ASSERT_STREQ(service_id.c_str(),na2.get_service_id().c_str());
}
}


