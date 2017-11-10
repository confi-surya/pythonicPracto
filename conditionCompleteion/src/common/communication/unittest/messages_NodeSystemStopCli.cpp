#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace NodeSystemStopCli_message_test
{
TEST(NodeSystemStopCliTest, explicit_constructor_check)
{
	std::string node_id="HN0101";
	comm_messages::NodeSystemStopCli nssc(node_id);
        ASSERT_STREQ(node_id.c_str(), nssc.get_node_id().c_str());
	ASSERT_EQ(messageTypeEnum::typeEnums::NODE_SYSTEM_STOP_CLI,nssc.get_type());
}

TEST(NodeSystemStopCliTest, explicit_constructor_serialization_check)
{
    std::string node_id="HN0101";
    comm_messages::NodeSystemStopCli nssc1(node_id);
    std::string serialized;
    ASSERT_TRUE(nssc1.serialize(serialized));
    uint64_t size= serialized.size();

    comm_messages::NodeSystemStopCli nssc2;
    char * msg_string=new char[size+1];
    memset(msg_string,'\0',size+1);
    strncpy(msg_string,serialized.c_str(),size);

    ASSERT_TRUE(nssc2.deserialize(msg_string,size));
    ASSERT_EQ(messageTypeEnum::typeEnums::NODE_SYSTEM_STOP_CLI,nssc2.get_type());
    ASSERT_STREQ(node_id.c_str(),nssc2.get_node_id().c_str());
}

}


