#ifndef __service_startup_manager_h__
#define __service_startup_manager_h__

#include <stdio.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <cool/misc.h>

#include "libraryUtils/object_storage_exception.h"

#ifdef INCL_PROD_MSGS
#include "msgcat/HOSSyslog.h"
#endif

#include "osm/localLeader/monitoring_manager.h"
#include "osm/localLeader/recovery_handler.h"
#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/service_config_reader.h"
#include "osm/osmServer/utils.h"

using namespace hfs_checker;

class ServiceStartupManager
{
private:
	boost::shared_ptr<HfsChecker> hfs_checker;
	boost::shared_ptr<MonitoringManager> monitor_mgr;
	boost::shared_ptr<ServiceConfigReader> config_reader;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	std::vector<std::string> task_list;
	bool n_restart;
	//enum OSDNodeStat::NodeStatStatus::node_stat node_stat;
	enum network_messages::NodeStatusEnum node_stat;
	bool if_all_service_start;
public:	
	bool verify_hfs_available();	
	bool enviormental_check();
	void command_executer(const char* cmd);
	bool task_excuter(std::string service_name, std::string action);		
	bool service_stop(std::string service_name);
	int service_config_file_reader();
	std::string get_service_id(std::string) const;
	bool verify_license_available();
public:
	void add_task(std::string service_name);
	void remove_task(std::string service_name);
	void remove_all_task();
public:
	ServiceStartupManager(boost::shared_ptr<MonitoringManager> mtr_mgr, 
				//enum OSDNodeStat::NodeStatStatus::node_stat,
				enum network_messages::NodeStatusEnum,
				boost::shared_ptr<HfsChecker> checker,
				cool::SharedPtr<config_file::OSMConfigFileParser>);
	virtual ~ServiceStartupManager();
	bool verify_all_service_started();
	bool stop_all_services();
	void start();
};
#endif 
