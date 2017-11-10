#include "osm/localLeader/election_identifier.h"
#include <iostream>
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"


ElectionIdentifier::ElectionIdentifier(
				boost::shared_ptr<ElectionManager> mgr, 
				boost::shared_ptr<HfsChecker>checker,
			 	boost::shared_ptr<MonitoringManager> mon_mgr,
				cool::SharedPtr<config_file::OSMConfigFileParser>config_parser,
				enum network_messages::NodeStatusEnum &stat,
				boost::mutex &mtx
				):mtx(mtx),stat(stat)
{
	this->stop = false;
	this->elec_mgr = mgr;
	this->hfs_checker = checker;
	this->monitor_mgr = mon_mgr;
	this->config_parser = config_parser;
	this->count = 0;
	this->heartbeat_failure_count = 0;
	this->strm_failure_count = 0;
	this->failure = false;
	this->successer = false;
	this->conn_stat = OSDException::OSDConnStatus::CONNECTED;
}

ElectionIdentifier::~ElectionIdentifier()
{
	this->elec_mgr.reset();
	this->hfs_checker.reset();
	this->monitor_mgr.reset();
	this->config_parser.reset();
}

boost::tuple<boost::shared_ptr<Communication::SynchronousCommunication>  , int> ElectionIdentifier::send_strm_to_gl()
{
	OSDLOG(INFO, "Sending strm packet to GL for starting monitoring of LL");
	int retry_count = 0;
	boost::shared_ptr<Communication::SynchronousCommunication> comm;
	std::string node_id;
	GLUtils utils;
	node_id = utils.get_my_node_id() + "_" + 
					boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
	while( retry_count != this->config_parser->strm_countValue())
	{
		this->failure = false;
		OSDLOG(DEBUG, "Retry count for sending strm packet to GL: "<< retry_count);
		boost::shared_ptr<ErrorStatus> error_status;
		OsmUtils ut;
		boost::tuple<std::string, uint32_t> info = ut.get_ip_and_port_of_gl();
		if(info.get<1>() == 0) {
			PASSERT(false, "CORRUPTED ENTRIES IN GL INFO FILE");
		}

		OSDLOG(INFO, "Connecting to GL with ip and port: "<< boost::get<0>(info) << " " << boost::get<1>(info)); 
		comm.reset(new Communication::SynchronousCommunication( boost::get<0>(info), boost::get<1>(info)));
		if(!comm->is_connected())
		{
			OSDLOG(INFO, "Connection couldn't be established with GL");
			if(comm)
			{
				//delete comm;
				//comm = NULL;
				comm.reset();
			}
			//return boost::make_tuple( comm, OSDException::OSDErrorCode::UNABLE_RECV_ACK_FROM_GL);
		}
		else
		{
			boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_obj(
					new comm_messages::LocalLeaderStartMonitoring(node_id));
			OSDLOG(DEBUG, "Sending STRM for node " << node_id << " to GL.");
			boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(messageTypeEnum::typeEnums::LL_START_MONITORING));
			MessageExternalInterface::sync_send_local_leader_start_monitoring(
					strm_obj.get(),
					comm,
					boost::bind(&ElectionIdentifier::success_handler, this, result1),
					boost::bind(&ElectionIdentifier::failure_handler, this),
					30
					);
			if( !this->failure)
			{
				boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoringAck> >	ack_obj( 
						new MessageResult<comm_messages::LocalLeaderStartMonitoringAck>());
				ack_obj = MessageExternalInterface::sync_read_local_leader_start_monitoring_ack(comm, 30);		
				OSDLOG(DEBUG, "Rcvd STRM ack from GL, status is " << ack_obj->get_status());
				if( ack_obj->get_status())
				{
					this->count = 0;
					boost::shared_ptr<comm_messages::LocalLeaderStartMonitoringAck> ll_strm_ack;
					ll_strm_ack.reset(ack_obj->get_message_object());
					if(!ll_strm_ack->get_status())
					{
						OSDLOG(INFO, "STRM packet rejected by GL");
					}
					else
						return boost::make_tuple(comm, OSDException::OSDSuccessCode::RECV_ACK_FROM_GL);
				}
				else
				{
					OSDLOG(DEBUG, "Sending of strm packet to GL failed with return status: " << ack_obj->get_status()); 
				}
			}
			else
			{
				OSDLOG(DEBUG, "Unable to connect to GL");
			}
		}
		retry_count++;
		if( comm)
		{
			//delete comm;
			//comm = NULL;
			comm.reset();
		}
		boost::this_thread::sleep(boost::posix_time::seconds(this->config_parser->ll_strm_retry_timeoutValue()));
	}
	OSDLOG(INFO, "Unable to send start monitoring packet to GL");
	enum network_messages::NodeStatusEnum node_stat = utils.verify_if_part_of_cluster(node_id);
	if(node_stat == network_messages::NEW) /*|| node_stat == network_messages::FAILED)*/
	{
		OSDLOG(INFO, "Stopping osd services because LL is not registered");
		this->stop_all_service();
		return boost::make_tuple(comm, OSDException::OSDErrorCode::LL_IS_NOT_REGISTERED);
	}
	if( comm)
		//delete comm;
		comm.reset();
//	comm = NULL;
	return boost::make_tuple( comm, OSDException::OSDErrorCode::UNABLE_RECV_ACK_FROM_GL);
}

void ElectionIdentifier::success_handler(boost::shared_ptr<MessageResultWrap> result)
{
	this->successer = true;
}

void ElectionIdentifier::failure_handler()
{
	OSDLOG(INFO, "Couldn't get back strm/hrbt ack from GL");
	this->failure = true;
}

int ElectionIdentifier::send_heartbeat_to_gl(boost::shared_ptr<Communication::SynchronousCommunication> comm, uint32_t seq_no)
{
	boost::shared_ptr<ErrorStatus> error_status;
	GLUtils utils;
	std::string node_id = utils.get_my_node_id() + "_" + boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
        boost::shared_ptr<comm_messages::HeartBeat> heartbeat( new comm_messages::HeartBeat(node_id, seq_no,this->hfs_checker->get_gfs_stat()));
	boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(messageTypeEnum::typeEnums::HEART_BEAT));
	OSDLOG(DEBUG, "Sending HB to GL");
	MessageExternalInterface::sync_send_heartbeat( 
					heartbeat.get(),
					comm,
					boost::bind(&ElectionIdentifier::success_handler, this, result1),
					boost::bind(&ElectionIdentifier::failure_handler, this),
					30
					);
	if( !this->failure )
	{
		OSDLOG(DEBUG, "Sent HB to GL successfully");
		boost::shared_ptr<MessageResult<comm_messages::HeartBeatAck> > ack_obj( 
								new MessageResult<comm_messages::HeartBeatAck>());
		ack_obj = MessageExternalInterface::sync_read_heartbeatack(comm, 30);
		if( !ack_obj->get_status())
        	{
                	OSDLOG(INFO, "Failed to receive heartbeat ack from GL");
			this->conn_stat = OSDException::OSDConnStatus::DISCONNECTED;
                	return OSDException::OSDErrorCode::UNABLE_RECV_ACK_FROM_GL;
        	}
		else
		{
			OSDLOG(INFO, "Recvd heartbeat ack from gl successfully and node status as " << ack_obj->get_message_object()->get_node_stat());
			this->conn_stat = OSDException::OSDConnStatus::CONNECTED;

			{ /*mutex for local node status*/ 
				boost::mutex::scoped_lock lock(this->mtx);
				this->local_stat = this->stat;
			}
			OSDLOG(INFO, "Got local node_stat from GL on start:" << this->local_stat);
			if(this->local_stat != ack_obj->get_message_object()->get_node_stat()) { 

				if(ack_obj->get_message_object()->get_node_stat() == network_messages::RECOVERED || ack_obj->get_message_object()->get_node_stat() ==  network_messages::FAILED || ack_obj->get_message_object()->get_node_stat() == network_messages::NODE_STOPPED) {
					OSDLOG(INFO, "Stopping all services as got node stat as recovered or failed");
					this->stop_all_service();

				}
			}
			return OSDException::OSDSuccessCode::RECV_ACK_FROM_GL;
		}
	}
	else
		OSDLOG(DEBUG, "Unable to connect to GL for heartbeat request");
        this->conn_stat = OSDException::OSDConnStatus::DISCONNECTED;
	return OSDException::OSDErrorCode::UNABLE_RECV_ACK_FROM_GL;
}

int ElectionIdentifier::notify_to_election_manager_for_start_election(enum OSDException::OSDConnStatus::connectivity_stat conn_stat)
{
	cool::unused(conn_stat);
//	GLUtils utils;
	OsmUtils osm_utils;
	OSDLOG(INFO, "Notifing Election Manager to start election");
//	boost::tuple<std::string, bool> file = utils.find_file(
//					utils.get_mount_path(this->config_parser->export_pathValue(), "osd_meta_config"),
//					"osd_global_leader_information_");
//	OSDLOG(INFO, "Election version file: " << file);
//	std::string gl_info = osm_utils.split( boost::get<0>(file), "_" ) ; 
//	OSDLOG(INFO, "GL version is "<< gl_info);
	GLUtils util_obj;
	std::string gl_info = util_obj.read_GL_file();
	std::string gl_service_id = gl_info.substr(0, gl_info.find("\t"));
	return this->elec_mgr->start_election(gl_service_id);
}

void ElectionIdentifier::stop_election_identifier()
{
	this->stop = true;
}

enum OSDException::OSDHfsStatus::gfs_stat ElectionIdentifier::verify_gfs_stat()
{
	return this->hfs_checker->get_gfs_stat();
}

bool ElectionIdentifier::stop_all_service()
{
	bool failover = false;
	OSDLOG(INFO, "Stopping all osd services");
	this->monitor_mgr->stop_all_service();
	OSDLOG(INFO, "All osd services stopped successfully");
        OSDLOG(INFO, "Terminating the monitoring thread");
	this->monitor_mgr->stop_mon_thread();
	this->monitor_mgr->join();
	return true;
}

int ElectionIdentifier::stat_handler(enum OSDException::OSDHfsStatus::gfs_stat stat)
{
	int return_value = 0;
	OSDLOG(INFO, "Stat of gfs is = "<<stat);
	if( stat == OSDException::OSDHfsStatus::NOT_WRITABILITY )
	{
		if( this->count != this->config_parser->election_countValue())
		{
			OSDLOG(INFO, "LL gfs reader is unable to update the stat, failure count: " << this->count);
			this->count++ ;
			return 0;
		}
		return_value =  this->notify_to_election_manager_for_start_election(this->conn_stat);
		OSDLOG(DEBUG, "Election is completed with return value "<<return_value);
		this->count = 0;
		this->strm_failure_count = 0;
		return return_value;
	}
	this->count = 0;
	return return_value;
}

void ElectionIdentifier::start()
{
	OSDLOG(INFO, "Election Indentifier thread start");
	this->stop = false;
	try
	{
	for(;;)
	{
		if( this->stop)
			return;
		boost::this_thread::sleep(boost::posix_time::seconds(5));
		boost::this_thread::interruption_point();
		boost::tuple<boost::shared_ptr<Communication::SynchronousCommunication> , int> conn_info;
		conn_info = this->send_strm_to_gl();
		if( boost::get<1>(conn_info) == OSDException::OSDErrorCode::UNABLE_RECV_ACK_FROM_GL )
		{
			int ret = 0;
			this->strm_failure_count++;
			OSDLOG(DEBUG, "STRM ACK failure count: " << this->strm_failure_count);
			if(this->strm_failure_count == this->config_parser->hrbt_ack_failure_countValue() && ((this->verify_gfs_stat() == OSDException::OSDHfsStatus::WRITABILITY) || (this->verify_gfs_stat() == OSDException::OSDHfsStatus::NOT_READABILITY))) {
				OSDLOG(INFO, "Stopping all osd services asumming GL as isolated or LL has NOT_READABILITY");
				this->stop_all_service();
			}
			if( (ret = this->stat_handler(this->verify_gfs_stat())) != OSDException::OSDSuccessCode::ELECTION_SUCCESS)
				continue;	
			if( ret == OSDException::OSDSuccessCode::ELECTION_SUCCESS)
			{
				OSDLOG(DEBUG, "Sending the strm msg to newly elected GL"); 
			        this->strm_failure_count = 0;		
				if( boost::get<0>(conn_info))
				{
					//				delete boost::get<0>(conn_info);
					//				boost::get<0>(conn_info) = NULL;
					boost::get<0>(conn_info).reset();
				}
				continue;
			}
		}
		else if(boost::get<1>(conn_info) == OSDException::OSDErrorCode::LL_IS_NOT_REGISTERED)
		{
	//		OSDLOG(INFO,"Sleep before stopping for 90 seconds");
	//		boost::this_thread::sleep(boost::posix_time::seconds(90));
			this->stop = true;   // stopping election thread as LL not registered
			continue;
		}
		OSDLOG(INFO, "Sent strm packet to GL successfully");
		this->strm_failure_count = 0;
		uint32_t seq_no = 0;
		boost::shared_ptr<Communication::SynchronousCommunication> comm = boost::get<0>(conn_info);
		if( !comm)
		{
			OSDLOG(INFO, "Invalid connection object");
			continue;
		}	
		this->failure = false;
		this->successer = false;
		boost::this_thread::sleep(boost::posix_time::seconds(30));
		for(;;)
		{
			if( this->stop)
				return;
			boost::this_thread::sleep(boost::posix_time::seconds(2));
			boost::this_thread::interruption_point();
			time_t begin, end;
			time(&begin);
			seq_no = seq_no + 1; 
			int ret = this->send_heartbeat_to_gl(comm, seq_no);
			if( ret == OSDException::OSDErrorCode::UNABLE_RECV_ACK_FROM_GL)
			{
				this->heartbeat_failure_count++;
				OSDLOG(DEBUG, "Heartbeat ack failure count: " << this->heartbeat_failure_count);
				if(this->heartbeat_failure_count == this->config_parser->hrbt_ack_failure_countValue() && ((this->verify_gfs_stat() == OSDException::OSDHfsStatus::WRITABILITY) || (this->verify_gfs_stat() == OSDException::OSDHfsStatus::NOT_READABILITY))) {
					OSDLOG(INFO, "Stopping all osd services asumming GL as isolated or LL has NOT_READABILITY");
					this->stop_all_service();
				}
				boost::this_thread::sleep(boost::posix_time::seconds(30));
				break;
			}
			time(&end);
			uint64_t timeout = this->config_parser->gns_accessibility_intervalValue();
			if( (timeout - (end - begin)) > 0)
				boost::this_thread::sleep(boost::posix_time::seconds(timeout - (end - begin)));
			if( this->stat_handler(this->verify_gfs_stat()) == OSDException::OSDSuccessCode::ELECTION_SUCCESS)
			{
				OSDLOG(INFO, "Election completed successfully");
				this->heartbeat_failure_count = 0;
				this->strm_failure_count = 0;
				break;
			}
		}
		if( comm)
		{
			OSDLOG(DEBUG, "Deleting comm object");
		//	delete comm;
		//	comm = NULL;
			comm.reset();
		}
	}}
	catch (boost::thread_interrupted&)
	{
		OSDLOG(INFO, "Closing election thread");
	}	
}

