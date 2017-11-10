#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include <vector>
#include <utility>

namespace TransferComponentsAck_message_test
{
TEST(TransferComponentsAckTest, explicit_constructor_check)
{
        std::pair<int32_t, bool> p(1,true);
        std::vector<std::pair<int32_t, bool> > comp_pair_list;
        comp_pair_list.push_back(p);
        comm_messages::TransferComponentsAck tca(comp_pair_list);
        ASSERT_EQ(comp_pair_list,tca.get_comp_list());
}

TEST(TransferComponentsAckTest, explicit_constructor_serialization_check)
{
        std::pair<int32_t, bool> p(1,true);
        std::vector<std::pair<int32_t, bool> > comp_pair_list;
        comp_pair_list.push_back(p);
        comm_messages::TransferComponentsAck tca1(comp_pair_list);
        std::string serialized;
        ASSERT_TRUE(tca1.serialize(serialized));
        uint64_t size = serialized.size();

        comm_messages::TransferComponentsAck tca2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

        ASSERT_TRUE(tca2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::TRANSFER_COMPONENT_RESPONSE, tca2.get_type());
        ASSERT_EQ(comp_pair_list,tca2.get_comp_list());
}

}



