#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"

namespace osd_monitoring_test

{

TEST(OsdStartMonitoringTest, default_constructor_msg_type_check)
{
   comm_messages::OsdStartMonitoring osdm;
   ASSERT_EQ(messageTypeEnum::typeEnums::OSD_START_MONITORING,osdm.get_type());
}

TEST(OsdStartMonitoringTest,explicit_constructor_check)
{
	std::string service_id="HN0101_proxyserver";
	uint32_t port=1;
	std::string ip="10.0.0.80";
comm_messages::OsdStartMonitoring osdm(service_id,port,ip);
ASSERT_STREQ(service_id.c_str(),osdm.get_service_id().c_str());
ASSERT_EQ(port,osdm.get_port());
ASSERT_STREQ(ip.c_str(),osdm.get_ip().c_str());
}
TEST(OsdStartMonitoringTest, default_constructor_fd_check)
{

 comm_messages::OsdStartMonitoring osdm;
 uint64_t fd=2;
 osdm.set_fd(fd);
ASSERT_EQ(fd,osdm.get_fd());
}

TEST(OsdStartMonitoringTest, explicit_constructor_serialization_check)
{
  std::string service_id="HN0101_proxyserver";
  uint32_t port=124;
  std::string ip="10.0.0.80";
  comm_messages::OsdStartMonitoring osdm(service_id,port,ip);
  std::string serialized;
  ASSERT_TRUE(osdm.serialize(serialized));
  uint64_t size= serialized.size();

  comm_messages::OsdStartMonitoring osdm2;
  char * msg_string=new char[size+1];
  memset(msg_string,'\0',size+1);
  strncpy(msg_string,serialized.c_str(),size);

  ASSERT_TRUE(osdm2.deserialize(msg_string,size));
  ASSERT_EQ(messageTypeEnum::typeEnums::OSD_START_MONITORING,osdm2.get_type());
  ASSERT_STREQ(service_id.c_str(),osdm2.get_service_id().c_str());
  ASSERT_EQ(port,osdm2.get_port());
  ASSERT_STREQ(ip.c_str(),osdm2.get_ip().c_str());



}


}























 
   
  
