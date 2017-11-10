#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace get_Version_Id_Test
{

TEST(GetVersionIdTest, default_constructor_msg_type_check)
{
	comm_messages::GetVersionID gv;
	ASSERT_EQ(messageTypeEnum::typeEnums::GET_OBJECT_VERSION,gv.get_type());
}

TEST(GetVersionIdTest,explicit_constructor_check)
{
	std::string service_id="HN0101_proxyserver";
	comm_messages::GetVersionID gv(service_id);
	ASSERT_EQ(service_id.c_str(),gv.get_service_id().c_str());
	ASSERT_EQ(messageTypeEnum::typeEnums::GET_OBJECT_VERSION,gv.get_type());
}


TEST(GetVersionIdTest, explicit_constructor_serialization_check)
{

	std::string service_id="HN0101_proxyserver";
	comm_messages::GetVersionID gv(service_id);
	std::string serialized;
	ASSERT_TRUE(gv.serialize(serialized));
	uint64_t size= serialized.size();

  	comm_messages::GetVersionID gv2;
  	char * msg_string=new char[size+1];
  	memset(msg_string,'\0',size+1);
  	strncpy(msg_string,serialized.c_str(),size);

  	ASSERT_TRUE(gv2.deserialize(msg_string,size));
  	ASSERT_EQ(messageTypeEnum::typeEnums::GET_OBJECT_VERSION,gv2.get_type());
  	ASSERT_STREQ(service_id.c_str(),gv2.get_service_id().c_str());


}
}

