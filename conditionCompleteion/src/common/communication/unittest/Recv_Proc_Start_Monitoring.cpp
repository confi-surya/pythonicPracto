#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace Recv_Proc_start_monitoring_test
{

TEST(RecvProcStartMonitoringTest, default_constructor_msg_type_check)
{

   comm_messages::RecvProcStartMonitoring rcv;
   ASSERT_EQ(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING,rcv.get_type());

}

TEST(RecvProcStartMonitoringTest,explicit_constructor_check)
{

std::string proc_id="ProcId";
comm_messages::RecvProcStartMonitoring rcv(proc_id);
ASSERT_STREQ(proc_id.c_str(),rcv.get_proc_id().c_str());
ASSERT_EQ(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING,rcv.get_type());


}

TEST(RecvProcStartMonitoringTest, default_constructor_fd_check)
{
comm_messages::RecvProcStartMonitoring rcv;
uint64_t fd=2;
rcv.set_fd(fd);
ASSERT_EQ(fd,rcv.get_fd());
}

TEST(RecvProcStartMonitoringTest, explicit_constructor_serialization_check)
{
  std::string proc_id="ProcId";
  comm_messages::RecvProcStartMonitoring rcv(proc_id);
  std::string serialized;
  ASSERT_TRUE(rcv.serialize(serialized));
  uint64_t size= serialized.size();

  comm_messages::RecvProcStartMonitoring rcvp;
  char * msg_string=new char[size+1];
  memset(msg_string,'\0',size+1);
  strncpy(msg_string,serialized.c_str(),size);

  ASSERT_TRUE(rcvp.deserialize(msg_string,size));
  ASSERT_EQ(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING,rcvp.get_type());
  ASSERT_STREQ(proc_id.c_str(),rcvp.get_proc_id().c_str());

}
}




