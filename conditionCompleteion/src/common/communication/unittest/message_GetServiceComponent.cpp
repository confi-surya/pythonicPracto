#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace GetServiceComponent_message_test
{

TEST(GetServiceComponentTest , explicit_constructor_check)
{
	std::string service_id = "HN0101_proxy-server_61005";
	comm_messages::GetServiceComponent gsc(service_id);
	ASSERT_STREQ(service_id.c_str(), gsc.get_service_id().c_str());
}

TEST(GetServiceComponentTest , explicit_constructor_serialization_check)
{
	 std::string service_id = "HN0101_proxy-server_61005";
	 comm_messages::GetServiceComponent gsc(service_id);
	 std::string serialized;
	 ASSERT_TRUE(gsc.serialize(serialized));
	 uint64_t size = serialized.size();

	 comm_messages::GetServiceComponent gsc2;
	 char * msg_string = new char[size + 1];
         memset(msg_string, '\0', size + 1);
         strncpy(msg_string, serialized.c_str(), size);

	 ASSERT_TRUE(gsc2.deserialize(msg_string, size));
	 ASSERT_EQ(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT, gsc2.get_type());
	 ASSERT_STREQ(service_id.c_str(), gsc2.get_service_id().c_str());
}

}



