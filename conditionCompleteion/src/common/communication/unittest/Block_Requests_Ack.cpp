#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace get_Version_Id_Test
{

TEST(BlockRequestsAckTest, default_constructor_msg_type_check)
{
    comm_messages::BlockRequestsAck br;
    ASSERT_EQ(messageTypeEnum::typeEnums::BLOCK_NEW_REQUESTS_ACK,br.get_type());
}

TEST(BlockRequestsAckTest,explicit_constructor_check)
{
    bool status=true;
    comm_messages::BlockRequestsAck br(status);
    ASSERT_EQ(status,br.get_status());
    ASSERT_EQ(messageTypeEnum::typeEnums::BLOCK_NEW_REQUESTS_ACK,br.get_type());
}


TEST(BlockRequestsAckTest, explicit_constructor_serialization_check)
{

    bool status=true;
    comm_messages::BlockRequestsAck br(status);
    std::string serialized;
    ASSERT_TRUE(br.serialize(serialized));
    uint64_t size= serialized.size();

    comm_messages::BlockRequestsAck br2;
    char * msg_string=new char[size+1];
    memset(msg_string,'\0',size+1);
    strncpy(msg_string,serialized.c_str(),size);

    ASSERT_TRUE(br2.deserialize(msg_string,size));
    ASSERT_EQ(messageTypeEnum::typeEnums::BLOCK_NEW_REQUESTS_ACK,br2.get_type());
    ASSERT_EQ(status,br2.get_status());


}
}

