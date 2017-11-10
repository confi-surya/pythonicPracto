#include "osm/localLeader/service_startup_manager.h"

using namespace objectStorage;
ServiceStartupManager::ServiceStartupManager(
						boost::shared_ptr<MonitoringManager> mtr_mgr, 
						enum network_messages::NodeStatusEnum stat, 
						boost::shared_ptr<HfsChecker> checker,
						cool::SharedPtr<config_file::OSMConfigFileParser>config_parser )
{
	this->monitor_mgr = mtr_mgr;
	this->hfs_checker = checker;
	this->node_stat = stat;
	this->if_all_service_start = false;
	this->config_reader.reset(new ServiceConfigReader());
	this->config_parser = config_parser;
	this->add_task(config_parser->account_server_nameValue());
	this->add_task(config_parser->container_server_nameValue());
	this->add_task(config_parser->object_server_nameValue());
	this->add_task(config_parser->proxy_server_nameValue());
	this->add_task(config_parser->account_updater_server_nameValue());
	this->add_task(config_parser->containerChild_nameValue());
#if 0
	this->add_task("account");
	this->add_task("container");
	this->add_task("object");
	this->add_task("proxy");
	this->add_task("account-updater");
	this->add_task("containerChild");
#endif
}

ServiceStartupManager::~ServiceStartupManager()
{
//	this->config_reader.reset();
}

bool ServiceStartupManager::verify_license_available()
{
	GLUtils utils;
	return utils.license_check();
}
#ifdef DEBUG_MODE
bool ServiceStartupManager::verify_hfs_available()
{
	return true;
}
#else
bool ServiceStartupManager::verify_hfs_available()
{
	GLUtils utils;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
	if( osd_config_dir)
		return false;
	bool flag = utils.mount_file_system(this->config_parser->export_pathValue(), 
			this->config_parser->mount_commandValue(), this->config_parser->HFS_BUSY_TIMEOUTValue());
	if( !flag)
	{
		if(this->node_stat == network_messages::RECOVERED)
		{
			uint64_t timeout = this->config_parser->remount_intervalValue();
			boost::this_thread::sleep(boost::posix_time::seconds(timeout));
			return flag;
		}	
		return flag;
	}
	return flag;
}
#endif

bool ServiceStartupManager::enviormental_check()
{
	GLUtils utils;	
	bool flag = true;
	while( !this->verify_license_available())
	{
		if(this->node_stat == network_messages::RECOVERED)
			continue;
		break;
	}
	while( !this->verify_hfs_available() )
	{
		if(this->node_stat == network_messages::RECOVERED)
		{
			utils.umount_file_system(this->config_parser->export_pathValue(),
                                        this->config_parser->UMOUNT_HYDRAFSValue(), this->config_parser->HFS_BUSY_TIMEOUTValue());
			continue;
		}
		flag = false;
		break;
	}
	return flag;
}
#ifdef DEBUG_MODE
bool ServiceStartupManager::verify_all_service_started()
{
	return true;
}
#else
bool ServiceStartupManager::verify_all_service_started()
{
	return this->if_all_service_start;
}
#endif

std::string ServiceStartupManager::get_service_id(std::string service_name) const
{
	GLUtils utils;
	uint16_t port = this->config_parser->osm_portValue();
	return utils.get_osd_service_id(port, service_name);
}

bool ServiceStartupManager::task_excuter(std::string service_name, std::string action)
{
	std::string cmd =  "";
	CommandExecuter cmd_exec;
	int status = 0;
	FILE *fp;
	//if(!this->verify_hfs_available())	//TODO: to be verified by Aditya san
	//	return false;
	if( service_name.compare(config_parser->account_updater_server_nameValue()) != 0) 
	{
		if(service_name.compare(config_parser->container_server_nameValue()) == 0 && ((this->node_stat == network_messages::RECOVERED) || (this->node_stat == network_messages::NODE_STOPPED))) {
		
			service_name = service_name + "-" + "server";
			cmd = cmd_exec.get_command(service_name, action);	
			cmd = cmd + " -l" + " " + boost::lexical_cast<std::string>(this->config_parser->osm_portValue()) + " --clean_journal";
		}else {
			service_name = service_name + "-" + "server";
			cmd = cmd_exec.get_command(service_name, action);
			cmd = cmd + " -l" + " " + boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
		}
	}	
	else
	{
		service_name = config_parser->account_updater_server_nameValue();
		cmd = cmd_exec.get_command(service_name, action);
		cmd = cmd + " -l" + " " + boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
	}
	OSDLOG(INFO, "Command for service start = "<<cmd);
	cmd_exec.exec_command(cmd.c_str(), &fp, &status);		
	OSDLOG(INFO, "Status of command is = "<< (status));
	if( (status))	
		return false;
	return true; 
}

bool ServiceStartupManager::stop_all_services()
{
	if(!this->monitor_mgr->stop_all_service())
		return false;
	return true;	
}

void ServiceStartupManager::add_task(std::string service_name)
{
	this->task_list.push_back(service_name);
}

void ServiceStartupManager::remove_task(std::string service_name)
{
	cool::unused(service_name);
	this->task_list.clear();
}

void ServiceStartupManager::remove_all_task()
{
	this->task_list.clear();
}

void ServiceStartupManager::start()
{
	OSDLOG(INFO, "Service Startup thread start");
	if( !this->enviormental_check() ) 
	{
		this->if_all_service_start = false;		
		return;
	}
	else
	{
		std::vector<std::string>::iterator it;
		for( it = this->task_list.begin(); it != this->task_list.end(); it++)
		{
			bool skip_startup_flag = false;
			if((*it) == config_parser->containerChild_nameValue())
			{
				continue;
			}
			std::string service_id = this->get_service_id(*it);
			if(this->node_stat == network_messages::REGISTERED)
			{
				if( this->task_excuter(*it, "status"))
				{
					OSDLOG(DEBUG,"Skipping start as service already running : " << service_id);
					skip_startup_flag = true;
				}
			}
			else if( this->node_stat == network_messages::NEW)
			{
				if( this->task_excuter(*it, "status"))
				{
					OSDLOG(DEBUG, "Stop the running service");
					this->task_excuter(*it, "stop");
				}
			}
			int retry_count = 0;
			while( (skip_startup_flag == false) && (retry_count != this->config_reader->get_retry_count()) && !this->task_excuter(*it, "start"))
			{	
				retry_count++;
				if( retry_count == this->config_reader->get_retry_count())
				{
					GLUtils gl_util;
					OSDLOG(INFO, "Osd service start failed "<<*it);
					CLOG(SYSMSG::OSD_SERVICE_START_FAILED, gl_util.get_my_node_id().c_str());
					this->if_all_service_start = false;
					return;
				}
			}
			OSDLOG(INFO, "Registered this service");
			this->monitor_mgr->register_service(service_id, this->config_reader->get_strm_timeout(*it),
				this->config_reader->get_heartbeat_timeout(),  this->config_reader->get_service_start_time_gap(),
				this->config_reader->get_retry_count());
			if((*it) == config_parser->container_server_nameValue())
			{
				std::string container_child_service_id = this->get_service_id(config_parser->containerChild_nameValue());
				OSDLOG(INFO, "Registered container server child : " << container_child_service_id);
				this->monitor_mgr->register_service(container_child_service_id, 
					this->config_reader->get_strm_timeout(config_parser->containerChild_nameValue()),
					this->config_reader->get_heartbeat_timeout(),  this->config_reader->get_service_start_time_gap(),
					this->config_reader->get_retry_count());
			}

		}
		OSDLOG(INFO, "Osd service starts successfuly");
		this->if_all_service_start = true;
	}
}
