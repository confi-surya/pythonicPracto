#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace Get_Version_ID_Ack_Test
{

TEST(GetVersionIDAckTest, default_constructor_msg_type_check)
{
	comm_messages::GetVersionIDAck gvack;
    ASSERT_EQ(messageTypeEnum::typeEnums::GET_OBJECT_VERSION_ACK,gvack.get_type());
}

TEST(GetVersionIDAckTest,explicit_constructor_check)
{
	std::string service_id="HN0101_proxyserver";
	uint64_t version=123;
	bool status=true;
	comm_messages::GetVersionIDAck gvack(service_id,version,status);
	ASSERT_EQ(service_id.c_str(),gvack.get_service_id().c_str());
	ASSERT_EQ(messageTypeEnum::typeEnums::GET_OBJECT_VERSION_ACK,gvack.get_type());
}


TEST(GetVersionIDAckTest, explicit_constructor_serialization_check)
{
  	std::string service_id="HN0101_proxyserver";
  	uint64_t version=123;
  	bool status=true;
  	comm_messages::GetVersionIDAck gvack(service_id,version,status);
  	std::string serialized;
  	ASSERT_TRUE(gvack.serialize(serialized));
  	uint64_t size= serialized.size();

  	comm_messages::GetVersionIDAck gvack2;
 	char * msg_string=new char[size+1];
  	memset(msg_string,'\0',size+1);
  	strncpy(msg_string,serialized.c_str(),size);

  	ASSERT_TRUE(gvack2.deserialize(msg_string,size));
  	ASSERT_EQ(messageTypeEnum::typeEnums::GET_OBJECT_VERSION_ACK,gvack2.get_type());
  	ASSERT_STREQ(service_id.c_str(),gvack2.get_service_id().c_str());
    ASSERT_EQ(version,gvack2.get_version_id());
    ASSERT_EQ(status,gvack2.get_status());
}
}

