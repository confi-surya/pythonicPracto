#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"


namespace Node_Stop_Cli_Test
{

TEST(NodeStopCliTest, default_constructor_msg_type_check)
{
    comm_messages::NodeStopCli ncli;
    ASSERT_EQ(messageTypeEnum::typeEnums::NODE_STOP_CLI, ncli.get_type());

}

TEST(NodeStopCliTest,explicit_constructor_check)
{
    std::string node_id="NODE_ID123";
    comm_messages::NodeStopCli ncli(node_id);
    ASSERT_EQ(messageTypeEnum::typeEnums::NODE_STOP_CLI,ncli.get_type());
    ASSERT_STREQ(node_id.c_str(),ncli.get_node_id().c_str());
}


TEST(NodeStopCliTest, default_constructor_fd_check)
{
    comm_messages::NodeStopCli ncli;
    uint64_t fd=2;
    ncli.set_fd(fd);
    ASSERT_EQ(fd,ncli.get_fd());
}


TEST(NodeStopCliTest, explicit_constructor_serialization_check)
{
    std::string node_id="NODE_ID123";
    comm_messages::NodeStopCli ncli(node_id);
    std::string serialized;
    ASSERT_TRUE(ncli.serialize(serialized));
    uint64_t size= serialized.size();

    comm_messages::NodeStopCli ncli2;
    char * msg_string=new char[size+1];
    memset(msg_string,'\0',size+1);
    strncpy(msg_string,serialized.c_str(),size);

    ASSERT_TRUE(ncli2.deserialize(msg_string,size));
    ASSERT_EQ(messageTypeEnum::typeEnums::NODE_STOP_CLI,ncli2.get_type());
    ASSERT_STREQ(node_id.c_str(),ncli2.get_node_id().c_str());
}
}

