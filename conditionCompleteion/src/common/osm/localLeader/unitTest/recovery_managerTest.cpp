#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "osm/localLeader/recovery_handler.h"

namespace RecoveryHandlerTest
{

	TEST(RecoveryHandlerTest, RecoveryHandlerTest)
	{
	
		CommandExecuter cmdExe;
		std::string cmd = "";
		OSDLOG(DEBUG, "EXECUTED COMMAND IS  ="<< cmd);
		cmd = cmdExe.get_command("ls", "");
		OSDLOG(DEBUG, "EXECUTED COMMAND IS  ="<< cmd);
		cmd = std::string("ls");

		int status;
		FILE *fp;
		cmdExe.exec_command(cmd.c_str(), &fp, &status);
		OSDLOG(DEBUG, "STATUS of EXECUTED COMMAND  = "<< cmd << " is: " << (status));
		ASSERT_NE(status, -1);


		//RecoveryManager recoveryManagerObj;
		boost::shared_ptr<RecoveryManager> recoveryManagerObj;
		recoveryManagerObj.reset(new RecoveryManager());
#ifdef DEBUG_MODE
		ASSERT_EQ(true,recoveryManagerObj->start_service("proxy", "8080"));
		ASSERT_EQ(true,recoveryManagerObj->stop_service("proxy"));
		ASSERT_EQ(true,recoveryManagerObj->start_all_service("8080"));
		OSDLOG(DEBUG, "EXECUTED COMMAND IS  ="<< recoveryManagerObj->stop_all_service());
		ASSERT_EQ(true,recoveryManagerObj->stop_all_service());
		recoveryManagerObj->kill_service("proxy");
#endif
	}
}
