#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace local_leader_start_monitoring_test
{

TEST(LocalLeaderStartMonitoringTest, default_constructor_msg_type_check)
{
   comm_messages::LocalLeaderStartMonitoring llsm;
   ASSERT_EQ(messageTypeEnum::typeEnums::LL_START_MONITORING,llsm.get_type());
}

TEST(LocalLeaderStartMonitoringTest,explicit_constructor_check)
{
std::string service_id="HN0101_proxyserver";
comm_messages::LocalLeaderStartMonitoring llsm(service_id);
ASSERT_EQ(service_id.c_str(),llsm.get_service_id().c_str());

}

TEST(LocalLeaderStartMonitoringTest, default_constructor_fd_check)
{
comm_messages::LocalLeaderStartMonitoring llsm;
uint64_t fd=2;
llsm.set_fd(fd);
ASSERT_EQ(fd,llsm.get_fd());
}

TEST(LocalLeaderStartMonitoringTest, explicit_constructor_serialization_check)
{
  std::string service_id="HN0101_proxyserver";
  comm_messages::LocalLeaderStartMonitoring llsm(service_id);
  std::string serialized;
  ASSERT_TRUE(llsm.serialize(serialized));
  uint64_t size= serialized.size();

  comm_messages::LocalLeaderStartMonitoring llsm2;
  char * msg_string=new char[size+1];
  memset(msg_string,'\0',size+1);
  strncpy(msg_string,serialized.c_str(),size);

  ASSERT_TRUE(llsm2.deserialize(msg_string,size));
  ASSERT_EQ(messageTypeEnum::typeEnums::LL_START_MONITORING,llsm2.get_type());
  ASSERT_STREQ(service_id.c_str(),llsm2.get_service_id().c_str());

}
}

