#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include <iostream>

namespace localLeaderStartMonitoringAck_message
{

TEST(localLeaderStartMonitoringAck_test, serialization_check)
{
	std::string service_id = "SN0101_61014";
	comm_messages::LocalLeaderStartMonitoringAck msg(service_id);

	std::string serialized;
	ASSERT_TRUE(msg.serialize(serialized));

	comm_messages::LocalLeaderStartMonitoringAck msg2;
	ASSERT_TRUE(msg2.deserialize(serialized.c_str(), serialized.size()));
}

}
