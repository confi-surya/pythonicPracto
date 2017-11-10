#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace nodefailover_message_test
{

TEST(Nodefailover, explicit_constructor_serialization_check)
{
	std::string service_id = "HN0101";
	boost::shared_ptr<ErrorStatus> err(new ErrorStatus(1, "abc"));
	comm_messages::NodeFailover hb(service_id, err);
	std::string serialized;
	ASSERT_TRUE(hb.serialize(serialized));
	uint64_t size = serialized.size();

	comm_messages::NodeFailover hb2;
	char * msg_string = new char[size + 1];
	memset(msg_string, '\0', size + 1);
	strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(hb2.deserialize(msg_string, size));
}

}
