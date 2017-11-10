#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "libraryUtils/object_storage_exception.h"
#include "osm/localLeader/monitor_status_map.h"

namespace MonitorStatusMapTest
{

TEST(MonitorStatusMapTest, MonitorStatusMapTest)
{
	uint64_t STRM_TIMEOUT = 120;
	uint64_t HRBT_TIMEOUT = 30;
	uint16_t gap = 300;
	uint8_t count = 3;
	int fd = 4;
	MonitorStatusMap status_map(STRM_TIMEOUT, HRBT_TIMEOUT, gap, count);	
	
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj;

	comm_obj = status_map.get_comm_obj();
	
	status_map.set_comm_obj(comm_obj);
	status_map.set_fd(fd);
	int val = status_map.get_fd();
	OSDLOG(DEBUG, "fd is = "<< val);
	ASSERT_EQ(val, 4);
	status_map.set_ip_addr("192.168.101.1");
	std::string s = status_map.get_ip_addr();
	OSDLOG(DEBUG, " IP ADDR IS  " << s);
	ASSERT_EQ(s, "192.168.101.1");
	status_map.set_port(61005);
	val = status_map.get_port();
	OSDLOG(DEBUG, "PORT is = "<< val);
	ASSERT_EQ(val, 61005);
	status_map.set_msg_type(OSDException::OSDMsgTypeInMap::STRM);
	ASSERT_EQ(status_map.get_msg_type(),  OSDException::OSDMsgTypeInMap::STRM);
	status_map.set_retry_count(count);
	val = status_map.get_retry_count();
	OSDLOG(DEBUG, "RETRY COUNT is = "<< val);
	ASSERT_EQ(val, 3);
//	OSDLOG(DEBUG, "STRM COUNT is = "<< status_map.get_strm_timeout());
//	OSDLOG(DEBUG, "HRBT COUNT is = "<< status_map.get_hrbt_timeout());
	uint64_t h = status_map.get_strm_timeout();
	ASSERT_EQ(h, STRM_TIMEOUT);
	h = status_map.get_hrbt_timeout();
	ASSERT_EQ(h, HRBT_TIMEOUT);
	status_map.update_last_update_time();
	time_t ts;
	ts = status_map.get_last_update_time();
	OSDLOG(DEBUG, "TIME is = "<< ts);
	status_map.set_service_status(OSDException::OSDServiceStatus::RUN);
	ASSERT_EQ(status_map.get_service_status(), 3);
	status_map.set_heartbeat_count(count);
	status_map.set_timeout(30);
	ts = status_map.get_timeout();
	OSDLOG(DEBUG, "TIMEOUT is = "<< ts);
	ASSERT_EQ(ts, 90);
	ASSERT_EQ(status_map.get_retry_time_gap(), 300);

}

}
