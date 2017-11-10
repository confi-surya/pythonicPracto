#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include <vector>

namespace GetServiceComponentAck_message_test
{
TEST(GetServiceComponentAckTest, explicit_constructor_check)
{
	uint32_t a[]={1,2,3};
	std::vector<uint32_t> component_list(a, a+3);
	int32_t code = 123;
	std::string msg = "Ack msg";
	boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
	comm_messages::GetServiceComponentAck gsca(component_list,error_status);
	ASSERT_EQ(component_list,gsca.get_component_list());
        ASSERT_EQ(code,error_status->get_code());
	ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}
 	
TEST(GetServiceComponentAckTest, explicit_constructor_serialization_check)
{	
	uint32_t a[]={1,2,3};
	std::vector<uint32_t> component_list(a,a+3);
        int32_t code = 123;
        std::string msg = "Ack msg";
        boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(code,msg));
        comm_messages::GetServiceComponentAck gsca1(component_list,error_status);
	std::string serialized;
        ASSERT_TRUE(gsca1.serialize(serialized));
        uint64_t size = serialized.size();

	comm_messages::GetServiceComponentAck gsca2;
	char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

	ASSERT_TRUE(gsca2.deserialize(msg_string, size));
	ASSERT_EQ(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT_ACK, gsca2.get_type());
	ASSERT_EQ(component_list,gsca2.get_component_list());
	ASSERT_EQ(code,error_status->get_code());
	ASSERT_STREQ(msg.c_str(), error_status->get_msg().c_str());
}

}





