#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "osm/osmServer/election_manager.h"
#include "osm/osmServer/file_handler.h"
#include "libraryUtils/record_structure.h"

namespace LeaderElectionTest
{
TEST(LeaderElectionTest, LeaderElectionTest)
{
	ElectionManager mgr("HN0103_123", "/remote/hydra042/dkushwaha/tmp/election");
	mgr.start_election("V4.0", 1);
}

}



