#include "osm/localLeader/local_leader.h"
#include "communication/message_type_enum.pb.h"
//using namespace election_identifier;

#include "osm/osmServer/osm_info_file.h"
#include "osm/osmServer/utils.h"

using namespace objectStorage;
namespace local_leader
{

LocalLeader::LocalLeader()
{
	this->if_cluster_part = false;
}

LocalLeader::LocalLeader( 
	enum network_messages::NodeStatusEnum stat, 
	boost::shared_ptr<common_parser::LLMsgProvider> ll_msg_provider,
	boost::shared_ptr<common_parser::LLNodeAddMsgProvider> na_msg,
	boost::shared_ptr<common_parser::MessageParser> parser,
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser,
	int32_t tcp_port
):
	osm_port(tcp_port)
{
	this->ll_msg_provider_obj = ll_msg_provider;	
	this->na_msg_provider = na_msg;
	this->parser_obj = parser;
	this->node_stat = network_messages::NEW;
	this->stop = false;
	GLUtils utils;
	this->config_parser = config_parser;
	this->hfs_checker.reset(new HfsChecker(
					this->config_parser));
	this->monitor_mgr.reset(new MonitoringManager(
					this->hfs_checker, 
					this->config_parser));
	this->election_mgr.reset(new ElectionManager(
					utils.get_my_service_id(tcp_port), 
					config_parser, this->hfs_checker));
	this->election_idf.reset(new ElectionIdentifier(
					this->election_mgr, 
					this->hfs_checker, 
					this->monitor_mgr, 
					this->config_parser,
					this->node_stat,
					this->mtx));
	this->msg_handler.reset(new ll_parser::LlMsgHandler(
					this->monitor_mgr, 
					this->election_mgr,
					this->config_parser));

	this->failure = false;
}

LocalLeader::~LocalLeader()
{
	this->terminate_all_active_thread();
	this->monitor_mgr.reset();
	this->msg_handler.reset();
	this->election_mgr.reset();
	this->ll_msg_provider_obj.reset();
	this->na_msg_provider.reset();
	this->parser_obj.reset();
	this->monitoring_thread.reset();
	this->hfs_checker_thread.reset();
	this->election_idf_thread.reset();
	this->config_parser.reset();
	//this->terminate_all_active_thread();
	this->io.stop();
}

void LocalLeader::start_timer()
{
	OSDLOG(INFO, " start_timer is called");
	this->io.reset();
	this->io.run();
	OSDLOG(INFO, "End of start_timer");
}

void LocalLeader::stop_operation(const boost::system::error_code &e)
{
	if( e == boost::asio::error::operation_aborted)
		return;
	OSDLOG(INFO, "Timer expire due to timeout");
	this->stop = true;
}

void LocalLeader::success_handler(boost::shared_ptr<MessageResultWrap> result)
{
	this->failure = false;
}

void LocalLeader::failure_handler()
{
        OSDLOG(INFO, "Couldn't acknowledge back NA gl");
	this->failure = true;
}

#ifdef DEBUG_MODE
bool LocalLeader::notify_gl(boost::shared_ptr<MessageCommunicationWrapper> msg_obj, bool is_all_service_started)
{
	return true;
}
#else
bool LocalLeader::notify_gl(boost::shared_ptr<MessageCommunicationWrapper> msg_obj, bool is_all_service_started)
{
	OSDLOG(INFO, "Send node addition ack to gl");
	bool flag = false;	
	boost::system::error_code ec; 
	boost::tuple<std::list<boost::shared_ptr<ServiceObject> >, bool> record;
	uint16_t timeout = this->config_parser->node_addition_response_intervalValue();
	boost::asio::deadline_timer timer(this->io, boost::posix_time::seconds(timeout));	
	timer.async_wait(boost::bind(&local_leader::LocalLeader::stop_operation, this, _1));
	while( !this->stop)
	{
		record = this->monitor_mgr->get_service_port_and_ip();
		if( boost::get<1>(record))
		{
			OSDLOG(INFO, "Get the all osd service port and ip address");	
			timer.cancel(ec);
			timer.expires_from_now(boost::posix_time::seconds(0), ec);
			this->io.stop();
			break;
		}
	}
	this->io.stop();
	boost::shared_ptr<ErrorStatus> error_status;
	if( !boost::get<1>(record) || !is_all_service_started)
	{
		OSDLOG(DEBUG, "send negative ack to gl for node addition");
		boost::get<0>(record).clear();
		error_status.reset( new ErrorStatus(OSDException::OSDErrorCode::NODE_ADDITION_FAILED, "Node addition failed"));
		monitor_mgr->change_recovery_status(true);
	}	
	else
	{
		OSDLOG(DEBUG, "Send success ack to gl for node addition");
		error_status.reset( new ErrorStatus(OSDException::OSDSuccessCode::NODE_ADDITION_SUCCESS, "Node addition success"));
		flag = true;
	}
	boost::shared_ptr<comm_messages::NodeAdditionGlAck> node_add_obj(new comm_messages::NodeAdditionGlAck(boost::get<0>(record),
										error_status));
	boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(messageTypeEnum::typeEnums::NODE_ADDITION_GL_ACK));
	MessageExternalInterface::sync_send_node_addition_ack_gl(
				node_add_obj.get(), 
				msg_obj->get_comm_object(),
				boost::bind(&LocalLeader::success_handler, this, result1),
	                        boost::bind(&LocalLeader::failure_handler, this)
				);

	if(this->failure == true)
	{
		return false;
		OSDLOG(INFO,"Failure handler executed from communication");
	}

	if(true == flag)
	{
		/*receive final ack, if received succesfully then return flag else make flag as false and return*/
		boost::shared_ptr<MessageResult<comm_messages::NodeAdditionFinalAck> >obj(
				new MessageResult<comm_messages::NodeAdditionFinalAck>);	

		obj = MessageExternalInterface::sync_read_node_add_final_ack_gl(msg_obj->get_comm_object(), 180);
		if(obj->get_return_code() == Communication::TIMEOUT_OCCURED)
		{   
			OSDLOG(ERROR, "Timeout occurred while rcving response from new GL");
			return false;
		} 
		bool status = obj->get_status();
		if( status)
		{
			OSDLOG(INFO, "Node addition final ack success");	
		}
		else
		{
			OSDLOG(INFO, "Node addition final ack failure");	
			return false;
		}
	}
	return flag;
}	
#endif

boost::shared_ptr<ll_parser::LlMsgHandler> LocalLeader::get_ll_msg_handler() const
{
	return this->msg_handler;
}

void LocalLeader::terminate_all_active_thread()
{
	OSDLOG(INFO, "Terminate the all active thread in local leader");
	OSDLOG(INFO, "Stop ll msg handler thread");
//#ifndef DEBUG_MODE
	this->msg_handler_thread->interrupt();
	this->msg_handler_thread->join();
	this->hfs_checker_thread->interrupt();
	this->hfs_checker_thread->join();
	if(monitor_mgr)
	{
		OSDLOG(INFO, "Stop the recover thread");
		this->monitor_mgr->stop_recovery_identifier();
	}
	OSDLOG(INFO, "Stop monintoring threa");
	this->monitoring_thread->interrupt();
	this->monitoring_thread->join();
	//this->msg_handler_thread->join();
	if( this->election_idf_thread)
	{
		OSDLOG(INFO, "Stop the election thread");
		this->election_idf_thread->interrupt();
		this->election_idf_thread->join();
		this->election_idf_thread.reset();
	}
//#endif
	this->msg_handler_thread.reset();
	this->hfs_checker_thread.reset();
	this->monitoring_thread.reset();
	this->hfs_checker_thread.reset();
}

void LocalLeader::start()
{
	OSDLOG(INFO, "Local leader main thread start");
	this->msg_handler_thread.reset(new boost::thread(&ll_parser::LlMsgHandler::start, this->msg_handler));
	this->monitoring_thread.reset(new boost::thread(boost::bind(&MonitoringManager::start, this->monitor_mgr)));
	this->hfs_checker_thread.reset(new boost::thread(boost::bind(&HfsChecker::start, this->hfs_checker)));
	int retry_count =0;
	bool transition_state_flag = false;
	std::string node_id;
	GLUtils utils_obj;
	node_id = utils_obj.get_my_node_id() + "_" +
	                                        boost::lexical_cast<std::string>(this->config_parser->osm_portValue());

	enum network_messages::NodeStatusEnum node_stat_cluster = utils_obj.verify_if_part_of_cluster(node_id);
	if(node_stat_cluster == network_messages::NEW) {
		 OSDLOG(INFO, "Setting Status as NEW as not part of cluster");
		 boost::mutex::scoped_lock lock(this->mtx);
		this->node_stat = network_messages::NEW;
	}else {
		if(election_idf_thread == NULL || election_idf_thread->timed_join(boost::posix_time::seconds(0))) {
			OSDLOG(DEBUG, "Starting election identifier thread");
			this->election_idf_thread.reset(
					new boost::thread(boost::bind(&ElectionIdentifier::start, this->election_idf)));
		}
		OsmUtils osm_obj;
		while(1) {

			int sleep_interval_time = this->config_parser->node_status_time_intervalValue();
			boost::tuple<std::string, uint32_t> gl_info = osm_obj.get_ip_and_port_of_gl(); 	
			boost::shared_ptr<Communication::SynchronousCommunication> comm_obj;
			if(gl_info.get<1>() == 0) {
				PASSERT(false, "CORRUPTED ENTRIES IN GL INFO FILE");
			}
			comm_obj.reset(new Communication::SynchronousCommunication(gl_info.get<0>(), gl_info.get<1>()));
			if(!comm_obj) {
				OSDLOG(INFO, "Invalid comm. object");
				continue;
			}
			if(!comm_obj->is_connected()) {
				OSDLOG(INFO, "connection couldn't established with GL");
				/* As GL is not present in the system, it sleeps for 20 secs and retry again for connection */
				boost::this_thread::sleep(boost::posix_time::seconds(sleep_interval_time));
				continue;
			}
			boost::shared_ptr<ServiceObject> node_info( new ServiceObject());
			std::string node_id = utils_obj.get_my_node_id() + "_" +
				boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
			char hostname[255];
			memset(hostname, 0, sizeof(hostname));
			::gethostname(hostname, sizeof(hostname));
			node_info->set_service_id(node_id);
			node_info->set_ip(utils_obj.get_node_ip(hostname));
			node_info->set_port(this->config_parser->osm_portValue());

			boost::shared_ptr<comm_messages::NodeStatus> node_obj(
					new comm_messages::NodeStatus(node_info));

			boost::shared_ptr<MessageResultWrap> result1(
					new MessageResultWrap(messageTypeEnum::typeEnums::NODE_STATUS));

			MessageExternalInterface::sync_send_node_status_to_gl(
					node_obj.get(),
					comm_obj,
					boost::bind(&LocalLeader::success_handler, this, result1),
					boost::bind(&LocalLeader::failure_handler, this),
					30);
					
			if(this->failure)
			{
				OSDLOG(INFO, "Unable to send node status msg on startup to GL");
				boost::this_thread::sleep(boost::posix_time::seconds(sleep_interval_time));
				/* If Unable to send node status to gl on startup then continue to while loop
				   after waiting for 20 secs */
				continue;
			}
			OSDLOG(INFO, "Wait for node status ack from GL on startup");

			boost::shared_ptr<MessageResult<comm_messages::NodeStatusAck> > ack_obj(
					new MessageResult<comm_messages::NodeStatusAck>);

			ack_obj = MessageExternalInterface::sync_read_node_status_ack(comm_obj);

			bool retValue = ack_obj->get_status();
			if(!retValue)
			{
				OSDLOG(INFO, "Unable to get node status ack from GL on startup");
			}else {

				enum network_messages::NodeStatusEnum node_status = ack_obj->get_message_object()->get_status();
				OSDLOG(INFO, "Got node status ack on startup from gl: " << node_status);
				boost::mutex::scoped_lock lock(this->mtx);
				this->node_stat = node_status;
				break;
			}
		}
	}

	try{
		for(;;)
		{		
			this->stop = false;
			bool m_term_flag = false;
			boost::this_thread::interruption_point();
			boost::shared_ptr<ServiceStartupManager> startup_mgr(new ServiceStartupManager(this->monitor_mgr,
						this->node_stat, this->hfs_checker, this->config_parser));
			boost::shared_ptr<ll_parser::LlMsgHandler> ll_msg_ptr(this->get_ll_msg_handler());
			boost::mutex::scoped_lock lock(this->mtx);
			enum network_messages::NodeStatusEnum local_stat = this->node_stat;
			this->mtx.unlock();

			switch(local_stat) 
			{
				case network_messages::RECOVERED : // fallthrough
				case network_messages::NODE_STOPPED :
				{	
					OSDLOG(INFO, "local node_stat= " << local_stat);
					if(election_idf_thread == NULL || election_idf_thread->timed_join(boost::posix_time::seconds(0)))
					{
						OSDLOG(DEBUG, "Starting election identifier thread");
						this->election_idf_thread.reset(
								new boost::thread(boost::bind(&ElectionIdentifier::start, this->election_idf)));
					}
					/*if(retry_count >= 3)
					{
						break;
					}*/
					while(this->hfs_checker->get_gfs_stat() == OSDException::OSDHfsStatus::NOT_WRITABILITY)
					{
						/*If GL is down at the time of recovery with two nodes this will stuck infinetly*/
					}
					OsmUtils obj;
					GLUtils utils;
					std::string node_id = utils.get_my_node_id() + "_" +
                                       		boost::lexical_cast<std::string>(this->osm_port);
					std::string node_name = utils.get_my_node_id();	
					std::string node_ip = utils.get_node_ip(node_name);

					boost::tuple<std::string, uint32_t> gl_in = obj.get_ip_and_port_of_gl();
				
					if(gl_in.get<1>() == 0) {
						PASSERT(false, "CORRUPTED ENTRIES IN GL INFO FILE");
					}

					boost::shared_ptr<Communication::SynchronousCommunication> comm_obj;
						comm_obj.reset(new Communication::SynchronousCommunication(gl_in.get<0>(), gl_in.get<1>()));							
					boost::shared_ptr<comm_messages::NodeRejoinAfterRecovery> node_rej(
								new comm_messages::NodeRejoinAfterRecovery(node_id, node_ip, this->osm_port));
		
					boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(
									messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY));

						MessageExternalInterface::sync_send_node_rejoin_recovery_msg_gl(
								node_rej.get(),
								comm_obj,
								boost::bind(&LocalLeader::success_handler, this, result1),
								boost::bind(&LocalLeader::failure_handler, this)			 
								);				


						if(!this->failure) {
							OSDLOG(DEBUG, "Recvd node rejoin ack from gl successfully");

							if(this->msg_handler->non_blocking_node_add_verify() == false) {
								// continue to outer for loop 
								continue;
							}


						}else {
							OSDLOG(DEBUG, "Unable to connect to GL for Node Rejoin Recovery");
							continue;
						}		
					}
					//fallthrough
				case network_messages::TRANSITION_STATE: //fallthrough
				case network_messages::NEW : //fallthrough
				case network_messages::RETIRING : 
					{
						/* STOPPED needs also be added in below check later*/
						if(local_stat == network_messages::NEW || local_stat == network_messages::RETIRING || local_stat == network_messages::TRANSITION_STATE)
						{
							this->msg_handler->verify_node_addition_msg_recv(); 
						}

					this->monitor_mgr->change_recovery_status(false);
					this->monitor_mgr->enable_mon_thread();
					if(!ll_msg_ptr->get_ring_file_error())
					{
						//ring file could not be created, send negative acknowledgement to GL
						this->notify_gl(this->msg_handler->get_gl_channel(), false);
						break;
					}
				}
					//fallthrough
				case network_messages::REGISTERED : //fallthrough 
				{
					OSDLOG(INFO, "local node_stat= " << local_stat);
					if( (election_idf_thread == NULL || election_idf_thread->timed_join(boost::posix_time::seconds(0)))
						&& local_stat == network_messages::REGISTERED)	{
						 OSDLOG(DEBUG, "Starting election identifier thread");
			 			 this->election_idf_thread.reset(new boost::thread(boost::bind(&ElectionIdentifier::start, this->election_idf)));
					}
					this->monitor_mgr->change_recovery_status(false);
					this->monitor_mgr->enable_mon_thread();
					this->ll_msg_provider_obj->register_msgs(this->parser_obj, ll_msg_ptr);
					// Below recovery thread == MonitoringManager::service_monitoring 
					this->monitor_mgr->start_recovery_identifier();
					//this->service_startup_thread.reset(
					 // new boost::thread(boost::bind(&ServiceStartupManager::start, startup_mgr)));
					 // this->service_startup_thread->join(); 
					startup_mgr->start();
					bool is_all_service_started = false;
					is_all_service_started = startup_mgr->verify_all_service_started();
					if( is_all_service_started ) 
					{
						GLUtils gl_util;
						CLOG(SYSMSG::OSD_SERVICE_START, gl_util.get_my_node_id().c_str());
						this->msg_handler->update_local_leader_status(network_messages::RUNNING);
						if(local_stat == network_messages::NEW || 
									local_stat == network_messages::RECOVERED
									 ||  local_stat == network_messages::NODE_STOPPED || 
									 local_stat == network_messages::TRANSITION_STATE)
						{
							this->io.stop();
							boost::scoped_ptr<boost::thread>timer_thread(
									new boost::thread(&local_leader::LocalLeader::start_timer, this));
							m_term_flag = this->notify_gl(this->msg_handler->get_gl_channel(), is_all_service_started); 
						}
						else 
							m_term_flag = true;
					}
					else if(local_stat == network_messages::NEW ||
									local_stat == network_messages::RECOVERED
									 ||  local_stat == network_messages::NODE_STOPPED || 
									 local_stat == network_messages::TRANSITION_STATE
									 )
					{
						this->io.stop();
						boost::scoped_ptr<boost::thread>timer_thread(
								new boost::thread(&local_leader::LocalLeader::start_timer, this));
						this->notify_gl(this->msg_handler->get_gl_channel(), is_all_service_started);
					}else if(local_stat == network_messages::REGISTERED)
					{
						// Services couldn't be started and notifying GL for failure 
						this->monitor_mgr->notify_to_gl();
					}
						
					// No fallthrough 
					break;
				}
				case network_messages::NODE_STOPPING : //fallthrough to failed state
				case network_messages::FAILED : //fallthrough
					{
						OSDLOG(INFO, "local node_stat= " << local_stat);
						while(1) 
						{
							if(election_idf_thread == NULL || election_idf_thread->timed_join(boost::posix_time::seconds(0)))
							{
								OSDLOG(DEBUG, "Starting election identifier thread");
								this->election_idf_thread.reset(
										new boost::thread(boost::bind(&ElectionIdentifier::start, this->election_idf)));
							}
							OsmUtils obj;
							GLUtils utils;

							boost::tuple<std::string, uint32_t> gl_in = obj.get_ip_and_port_of_gl();

							boost::shared_ptr<Communication::SynchronousCommunication> comm_obj;
							if(gl_in.get<1>() == 0) {
								PASSERT(false, "CORRUPTED ENTRIES IN GL INFO FILE");
							}
							comm_obj.reset(new Communication::SynchronousCommunication(gl_in.get<0>(), gl_in.get<1>())); 	
							OSDLOG(INFO, "communicate to gl successfully");
							
							boost::shared_ptr<ServiceObject> node_info( new ServiceObject());
		                                        
                                    			std::string node_id = utils.get_my_node_id() + "_" +
                                                        boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
                                	        	char hostname[255];
                                        		memset(hostname, 0, sizeof(hostname));
                                        		::gethostname(hostname, sizeof(hostname));
                                        		node_info->set_service_id(node_id);
                                       			node_info->set_ip(utils.get_node_ip(hostname));
                                       			node_info->set_port(this->config_parser->osm_portValue());


							boost::shared_ptr<comm_messages::NodeStatus> node_obj(
									new comm_messages::NodeStatus(node_info));

							boost::shared_ptr<MessageResultWrap> result1(
									new MessageResultWrap(messageTypeEnum::typeEnums::NODE_STATUS));

							MessageExternalInterface::sync_send_node_status_to_gl(
									node_obj.get(),
									comm_obj,
									boost::bind(&LocalLeader::success_handler, this, result1),
									boost::bind(&LocalLeader::failure_handler, this),
									30);
							int sleep_interval = this->config_parser->node_status_query_intervalValue();
							if(this->failure) 
							{
								OSDLOG(INFO, "Unable to send node status msg to GL");
								//boost::this_thread::sleep(boost::posix_time::seconds(120));
								boost::this_thread::sleep(boost::posix_time::seconds(sleep_interval));
								/* If Unable to send node status to gl then continue to while loop
								   after waiting for 120 secs */
								continue;
							}
							OSDLOG(INFO, "Wait for node status ack from GL");

							boost::shared_ptr<MessageResult<comm_messages::NodeStatusAck> > ack_obj(
									new MessageResult<comm_messages::NodeStatusAck>);

							ack_obj = MessageExternalInterface::sync_read_node_status_ack(comm_obj);


							bool retValue = ack_obj->get_status();
							if(!retValue)
							{	
								OSDLOG(INFO, "Unable to get node status ack from GL");
							}
							else
							{
								enum network_messages::NodeStatusEnum node_status = ack_obj->get_message_object()->get_status();	
								OSDLOG(INFO, "Got node status ack from gl: " << node_status);

								if(node_status == network_messages::RECOVERED || node_status == network_messages::NODE_STOPPED)
								{
									/* if node status is recovered then break while loop and
									   continues to outer for loop */
									boost::mutex::scoped_lock lock(this->mtx);
									this->node_stat = network_messages::RECOVERED;
									break;
								}
								else if(node_status == network_messages::INVALID_NODE)
								{
									OSDLOG(INFO,"Node is not in cluster, set local status as NEW");										
									boost::mutex::scoped_lock lock(this->mtx);
									this->node_stat = network_messages::NEW;
									break;
								}
								else if(node_status == network_messages::TRANSITION_STATE)
								{
									
									OSDLOG(INFO,"cluster recovery is in progress, set local status as NEW");
									boost::mutex::scoped_lock lock(this->mtx);
									this->node_stat = network_messages::NEW;
									transition_state_flag = true;
									break; 
								
								}
								else if(node_status == network_messages::REGISTERED)
								{
									OSDLOG(INFO, "Local status is FAILED and REGISTERED at GL,");
									OSDLOG(INFO, "Moving to REGISTERED, node_stat= "<< node_status);
									boost::mutex::scoped_lock lock(this->mtx);
									this->node_stat = network_messages::REGISTERED;
									break;
								}

							}
							boost::this_thread::sleep(boost::posix_time::seconds(sleep_interval));
						} /* while(1) ends */
						/*  continue to outer for loop */
						continue; 
					}
				case network_messages::RETIRING_FAILED : //fallthrough
				case network_messages::RETIRED_RECOVERED : //fallthrough 
				case network_messages::RETIRED : 
				{
					continue;
				}
				default :
				{
					OSDLOG(FATAL,"unhandled node stat : " << this->node_stat);
					break;
				}
			}
			if( m_term_flag ) 
			{
				this->msg_handler->node_addition_completed();
				this->ll_msg_provider_obj->register_msgs(this->parser_obj, ll_msg_ptr);
#ifndef DEBUG_MODE		
				{/* block for registered state created */
					boost::mutex::scoped_lock lock(this->mtx);
					this->node_stat = network_messages::REGISTERED;
					this->mtx.unlock();
				}
				
				if(election_idf_thread == NULL || election_idf_thread->timed_join(boost::posix_time::seconds(0)))
				{
					this->election_idf_thread.reset(
							new boost::thread(boost::bind(&ElectionIdentifier::start, this->election_idf)));
				}
#endif
				OSDLOG(INFO, "Local leader wait for monitoring thread termination");
				this->monitor_mgr->join();
				this->msg_handler->update_local_leader_status(network_messages::STOPPING);
				startup_mgr->stop_all_services();
				this->msg_handler->update_local_leader_status(network_messages::STOP);	
				/*
				if(true == this->monitor_mgr->if_notify_to_gl_success())
				{
					// notified succesfully to GL regarding node failover,Thus stopping election thread 
					this->election_idf->stop_election_identifier();
					this->election_idf_thread->join();
					this->election_idf_thread.reset();
				}
				*/
				boost::mutex::scoped_lock lock(this->mtx);
				this->node_stat = network_messages::FAILED;
				OSDLOG(DEBUG, "node_stat changed to :" << this->node_stat);
				this->mtx.unlock();
			
#ifdef DEBUG_MODE
				startup_mgr.reset();
				return;
#else
				//this->election_idf->stop_election_identifier();
				//this->election_idf_thread->join();
				//this->election_idf_thread.reset();
#endif
				startup_mgr.reset();
				continue;
			}
			startup_mgr->stop_all_services();
			this->msg_handler->node_addition_completed();	
			this->na_msg_provider->register_msgs(this->parser_obj, ll_msg_ptr);	
			/*
			if(this->node_stat == network_messages::RECOVERED ||
                               this->node_stat == network_messages::REGISTERED || 
					this->node_stat == network_messages::NODE_STOPPED)
			{
				this->election_idf->stop_election_identifier();
				this->election_idf_thread->join();
				this->election_idf_thread.reset();
			}
			*/
			{/* block for mutex scop */
				boost::mutex::scoped_lock lock(this->mtx);
				/* local stat for comparison */
				if(this->node_stat != network_messages::NEW)
				{
					this->node_stat = network_messages::FAILED;

				}else if(this->node_stat == network_messages::NEW) {
					/* local stat for comparison */
					if(transition_state_flag == true) {
						this->node_stat = network_messages::FAILED;
						OSDLOG(INFO, "Transition state flag is true change node_stat changed to :" << this->node_stat);
						transition_state_flag = false;
					}
				}
			} /* block for mutex scope ends */
			this->monitor_mgr->stop_mon_thread();
			this->monitor_mgr->join();
			startup_mgr.reset();
#ifdef DEBUG_MODE
			return;
#endif
		}
	}catch(const boost::thread_interrupted&)
	{
		OSDLOG(INFO, "Closing Local local leader thread");
	}

}
} //namespace local_leader 
