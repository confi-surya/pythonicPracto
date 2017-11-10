#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace Take_Gl_Ownership_Test 
{

TEST(TakeGlOwnershipTest, default_constructor_msg_type_check)
{
    comm_messages::TakeGlOwnership tgl;
    ASSERT_EQ(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP,tgl.get_type());
}

TEST(TakeGlOwnershipTest,explicit_constructor_check)
{
    std::string old_gl_id="OLD_GL_ID:123"; 
    std::string new_gl_id="NEW_GL_ID:123";
    comm_messages::TakeGlOwnership tgl(old_gl_id,new_gl_id);
    ASSERT_STREQ(old_gl_id.c_str(),tgl.get_old_gl_id().c_str());
	ASSERT_STREQ(new_gl_id.c_str(),tgl.get_new_gl_id().c_str());
    ASSERT_EQ(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP,tgl.get_type());
}

TEST(TakeGlOwnershipTest,default_constructor_fd_check)
{
    comm_messages::TakeGlOwnership tgl;
    uint64_t fd=2;
    tgl.set_fd(fd);
    ASSERT_EQ(fd,tgl.get_fd());
}


TEST(TakeGlOwnershipTest, explicit_constructor_serialization_check)
{

    std::string old_gl_id="OLD_GL_ID:123";
    std::string new_gl_id="NEW_GL_ID:123";

    comm_messages::TakeGlOwnership tgl(old_gl_id,new_gl_id);
    std::string serialized;
    ASSERT_TRUE(tgl.serialize(serialized));
    uint64_t size= serialized.size();

    comm_messages::TakeGlOwnership tgl2;
    char * msg_string=new char[size+1];
    memset(msg_string,'\0',size+1);
    strncpy(msg_string,serialized.c_str(),size);

    ASSERT_TRUE(tgl2.deserialize(msg_string,size));
    ASSERT_EQ(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP,tgl2.get_type());
    ASSERT_STREQ(old_gl_id.c_str(),tgl2.get_old_gl_id().c_str());
    ASSERT_STREQ(new_gl_id.c_str(),tgl2.get_new_gl_id().c_str());


}
}

