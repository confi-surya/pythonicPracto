#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "osm/localLeader/monitoring_manager.h"
#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/service_config_reader.h"
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <cstdlib>

namespace MonitoringManagerTest
{

TEST(MonitoringManagerTest, MonitoringManagerTest)
{
	uint64_t strm_timeout = 1;
	uint64_t hrbt_timeout = 2;
	uint16_t time_gap = 300;
	uint16_t retry_count = 3;

	Communication::SynchronousCommunication *comm = NULL;
	boost::shared_ptr<HfsChecker>hfs_checker;
	GLUtils gl_utils_obj;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	config_parser = gl_utils_obj.get_config();
	ServiceConfigReader service_config_reader;

	hfs_checker.reset(new HfsChecker(config_parser));
	
	boost::shared_ptr<MonitoringManager> mgr;
        mgr.reset(new MonitoringManager(hfs_checker, config_parser));
	
	
	boost::shared_ptr<MonitoringManager> mgr2;
        mgr2.reset(new MonitoringManager(hfs_checker, config_parser));

	std::string container_service_id = "HN0101_60014_container-server";
	std::string containerChild_service_id = "HN0101_60014_containerChild-server";
	std::string account_service_id = "HN0101_60014_account-server";
	std::string proxy_service_id = "HN0101_60014_proxy-server";
	std::string object_service_id = "HN0101_60014_object-server";
	
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj(new Communication::SynchronousCommunication("192.168.131.123", 60123));

	bool val;
	int fd;

	mgr->start_monitoring(comm_obj, 1, proxy_service_id, 61005, "192.168.101.1");
	mgr->end_monitoring(comm_obj, proxy_service_id);
	val = mgr->un_register_service(1, proxy_service_id);
	OSDLOG(DEBUG, "UN Registration of service "<< val);
	val = mgr->register_service(account_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	ASSERT_EQ(val, true);
	val = mgr->register_service(proxy_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
//	val = mgr->register_service(object_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr->start_monitoring(comm_obj, 2, account_service_id, 61005, "192.168.101.1");
	val = mgr->register_service(account_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	OSDLOG(DEBUG, "Registration of service id "<< val);
	ASSERT_EQ(val, true);

	fd = 45;
	mgr->start_monitoring(comm_obj, 45, account_service_id, 61005, "192.168.101.1");
	
	val = mgr->notify_to_gl();
	OSDLOG(DEBUG, "NOTIFY TO GL "<< val);
	ASSERT_EQ(val, true);

	mgr->start_monitoring(comm_obj, fd, proxy_service_id, 61004, "192.168.101.1");	
	boost::tuple<std::list<boost::shared_ptr<ServiceObject> >, bool> l = mgr->get_service_port_and_ip();
	
	val = mgr2->register_service(account_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	val = mgr2->register_service(proxy_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr2->start_monitoring(comm_obj, fd, account_service_id, 61005, "192.168.101.1");
	mgr2->start_monitoring(comm_obj, fd, proxy_service_id, 61004, "192.168.101.1");
	sleep(10);
	
	mgr2->start_recovery_identifier();
	sleep(5);	

	val = mgr->unmount_file_system();
	OSDLOG(DEBUG, "unmount file system "<< val);
	ASSERT_EQ(val, false);	

	val = mgr->mount_file_system();
	OSDLOG(DEBUG, "mount file system "<< val);
	ASSERT_EQ(val, false);

	mgr->end_monitoring(comm_obj, account_service_id);
	val = mgr->is_register_service(account_service_id);
	OSDLOG(DEBUG, "IS Registered service "<< val);
	ASSERT_EQ(val, true);
	val = mgr->is_register_service(object_service_id);
	OSDLOG(DEBUG, "IS Registered service "<< val);
	ASSERT_EQ(val, false);

	val = mgr->un_register_service(2, account_service_id);
	OSDLOG(DEBUG, "UN Registration of service "<< val);
	val = mgr->register_service(account_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	
#ifndef DEBUG_MODE	
	val = mgr->stop_all_service();
	OSDLOG(DEBUG, "stop service "<< val);
	ASSERT_EQ(val, false);
#endif

#ifdef DEBUG_MODE
	 val = mgr->stop_all_service();
	OSDLOG(DEBUG, "stop all service "<< val);
	ASSERT_EQ(val, true);
#endif


#ifndef DEBUG_MODE
	val = mgr->start_all_service();
	OSDLOG(DEBUG, "start service "<< val);
	ASSERT_EQ(val, false);
#else
	val = mgr->start_all_service();
	ASSERT_EQ(val, true);
#endif

	val = mgr->register_service(containerChild_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr->start_monitoring(comm_obj, fd, containerChild_service_id, 61005, "192.168.101.1");
	val = mgr->system_stop_operation();
	OSDLOG(DEBUG, "system_stop_opn "<< val);
	ASSERT_EQ(val, true);

	mgr->send_http_requeset_for_service_stop(61005, "192.168.101.1", account_service_id);
	mgr->end_all();

	FILE *fp;
	fp = fopen("test.dat", "w");
	char buff[50] = "hello my name is you";
	for(int i = 0; i < 100; i++) {
		fprintf(fp, "%s\n", buff);
	}
	
	fd = fileno(fp);
	OSDLOG(DEBUG, "fd of test.dat is "<< fd);
	hfs_checker->start();
	boost::shared_ptr<MonitoringManager> mgr3;
        mgr3.reset(new MonitoringManager(hfs_checker, config_parser));
	val = mgr3->register_service(proxy_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr3->start_monitoring(comm_obj, fd, proxy_service_id, 61005, "192.168.101.1");
	sleep(10);
	mgr3->start_recovery_identifier();
	
	hfs_checker->start();
	boost::shared_ptr<MonitoringManager> mgr4;
        mgr4.reset(new MonitoringManager(hfs_checker, config_parser));
	val = mgr4->register_service(proxy_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr4->start_monitoring(comm_obj, fd, proxy_service_id, 61005, "192.168.101.1");

	boost::shared_ptr<boost::thread> monitoringMgrThread;
        monitoringMgrThread.reset(new boost::thread(boost::bind(&MonitoringManager::start, mgr4)));
        sleep(5);
        monitoringMgrThread->interrupt();
	monitoringMgrThread->join();

	std::string s = "test.dat";	
	std::string command = "rm " + s;
	std::system(command.c_str());
	fclose(fp);
	
	hfs_checker->stop();
  
	boost::shared_ptr<MonitoringManager> mgr5;
	mgr5.reset(new MonitoringManager(hfs_checker, config_parser));
	val = mgr5->register_service(container_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr5->start_monitoring(comm_obj, fd+2, container_service_id, 61005, "192.168.101.1");
	val = mgr5->register_service(containerChild_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr5->start_monitoring(comm_obj, fd+5, containerChild_service_id, 61005, "192.168.101.1");
	sleep(10);
	mgr5->start_recovery_identifier();
	sleep(10);

	hfs_checker->stop();
	boost::shared_ptr<MonitoringManager> mgr6;
        mgr6.reset(new MonitoringManager(hfs_checker, config_parser));
	val = mgr6->register_service(containerChild_service_id, strm_timeout, hrbt_timeout, time_gap, retry_count);
	mgr6->start_monitoring(comm_obj, fd+4, containerChild_service_id, 61005, "192.168.101.1");
	sleep(10);
	mgr6->start_recovery_identifier();
	sleep(10);
	

	std::string gfs_path = config_parser->osm_info_file_pathValue();
	std::string infofile = gfs_path + "/osd_config" + "/osd_global_leader_information.info";
	boost::filesystem::remove(boost::filesystem::path(infofile));
}
}

 // namespace MonitoringManagerTest
