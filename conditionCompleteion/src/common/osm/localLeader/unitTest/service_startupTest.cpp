#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "osm/localLeader/service_startup_manager.h"
#include <iostream>
#include "osm/localLeader/monitoring_manager.h"
#include "osm/osmServer/utils.h"
#include <cool/cool.h>
#include <cool/misc.h>

namespace ServiceStartupManagerTest
{

TEST(ServiceStartupManagerTest, ServiceStartupManagerTest)
{
	GLUtils gl_utils_obj;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	config_parser = gl_utils_obj.get_config();

	boost::shared_ptr<hfs_checker::HfsChecker> checker;
	checker.reset(new hfs_checker::HfsChecker(config_parser));

	boost::shared_ptr<MonitoringManager> mtr_mgr;
	mtr_mgr.reset(new MonitoringManager(checker, config_parser));
	
	boost::shared_ptr<ServiceStartupManager> startup_mgr_test;
	startup_mgr_test.reset(new ServiceStartupManager(mtr_mgr, network_messages::RECOVERED, checker, config_parser));
	
	bool val;

	val =  startup_mgr_test->verify_license_available();
	ASSERT_EQ(val, true);

#ifdef DEBUG_MODE
	ASSERT_EQ(startup_mgr_test->verify_hfs_available(), true);	
	ASSERT_EQ(startup_mgr_test->enviormental_check(), true);
#else	
	ASSERT_EQ(startup_mgr_test->verify_hfs_available(), false);
#endif

#ifdef DEBUG_MODE
	 val = startup_mgr_test->verify_all_service_started();
	 ASSERT_EQ(val, true);
#else
	 val = startup_mgr_test->verify_all_service_started();
	 ASSERT_EQ(val, false);
#endif 

#ifndef DEBUG_MODE
	val = startup_mgr_test->stop_all_services();
	OSDLOG(DEBUG, "stop all service "<< val);
	ASSERT_EQ(val, false);
#endif
//	ASSERT_FALSE(val);

#ifdef DEBUG_MODE
	val = startup_mgr_test->stop_all_services();
	 ASSERT_EQ(val, true);
#endif

	std::string s = startup_mgr_test->get_service_id(std::string ("account"));
	ASSERT_EQ(s, std::string("hydra042_61014_account-server"));	
	
#ifndef DEBUG_MODE	
	val = startup_mgr_test->task_excuter("account-updater", "");
	OSDLOG(DEBUG, " task executor "<< val);
	ASSERT_EQ(val, false);
	val = startup_mgr_test->task_excuter("account-server", "");
	ASSERT_EQ(val, false);
#else
	val = startup_mgr_test->task_excuter("account-updater", "");
	ASSERT_EQ(val, true);
	val = startup_mgr_test->task_excuter("account-server", "");
	OSDLOG(DEBUG, "account service "<< val);
	ASSERT_EQ(val, true);
#endif
	
	
#ifdef DEBUG_MODE
	startup_mgr_test->start();

	startup_mgr_test.reset(new ServiceStartupManager(mtr_mgr, network_messages::FAILED, checker, config_parser));
	startup_mgr_test->start();
	
	startup_mgr_test.reset(new ServiceStartupManager(mtr_mgr, network_messages::NEW, checker, config_parser));
	startup_mgr_test->start();
#endif
	startup_mgr_test->remove_task("account");
	startup_mgr_test->remove_all_task();
}
}

