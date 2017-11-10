#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "osm/osmServer/service_config_reader.h"

namespace ServiceConfigReaderTest
{

TEST(ServiceConfigReaderTest, ServiceConfigReaderTest)
{

	ServiceConfigReader config_reader;
	ASSERT_EQ(config_reader.get_strm_timeout("container"), uint64_t(120));
	ASSERT_EQ(config_reader.get_strm_timeout("account"), uint64_t(120));
	ASSERT_EQ(config_reader.get_strm_timeout("proxy"), uint64_t(120));
	ASSERT_EQ(config_reader.get_strm_timeout("object"), uint64_t(120));
	ASSERT_EQ(config_reader.get_strm_timeout("account-updater"), uint64_t(120));
	ASSERT_EQ(config_reader.get_retry_count(), uint16_t(1));	
	ASSERT_EQ(config_reader.get_heartbeat_timeout(), uint64_t(5));
	ASSERT_EQ(config_reader.get_service_start_time_gap(), uint16_t(300));	

}
} //namespace ServiceConfigReaderTest

