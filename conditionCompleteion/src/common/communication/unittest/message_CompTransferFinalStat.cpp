#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include <vector>
#include <utility>

namespace CompTransferFinalStat_message_test
{
TEST(CompTransferFinalStatTest, explicit_constructor_check)
{
        std::string service_id = "HN0101_proxy-server_61005";
	std::pair<int32_t, bool> p(1,true);
        std::vector<std::pair<int32_t, bool> > comp_pair_list;
	comp_pair_list.push_back(p);
        comm_messages::CompTransferFinalStat ctfs(service_id,comp_pair_list);
        ASSERT_EQ(comp_pair_list,ctfs.get_component_pair_list());
        ASSERT_STREQ(service_id.c_str(), ctfs.get_service_id().c_str());
}

TEST(CompTransferFinalStatTest, explicit_constructor_serialization_check)
{
        std::string service_id = "HN0101_proxy-server_61005";
	std::pair<int32_t, bool> p(1,true);
        std::vector<std::pair<int32_t, bool> > comp_pair_list;
	comp_pair_list.push_back(p);
        comm_messages::CompTransferFinalStat ctfs1(service_id,comp_pair_list);
        std::string serialized;
        ASSERT_TRUE(ctfs1.serialize(serialized));
        uint64_t size = serialized.size();

        comm_messages::CompTransferFinalStat ctfs2;
        char * msg_string = new char[size + 1];
        memset(msg_string, '\0', size + 1);
        strncpy(msg_string, serialized.c_str(), size);

        ASSERT_TRUE(ctfs2.deserialize(msg_string, size));
        ASSERT_EQ(messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT, ctfs2.get_type());
        ASSERT_EQ(comp_pair_list,ctfs2.get_component_pair_list());
        ASSERT_STREQ(service_id.c_str(), ctfs2.get_service_id().c_str());
}

}

