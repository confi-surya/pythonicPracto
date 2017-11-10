#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "osm/osmServer/service_config_reader.h"

namespace ServiceConfigReaderTest
{

TEST(ServiceConfigReaderTest, ServiceConfigReaderTest)
{

	unsigned int var = 120;
	ServiceConfigReader config_reader;
	ASSERT_EQ(config_reader.get_strm_timeout("container"), var);
	ASSERT_EQ(config_reader.get_strm_timeout("account"), var);
	ASSERT_EQ(config_reader.get_strm_timeout("proxy"), var);
	ASSERT_EQ(config_reader.get_strm_timeout("object"), var);
	ASSERT_EQ(config_reader.get_strm_timeout("accountUpdater"), var);
	var=1;
	ASSERT_EQ(config_reader.get_retry_count(), var);
	var=5;
	ASSERT_EQ(config_reader.get_heartbeat_timeout(), var);
	var=300;
	ASSERT_EQ(config_reader.get_service_start_time_gap(), var);	

}
} //namespace ServiceConfigReaderTest

