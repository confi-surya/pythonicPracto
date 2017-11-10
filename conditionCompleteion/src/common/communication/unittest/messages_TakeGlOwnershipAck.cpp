#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace Take_Gl_Ownership_Ack_Test 
{

TEST(TakeGlOwnershipAckTest, default_constructor_msg_type_check)
{
    comm_messages::TakeGlOwnershipAck tgl;
    ASSERT_EQ(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP_ACK,tgl.get_type());
}

TEST(TakeGlOwnershipAckTest,explicit_constructor_check)
{
    std::string new_gl_id="NEW_GL_ID:123";
    boost::shared_ptr<ErrorStatus> status(new ErrorStatus(0, "Took GL ownership"));
    comm_messages::TakeGlOwnershipAck tgl_ack(new_gl_id, status);
    ASSERT_STREQ(new_gl_id.c_str(),tgl_ack.get_new_gl_id().c_str());
    ASSERT_EQ(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP_ACK,tgl_ack.get_type());
}

TEST(TakeGlOwnershipAckTest,default_constructor_fd_check)
{
    comm_messages::TakeGlOwnershipAck tgl;
    uint64_t fd=2;
    tgl.set_fd(fd);
    ASSERT_EQ(fd,tgl.get_fd());
}


TEST(TakeGlOwnershipAckTest, explicit_constructor_serialization_check)
{

    std::string new_gl_id="NEW_GL_ID:123";
    boost::shared_ptr<ErrorStatus> status(new ErrorStatus(0, "Took GL ownership"));
    comm_messages::TakeGlOwnershipAck tgl(new_gl_id, status);
    std::string serialized;
    ASSERT_TRUE(tgl.serialize(serialized));
    uint64_t size1= serialized.size();

    comm_messages::TakeGlOwnershipAck tgl2;
    char * msg_string=new char[size1+1];
    memset(msg_string,'\0',size1+1);
    memcpy(msg_string,serialized.c_str(),size1);

    ASSERT_TRUE(tgl2.deserialize(msg_string,size1));
    ASSERT_EQ(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP_ACK,tgl2.get_type());
    ASSERT_STREQ(new_gl_id.c_str(),tgl2.get_new_gl_id().c_str());

}
}

