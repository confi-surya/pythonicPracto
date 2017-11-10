#include "osm/localLeader/monitoring_manager.h"
#include "communication/message.h"
#include "communication/message_result_wrapper.h"
#include "communication/message_type_enum.pb.h"
//#include "communication/message_interface_handlers.h"
#include "communication/communication.h"
#include <iostream>
#include <vector>
using namespace objectStorage;
void MonitoringManager::insert(int fd)
{
    if(fd < 0)
    {
        return;	
    }
    FD_SET(fd, &this->m_fd_set);
    this->m_fds.insert(fd);
    if (fd > this->m_max) this->m_max = fd;
}

void  MonitoringManager::erase(int fd)
{
    if(fd < 0)
    {
        return;
    }
    FD_CLR(fd, &this->m_fd_set);
    this->m_fds.erase(fd);
    if (fd == this->m_max) {
        if (this->m_fds.size() == 0) {
            this->m_max = -1;
        } else {
            this->m_max = *m_fds.rbegin();
        }
    }	
}

MonitoringManager::MonitoringManager(boost::shared_ptr<HfsChecker> checker,
        cool::SharedPtr<config_file::OSMConfigFileParser>config_parser)
{
    this->stop = false;
    this->recovery_disable  = false;
    FD_ZERO(&this->m_fd_set);
    this->m_max = -1;
    this->recovery_identifier_thread.reset();
    this->recovery_process_thread.reset();
    this->hfs_checker = checker;
    this->config_parser = config_parser;
    this->notify_to_gl_result = false;
}

MonitoringManager::~MonitoringManager()
{
    FD_ZERO(&this->m_fd_set);
    this->recovery_identifier_thread.reset();	
    this->hfs_checker.reset();
    this->config_parser.reset();
}

void MonitoringManager::failure_handler()
{
    OSDLOG(INFO, "Do nothing");
}

void MonitoringManager::success_handler(boost::shared_ptr<MessageResultWrap> result)
{
}

void MonitoringManager::send_ack_to_service(std::string service_id, boost::shared_ptr<Communication::SynchronousCommunication> comm, bool flag)
{
    boost::shared_ptr<ErrorStatus> error_status;
    std::string msg = "";
    if( !flag)
    {
        OSDLOG(DEBUG, "send negative ack of strm packet to osd service");
        msg = "failed the start the monitoring of osd service";
        error_status.reset( new ErrorStatus(OSDException::OSDErrorCode::OSD_START_MONITORING_FAILED, msg));
    }
    else
    {
        OSDLOG(DEBUG, "Send success ack to osd service for strm packet");
        msg = "start the monitoring of osd service";
        error_status.reset( new ErrorStatus(OSDException::OSDSuccessCode::OSD_START_MONITORING_SUCCESS, msg));
    }
    boost::shared_ptr<comm_messages::OsdStartMonitoringAck> obj(new comm_messages::OsdStartMonitoringAck(
                service_id, error_status)); 
    boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(messageTypeEnum::typeEnums::OSD_START_MONITORING_ACK));
    MessageExternalInterface::sync_send_osd_start_monitoring_ack(
            obj.get(),
            comm,
            boost::bind(&MonitoringManager::success_handler, this, result1),
            boost::bind(&MonitoringManager::failure_handler, this)
            );
}

void MonitoringManager::start_monitoring(boost::shared_ptr<Communication::SynchronousCommunication> comm,int fd, std::string service_id, 
        int32_t port, std::string ip)
{
    if( verify_recovery_disable() )
        return;
    std::map < std::string, MonitorStatusMap > ::iterator it;
    it = this->monitor_status_map.find(service_id);
    if(it != this->monitor_status_map.end())
    {
        it->second.set_fd(fd);
        it->second.set_comm_obj(comm);
        it->second.set_ip_addr(ip);
        it->second.set_port(port);	
        it->second.set_service_status(OSDException::OSDServiceStatus::RUN);
        it->second.set_heartbeat_count(HEARTBEAT_COUNT);
        it->second.set_timeout(it->second.get_hrbt_timeout());
	it->second.update_last_update_time();
        this->insert(fd);
//        this->send_ack_to_service(it->first, it->second.get_comm_obj(), true);
        this->send_ack_to_service(it->first, comm, true);	//TODO: devesh: verify this change
        return; 
    }
    OSDLOG(DEBUG, "start_monitoring Service is not registered : "<< service_id);
    this->send_ack_to_service(service_id, comm, false);
}

void MonitoringManager::end_monitoring(boost::shared_ptr<Communication::SynchronousCommunication> comm, std::string service_id)
{
    std::map < std::string, MonitorStatusMap > ::iterator it;
    it = this->monitor_status_map.find(service_id);
    if(it != this->monitor_status_map.end())
    {
        this->erase(it->second.get_fd());
        it->second.set_fd(-1);
        it->second.set_service_status(OSDException::OSDServiceStatus::UREG);
        it->second.set_msg_type(OSDException::OSDMsgTypeInMap::ENDM);
	boost::shared_ptr<Communication::SynchronousCommunication> sync_obj(it->second.get_comm_obj());
        //this->send_ack_to_service(it->first, it->second.get_comm_obj(), true);
        this->send_ack_to_service(it->first, sync_obj, true);
    }	
    OSDLOG(DEBUG, "end_monitoring Service is not registered" << service_id);
    this->send_ack_to_service(service_id, comm, false);
}

void MonitoringManager::end_all()
{
    std::map < std::string, MonitorStatusMap > ::iterator it;
    for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
    {
        boost::shared_ptr<Communication::SynchronousCommunication> sync_obj(it->second.get_comm_obj());
        this->end_monitoring(sync_obj, it->first);
    }
}

bool MonitoringManager::register_service(std::string service_id, uint64_t strm_timeout, uint64_t hrbt_timeout, uint16_t time_gap,
        uint16_t retry_count)   
{
    MonitorStatusMap msm(strm_timeout, hrbt_timeout, time_gap, retry_count) ;
    std::map < std::string, MonitorStatusMap > ::iterator it;
    it = this->monitor_status_map.find(service_id);
    if(it == this->monitor_status_map.end())
    {
        this->monitor_status_map.insert(std::make_pair(service_id, msm));		
        OSDLOG(INFO, "Registration of service id "<<service_id<< " successfully");
    }
    else
    {   
        OSDLOG(INFO, "Registration of service id "<<service_id<< "success");
	it->second = msm;
    }
        return true;
}

bool MonitoringManager::un_register_service(int fd, std::string service_id)
{
    std::map < std::string, MonitorStatusMap > ::iterator it;
    it = this->monitor_status_map.find(service_id);
    if(it != this->monitor_status_map.end())
    {
        this->erase(it->second.get_fd());
	boost::shared_ptr<Communication::SynchronousCommunication> com(it->second.get_comm_obj());
        this->send_ack_to_service(it->first, com, true);
        this->monitor_status_map.erase( it );
        return true;
    }
    else
    {
        OSDLOG(DEBUG, "Service is not registered");	
        return false;
    }
}

bool MonitoringManager::is_register_service(std::string service_id)
{
    std::map < std::string, MonitorStatusMap > ::iterator it;
    it = this->monitor_status_map.find(service_id);
    if(it != this->monitor_status_map.end())
    {
        return true;
    }
    return false;
}

bool MonitoringManager::recv_heartbeat(boost::shared_ptr<Communication::SynchronousCommunication> comm, 
	uint64_t & comm_code)
{
    boost::shared_ptr<ErrorStatus> error_status;
    boost::shared_ptr<MessageResult<comm_messages::HeartBeat> > heartbeat( new MessageResult<comm_messages::HeartBeat>());
    heartbeat = MessageExternalInterface::sync_read_heartbeat( comm);
    bool status = heartbeat->get_status();
    uint64_t return_code = heartbeat->get_return_code();
    if( !status)
    {
	    comm_code = return_code;
	    OSDLOG(DEBUG,"Communication error reported with return code : " << return_code);
        //OSDLOG(INFO, "Unable to receive the heartbeat from service");
        return false;
    }
    //OSDLOG(DEBUG, "Recvd the heartbeat from osd service successfully");
    boost::shared_ptr<comm_messages::HeartBeatAck> obj( new comm_messages::HeartBeatAck(123));
    boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(messageTypeEnum::typeEnums::HEART_BEAT_ACK));
    MessageExternalInterface::sync_send_heartbeat_ack(
		    obj.get(),
		    comm,
		    boost::bind(&MonitoringManager::success_handler, this, result1),
		    boost::bind(&MonitoringManager::failure_handler, this)
		    );  
    return true;
}

void MonitoringManager::stop_recovery_identifier()
{
    if(recovery_identifier_thread)
    {
	    this->recovery_identifier_thread->interrupt();
#ifndef DEBUG_MODE
	    this->recovery_identifier_thread->join();
#endif
	    this->recovery_identifier_thread.reset();
    }
}

void MonitoringManager::start_recovery_identifier()
{
    this->recovery_identifier_thread.reset(new boost::thread(boost::bind(&MonitoringManager::service_monitoring, this)));
}

void MonitoringManager::start()
{
    GLUtils gl_util;
    std::map < std::string, MonitorStatusMap > ::iterator it;
    OSDLOG(INFO, "Monitoring thread start");
    try
    {
        for(;;)
        {
            boost::this_thread::sleep( boost::posix_time::seconds(3));
            boost::this_thread::interruption_point();
            while( verify_recovery_disable() )
	    {
		boost::this_thread::sleep( boost::posix_time::seconds(5));
	    }
            if(this->m_max == -1 )
                continue;
	    int error;
	    std::set<int64_t> active_fds = gl_util.selectOnSockets(this->m_fds, error, this->config_parser->select_timeoutValue());
	    if(active_fds.size()<=0)
	    {
		    continue;
	    }
            for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
            {	
                int fd = it->second.get_fd();
                if(fd ==  -1)
                    continue;
		std::set<int64_t>::iterator active_fd_it = active_fds.find(fd);
		if(active_fd_it != active_fds.end()){
			uint64_t comm_code;
			boost::shared_ptr<Communication::SynchronousCommunication> com(it->second.get_comm_obj());
			if(this->recv_heartbeat(com, comm_code)) 
			{				
				it->second.update_last_update_time();
				OSDLOG(INFO, "Recvd the heartbeat from osd service successfully: " << it->first);
			}else{
				if(Communication::TIMEOUT_OCCURED == comm_code) {
					 /* 4 is communication error for TIMEOUT_OCCURED */
                                        OSDLOG(DEBUG, "TIMEOUT OCCURED while getting heartbeat");	
				}else  {
					/*16 is for communication error as bad FD, broken pipe*/
					this->erase(fd);
					it->second.set_fd(-1);
				}
				OSDLOG(INFO, "Unable to receive the heartbeat from service: " << it->first);
			}
		}
            }
        }
    }
    catch (boost::thread_interrupted&) 
    {
        OSDLOG(INFO, "Monitoring thread stop");
    }

}

bool MonitoringManager::is_hfs_avaialble()
{
    if( this->hfs_checker->get_gfs_stat() != OSDException::OSDHfsStatus::NOT_READABILITY )	
        return true;
    return false;
}
bool MonitoringManager::if_notify_to_gl_success() const
{
	return this->notify_to_gl_result;
}
bool MonitoringManager::notify_to_gl()
{
    OsmUtils ut;
    GLUtils utils;
    boost::tuple<std::string, uint32_t> info = ut.get_ip_and_port_of_gl();
    this->notify_to_gl_result = false;
    OSDLOG(INFO, "Connect to gl with ip and port: "<<boost::get<0>(info)<<"  "<<boost::get<1>(info));
	
    if(info.get<1>() == 0) {
		PASSERT(false, "CORRUPTED ENTRIES IN GL INFO FILE");
    }

    boost::shared_ptr<Communication::SynchronousCommunication> comm( 
        new Communication::SynchronousCommunication( boost::get<0>(info), boost::get<1>(info)));


	if(!comm->is_connected())
	{
    		OSDLOG(INFO, "Connection from gl failed: "<<boost::get<0>(info)<<"  "<<boost::get<1>(info));
		return false;
	}
    boost::shared_ptr<ErrorStatus> error_status( new ErrorStatus(0, ""));
    std::string node_id = utils.get_my_node_id() + "_" +
        boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
    boost::shared_ptr<comm_messages::NodeFailover> obj( new comm_messages::NodeFailover(node_id, error_status));	
    boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(messageTypeEnum::typeEnums::NODE_FAILOVER));
    MessageExternalInterface::sync_send_node_failover(
            obj.get(),
            comm,
            boost::bind(&MonitoringManager::success_handler, this, result1),
            boost::bind(&MonitoringManager::failure_handler, this)
            );
    boost::shared_ptr<MessageResult<comm_messages::NodeFailoverAck> >ack_obj;
    ack_obj = MessageExternalInterface::sync_read_node_failover_ack(comm);
    if( !ack_obj->get_status())
    {
        OSDLOG(DEBUG, "Failed to recvd node failover ack from gl");
        if( comm)
            //delete comm;
	    comm.reset();
//        comm = NULL;
        return false;
    }
    OSDLOG(DEBUG, "Node failover done");
    if( comm)
        comm.reset();
//        delete comm;
//    comm = NULL;
    this->notify_to_gl_result = true;
    return true;
}

void MonitoringManager::join()
{
#ifndef DEBUG_MODE
       if(this->recovery_identifier_thread)
    	{
	    this->recovery_identifier_thread->join();
    	}
#endif
    /*boost::mutex::scoped_lock lock(this->mtx);
      while (!this->stop);
      this->condition.wait(lock);*/
}

void MonitoringManager::enable_mon_thread()
{
	OSDLOG(DEBUG, "Enable monitoring thread");
	this->stop = false;
}

void MonitoringManager::stop_mon_thread()
{
    //boost::mutex::scoped_lock lock(this->mtx);
    OSDLOG(DEBUG, "Disable monitoring thread");
    this->stop = true;
    //this->condition.notify_one();
}

void MonitoringManager::wait(uint16_t timeout)
{
    boost::asio::io_service io_service_obj;
    boost::asio::deadline_timer timer(io_service_obj);
    timer.expires_from_now(boost::posix_time::seconds(timeout));
    io_service_obj.run();
    timer.wait();
}

bool MonitoringManager::unmount_file_system()
{
    OSDLOG(INFO, "Unmount the all global filesystem");
    GLUtils utils;
    char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
    if( osd_config_dir)
        return false;
    return utils.umount_file_system(this->config_parser->export_pathValue(),
            this->config_parser->UMOUNT_HYDRAFSValue(), this->config_parser->HFS_BUSY_TIMEOUTValue()); 
}

bool MonitoringManager::mount_file_system()
{
    OSDLOG(INFO, "Mount the all global filesystem");
    char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
    if( osd_config_dir)
        return false;
    GLUtils utils;
    return utils.mount_file_system(this->config_parser->export_pathValue(), this->config_parser->mount_commandValue(),
            this->config_parser->HFS_BUSY_TIMEOUTValue());
}

bool MonitoringManager::verify_recovery_disable()
{
    return this->recovery_disable;
}

void MonitoringManager::change_recovery_status(bool flag)
{
    OSDLOG(DEBUG, "Change the recovery status "<<flag);
    this->recovery_disable = flag;
} 

boost::tuple<std::list<boost::shared_ptr<ServiceObject> >, bool> MonitoringManager::get_service_port_and_ip()
{
    std::list<boost::shared_ptr<ServiceObject> > service_info_list;
    std::map < std::string, MonitorStatusMap > ::iterator it;
    for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
    {
        if( it->second.get_port() == -1)
        {	
            service_info_list.clear();
            return boost::make_tuple(service_info_list, false);	
        }
        boost::shared_ptr<ServiceObject> service_record( new ServiceObject());
        service_record->set_service_id(it->first);
        service_record->set_ip(it->second.get_ip_addr());
        service_record->set_port(it->second.get_port());
        service_info_list.push_back(service_record);
    }
    OSDLOG(INFO, "Return service info_list");
    return boost::make_tuple(service_info_list, true);
}

std::string service_name_only (const std::string& str)
{
	  std::size_t found = str.find_last_of("-");
	    return str.substr(0,found);
}

void MonitoringManager::service_monitoring(void)
{
    OSDLOG(INFO, "Recovery Identifier thread start");
    try
    {
        std::map < std::string, MonitorStatusMap > ::iterator it;
	map<string, boost::tuple<boost::thread::native_handle_type, bool> > :: iterator recovery_thread_it;
        for(;;)
	{
		if( this->stop)
		{
			OSDLOG(INFO, "Exiting Recovery Identifier thread ");
			return;
		}
		boost::this_thread::sleep( boost::posix_time::seconds(3));	
		boost::this_thread::interruption_point();
		bool flag = false;
		for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
		{
			if( this->stop)
			{
				OSDLOG(INFO, "Exiting Recovery Identifier thread ");
				return;
			}
			bool stop_service_recovery = false;
			time_t now;
			time(&now);
			if ( ((now - it->second.get_last_update_time()) > it->second.get_timeout()) 
					&& ((it->second.get_service_status() == OSDException::OSDServiceStatus::REG) ||
						(it->second.get_service_status() == OSDException::OSDServiceStatus::RUN)))
			{
				OSDLOG(INFO, "Recovery of "<< it->first<<" Start "<< it->second.get_service_status()); 
				if(strstr((it->first).c_str(), "containerChild-server") != NULL)
				{
					std::map < std::string, MonitorStatusMap > ::iterator it2;
					for(it2 = this->monitor_status_map.begin(); it2 != this->monitor_status_map.end(); it2++)
					{
						if(strstr((it2->first).c_str(), "container-server") != NULL)
						{
							this->erase(it2->second.get_fd());
							it2->second.set_msg_type(OSDException::OSDMsgTypeInMap::STRM);
							it2->second.set_service_status(OSDException::OSDServiceStatus::MTR);
							break;
						}
					}
				}
				if(strstr((it->first).c_str(), "container-server") != NULL)
				{
					std::map < std::string, MonitorStatusMap > ::iterator it2;
					for(it2 = this->monitor_status_map.begin(); it2 != this->monitor_status_map.end(); it2++)
					{
						if(strstr((it2->first).c_str(), "containerChild-server") != NULL)
						{
							this->erase(it2->second.get_fd());
							it2->second.set_msg_type(OSDException::OSDMsgTypeInMap::STRM);
							it2->second.set_service_status(OSDException::OSDServiceStatus::MTR);
							break;
						}
					}
				}
				this->erase(it->second.get_fd());
				it->second.set_msg_type(OSDException::OSDMsgTypeInMap::STRM);
				it->second.set_service_status(OSDException::OSDServiceStatus::MTR);
				if( this->is_hfs_avaialble())
				{
					if( this->stop)
					{
						OSDLOG(INFO, "Exiting Recovery Identifier thread ");
						return;
					}
					it->second.set_service_status(OSDException::OSDServiceStatus::MTR);
					if( this->verify_recovery_disable() )
						continue;
					GLUtils gl_util;
					std::string service_name = this->get_service_name(it->first);
					if(config_parser->containerChild_nameValue() + "-server" != std::string(service_name))
					{
						std::string service_only = service_name_only(service_name);
						CLOG(SYSMSG::OSD_SERVICE_RESTART, gl_util.get_my_node_id().c_str(), service_only.c_str());
					}
			
					this->recovery_process_thread.reset( 
							new boost::thread(boost::bind(&MonitoringManager::start_service_recovery,
									this, it->first)));
				}
				else
				{
					OSDLOG(INFO, "HFS is not available");
					bool fail_over_done = false;
					this->unmount_file_system();
					if( this->mount_file_system())
					{
						OSDLOG(INFO, "Hfs is available and start the all osd service"); 
						if( this->start_all_service())
							continue;
					}	
					OSDLOG(INFO, "Stop the all osd service because hfs is not availbale");	
					if( !this->stop_all_service())
					{
						OSDLOG(INFO, "Unable to stop the all osd service");
						this->change_recovery_status(false);	
						it->second.set_service_status(OSDException::OSDServiceStatus::MTF);
						continue;
					}
					bool hfs_avaialble = false;
					while(!this->is_hfs_avaialble())
					{
						if( !fail_over_done && this->notify_to_gl())
						{
							OSDLOG(INFO, "Notification to gl failed for node failover");	
							fail_over_done = true;
						}
						this->unmount_file_system();
						if( this->mount_file_system())
						{
							hfs_avaialble = true;
							uint64_t timeout = this->config_parser->hfs_avaialble_check_intervalValue();
							boost::this_thread::sleep(boost::posix_time::seconds(timeout));
							continue;
						}
						uint64_t timeout = this->config_parser->remount_intervalValue();
						boost::this_thread::sleep(boost::posix_time::seconds(timeout));
					}
					if( hfs_avaialble )
					{
						OSDLOG(INFO, "Hfs is available and start the all osd service");
						if( this->start_all_service())
							continue;
					}
				}
			}
			else if((it->second.get_service_status() == OSDException::OSDServiceStatus::MTF))
			{
				/*
				   if( !this->stop_all_service() )
				   {
				   OSDLOG(INFO, "All osd service stop failed");	
				   this->change_recovery_status(false);
				   it->second.set_service_status(OSDException::OSDServiceStatus::MTF);
				   continue;
				   }
				 */
				this->stop_all_service();
				OSDLOG(INFO, "All osd services stop successfully");	
				uint8_t retry_count = 0;
				while( retry_count != MAX_RETRY_COUNT && !this->notify_to_gl()) 
				{					
					OSDLOG(INFO, "Unable to send the notification to gl for node failover");	
					retry_count++;
					uint64_t timeout = this->config_parser->thread_wait_timeoutValue();	
					OSDLOG(INFO, "Wait for next try send to notify to gl for node failover"<<timeout);
					boost::this_thread::sleep(boost::posix_time::seconds(timeout));
				}
				this->stop_all_service();	
				
				OSDLOG(INFO, "Terminate the recovery indentifier thread");
				flag = true;			
				//this->terminate();
				break;		
			}
		}
			if( flag)
				break;
	}
    }
    catch(boost::thread_interrupted&)
    {
        OSDLOG(INFO, "Closing Recovery Indentifier Thread\n");
    }
}

std::string MonitoringManager::get_service_name(std::string service_id) 
{
	/*start*/
	std::size_t pos = service_id.find_last_of('_');
	if(pos == std::string::npos)
	{
		OSDLOG(DEBUG,"Wrong Service Id : " << service_id);
		return std::string("");
	}
	std::string service_name = service_id.substr(pos+1);
	return service_name;
	/*end*/
	/*
    const char* name = NULL;  
    name = service_id;
    if( !name)
    {
        return NULL;
    }
    name = service_id + std::strlen(service_id) - 1;
    while( name != service_id && *name != '_')
        --name;
    if( name == service_id )
        return NULL;
    return ++name;
    */
}

void MonitoringManager::start_service_recovery(std::string service_id)
{
    std::map < std::string, MonitorStatusMap > ::iterator it;
    it = this->monitor_status_map.find(service_id);	
    if ( it != this->monitor_status_map.end() )
    {
        string service_name = this->get_service_name(service_id.c_str());
        OSDLOG(INFO, "Starting recovery for service " << service_name);
        if ( service_name== std::string(""))
	{
		OSDLOG(DEBUG, "service name returned NULL");
		return;
	}
/* handling container-server-child special case start */
	//if(std::string("containerChild-server") == std::string(service_name))
	if(config_parser->containerChild_nameValue() + "-server" == std::string(service_name))
	{
            	RecoveryManager recovery_mgr;	
		/* Make service name as container-server */
		service_name = std::string(config_parser->container_server_nameValue() + "-server").c_str();
		recovery_mgr.stop_service(service_name);
		OSDLOG(DEBUG, "Container-server-child not working, starting container server recovery");
/* handling container-server-child special case end */
	//}else if(std::string("account-updater-server") == std::string(service_name))
	}else if(config_parser->account_updater_server_nameValue() + "-server" == std::string(service_name))
	{
/* account-updater-server is in service_name, but command wants account-updater start */
		service_name = std::string(config_parser->account_updater_server_nameValue()).c_str();
/* account-updater-server is in service_name, but command wants account-updater end */
	}

        int retry_count = 1;
        while( retry_count <= it->second.get_retry_count() )  
        {
            RecoveryManager recovery_mgr;	
            if( recovery_mgr.start_service(service_name, 
                        boost::lexical_cast<std::string>(this->config_parser->osm_portValue())))  
            {
                it->second.set_service_status(OSDException::OSDServiceStatus::REG);
                it->second.set_heartbeat_count(1);
                it->second.set_timeout(it->second.get_strm_timeout());
                it->second.update_last_update_time();
                OSDLOG(INFO, "Recovery of " << service_name << " completed successfully");

                break;	
            }	
            else {
                retry_count++; 
                OSDLOG(DEBUG, "Count for attempt to start the service  = "<< retry_count );	
                uint64_t timeout = this->config_parser->thread_wait_timeoutValue();
                boost::this_thread::sleep(boost::posix_time::seconds(timeout));
            }
        }
        if( retry_count > it->second.get_retry_count() ) 
        {
	    GLUtils gl_util;
            OSDLOG(INFO, service_name<<" failed to start the service in all attempt"); 
	    CLOG(SYSMSG::OSD_SERVICE_START_FAILED, gl_util.get_my_node_id().c_str());
	    if(OSDException::OSDServiceStatus::NDFO != it->second.get_service_status())
	    {
		    it->second.set_service_status(OSDException::OSDServiceStatus::MTF);
		    this->change_recovery_status(true);
	    }
	    this->all_service_started = false;
        } 
    }
}


void MonitoringManager::update_table_in_case_stop_service()
{
    std::map < std::string, MonitorStatusMap > ::iterator it;
    for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
    {
        if( it->second.get_fd() > 0)
            this->erase(it->second.get_fd());
        it->second.set_service_status(OSDException::OSDServiceStatus::NDFO);
    }
} 


bool MonitoringManager::stop_all_service()
{
    OSDLOG(INFO, "Stop the all service");
    this->change_recovery_status(true);
    this->update_table_in_case_stop_service();
    RecoveryManager recovery_mgr;
    bool retVal = recovery_mgr.stop_all_service();
    if(retVal == false)
    {
	    /* stop all could not stop all services, need to kill services forcefully */
	    this->kill_all_services();
    }
    return retVal;
}
/*
bool MonitoringManager::node_stop_operation(int fd)
{
    cool::unused(fd);
    RecoveryManager recovery_mgr;
    this->update_table_in_case_stop_service();
    recovery_mgr.stop_service(config_parser->proxy_server_nameValue());
    this->notify_to_gl();
    recovery_mgr.stop_all_service();	
    //[TODO]: send the ack to user on given fd
    return true;
}
*/

bool MonitoringManager::start_all_service()
{
    this->all_service_started = true;
    RecoveryManager recovery_mgr;
    std::map < std::string, MonitorStatusMap > ::iterator it;
    for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
    {
	    start_service_recovery(it->first);
    }
    return this->all_service_started;
#if 0 
    if(!recovery_mgr.start_all_service( boost::lexical_cast<std::string>((this->config_parser->osm_portValue()))))
        return false;
    std::map < std::string, MonitorStatusMap > ::iterator it;
    for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
    {
        it->second.set_service_status(OSDException::OSDServiceStatus::REG);
    }
    return true;	
#endif
}

void MonitoringManager::send_http_requeset_for_service_stop(int32_t port, std::string ip, std::string service_name)
{
    boost::shared_ptr<MessageResult<comm_messages::BlockRequestsAck> > obj;
    boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication(ip, port));
	if(!comm->is_connected())
	{
    		OSDLOG(INFO, "Http connection with service failed: "<< service_name);
		return ;
	}
    std::map<std::string, std::string> dict;
    dict["X-GLOBAL-MAP-VERSION"] = "-1";
    comm_messages::BlockRequests req(dict);
    uint32_t timeout = this->config_parser->service_stop_intervalValue();
    obj = MessageExternalInterface::send_stop_service_http_request(comm, &req, ip, port, timeout);
    if( !obj->get_status())
    {
	    RecoveryManager recovery_mgr;
	    if(config_parser->account_updater_server_nameValue() + "-server" == std::string(service_name))
	    {
		    /* account-updater-server is in service_name, but command wants account-updater start */
		    service_name = std::string(config_parser->account_updater_server_nameValue()).c_str();
		    /* account-updater-server is in service_name, but command wants account-updater end */
	    }
	    OSDLOG(DEBUG, " Service: "<<service_name<< "stopped  forcefully obj status: " << obj->get_status());
	    recovery_mgr.stop_service(service_name);
    }
    OSDLOG(INFO, "Service: "<<service_name<<" stopped successfully");
}


bool MonitoringManager::system_stop_operation()
{
    this->update_table_in_case_stop_service();
    this->change_recovery_status(true);
    OSDLOG(INFO, "System stop operaton started");
    GLUtils utils;
    bool flag = true;
    boost::thread *thr[number_of_service];
    uint8_t count = 0;
    std::map < std::string, MonitorStatusMap > ::iterator it;
    for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
    {
        if( count > number_of_service)
        {
            OSDLOG(ERROR, "Number of running service is more then number of osd service");
            flag = false;
            break;
        }
        int32_t port = it->second.get_port();
        std::string ip = it->second.get_ip_addr();
        if( port == -1 && (ip.compare("") == 0))
            continue; 
        std::string service_name = this->get_service_name(it->first.c_str());
	//if(service_name == std::string("containerChild-server"))
	if(config_parser->containerChild_nameValue() + "-server" == std::string(service_name))
	{
        	OSDLOG(INFO, "skipping container child process");
		continue;
	}
        OSDLOG(INFO, "Send the http request to service :"<<service_name);
        thr[count] = new boost::thread(boost::bind(&MonitoringManager::send_http_requeset_for_service_stop, this, port, ip, 
                    service_name));
        count++;
    }
    uint8_t i = 0;
    for( ; i < count; i++)
    {
        if( thr[i]->joinable())
            thr[i]->timed_join(boost::posix_time::seconds(this->config_parser->service_stop_intervalValue()));
    }
    OSDLOG(INFO, "System stop operation successfully");
    RecoveryManager recovery_mgr;
    OSDLOG(INFO, "Try to stop the all osd services for surety");
    if(!recovery_mgr.stop_all_service())
    {
	    std::map < std::string, MonitorStatusMap > ::iterator it;
	    for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
	    {
		    std::string service_name = this->get_service_name(it->first.c_str());
		    service_name.erase (service_name.find_last_of('-'), service_name.size()-1);

		    recovery_mgr.kill_service(service_name);
	    }
    }
    return flag;
}

void MonitoringManager::kill_all_services()
{
	RecoveryManager recovery_mgr;
	std::map < std::string, MonitorStatusMap > ::iterator it;
	for(it = this->monitor_status_map.begin(); it != this->monitor_status_map.end(); it++)
	{
		std::string service_name = this->get_service_name(it->first.c_str());
		service_name.erase (service_name.find_last_of('-'), service_name.size()-1);

		recovery_mgr.kill_service(service_name);
	}
}
