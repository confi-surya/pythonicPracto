#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"


namespace local_node_status_test
{

TEST(LocalNodeStatusTest, default_constructor_msg_type_check)
{
	comm_messages::LocalNodeStatus ln;
	ASSERT_EQ(messageTypeEnum::typeEnums::LOCAL_NODE_STATUS, ln.get_type());

}

TEST(LocalNodeStatusTest,explicit_constructor_check)
{
    std::string node_id="NODE_ID123";
    comm_messages::LocalNodeStatus ln(node_id);
    ASSERT_EQ(messageTypeEnum::typeEnums::LOCAL_NODE_STATUS,ln.get_type());
    ASSERT_STREQ(node_id.c_str(),ln.get_node_id().c_str());
}


TEST(LocalNodeStatusTest, default_constructor_fd_check)
{
    comm_messages::LocalNodeStatus ln;
    uint64_t fd=2;
    ln.set_fd(fd);
    ASSERT_EQ(fd,ln.get_fd());
}


TEST(LocalNodeStatusTest, explicit_constructor_serialization_check)
{
    std::string node_id="NODE_ID123";
    comm_messages::LocalNodeStatus ln(node_id);
    std::string serialized;
    ASSERT_TRUE(ln.serialize(serialized));
    uint64_t size= serialized.size();

    comm_messages::LocalNodeStatus ln2;
    char * msg_string=new char[size+1];
    memset(msg_string,'\0',size+1);
    strncpy(msg_string,serialized.c_str(),size);

    ASSERT_TRUE(ln2.deserialize(msg_string,size)); 
    ASSERT_EQ(messageTypeEnum::typeEnums::LOCAL_NODE_STATUS,ln2.get_type());
    ASSERT_STREQ(node_id.c_str(),ln2.get_node_id().c_str());
}
}


