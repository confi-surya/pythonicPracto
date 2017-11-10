#include "osm/localLeader/ll_msg_handler.h"
#include "osm/osmServer/msg_handler.h"
#include "communication/message_type_enum.pb.h"

namespace ll_parser
{

LlMsgHandler::LlMsgHandler(
				boost::shared_ptr<MonitoringManager> mtr_mgr, boost::shared_ptr<ElectionManager> mgr,
				cool::SharedPtr<config_file::OSMConfigFileParser>config_parser )
{
	this->ring_file_error = false;
	this->monitor_mgr = mtr_mgr;
	this->elec_mgr  = mgr;
	this->config_parser = config_parser;
	this->node_addition_msg = false;
	this->stop = false;
	this->is_osd_services_stop = false;
	this->io_service.stop();
	this->node_status = network_messages::IN_LOCAL_NODE_REC;
}

LlMsgHandler::~LlMsgHandler()
{
	this->io_service.stop();
	this->elec_mgr.reset();
	this->monitor_mgr.reset();
}

bool LlMsgHandler::non_blocking_node_add_verify()
{
        if (!this->node_addition_msg)
        {
                /*sleep for 3 second*/
                boost::this_thread::sleep(boost::posix_time::seconds(5));
                return false;
        }
        OSDLOG(INFO, "Node Addition Message Received" );
        return true;
}

void LlMsgHandler::verify_node_addition_msg_recv()
{
#ifndef DEBUG_MODE
	boost::mutex::scoped_lock lock(this->node_mtx);
	while (!this->node_addition_msg) 
		this->node_condition.wait(lock);	
	OSDLOG(DEBUG, "Node Addition Message Received" );
#endif
}

bool LlMsgHandler::get_ring_file_error() const
{
	return this->ring_file_error;
}
void LlMsgHandler::node_addition_completed()
{
	this->node_addition_msg  = false;
}

boost::shared_ptr<MessageCommunicationWrapper> LlMsgHandler::get_gl_channel() const
{
	return this->msg_comm_obj;
}

void LlMsgHandler::push(boost::shared_ptr<MessageCommunicationWrapper> msg)
{
        boost::mutex::scoped_lock lock(mtx);
        this->ll_msg_queue.push(msg);
        this->condition.notify_one();
}

void LlMsgHandler::success_handler(boost::shared_ptr<MessageResultWrap> result)
{
}

void LlMsgHandler::failure_handler()
{
	OSDLOG(INFO, "Couldn't acknowledge back NA gl");
}

void LlMsgHandler::start_io_service()
{
	OSDLOG(INFO, "Start the io service");
	this->io_service.run();
	OSDLOG(INFO, "End the io service"); 
}

boost::shared_ptr<MessageCommunicationWrapper> LlMsgHandler::pop()
{
        boost::mutex::scoped_lock lock(this->mtx);
        while(this->ll_msg_queue.empty() and !this->stop)
        {
                this->condition.wait(lock);
        }
        boost::shared_ptr<MessageCommunicationWrapper> msg = this->ll_msg_queue.front();
        this->ll_msg_queue.pop();
        return msg;
}

void LlMsgHandler::update_local_leader_status(enum network_messages::NodeStatusEnum status)
{
	 this->node_status = status;
}

network_messages::NodeStatusEnum LlMsgHandler::get_local_leader_status() const
{
	return this->node_status;
}

void LlMsgHandler::node_addition_msg_rcv(boost::shared_ptr<MessageCommunicationWrapper> comm_obj)
{
	boost::mutex::scoped_lock lock(node_mtx);
	this->node_addition_msg = true;
	this->msg_comm_obj = comm_obj;
	this->node_condition.notify_one();
}

void LlMsgHandler::start() 
{
	OSDLOG(INFO, "ll msg handler thread start");
	for(;;)
	{
		boost::this_thread::interruption_point();
		boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj = this->pop();
		if (!msg_comm_obj)
			break;
		const uint32_t msg_type = msg_comm_obj->get_msg_result_wrap()->get_type();
		OSDLOG(DEBUG, "Msg type rcvd is: " << msg_type);
		switch(msg_type)
		{	
			case messageTypeEnum::typeEnums::NODE_ADDITION_GL:
				OSDLOG(DEBUG, "Handling NA msg rcvd from GL");
				{
					boost::shared_ptr<ring_file_manager::RingFileWriteHandler> ring_ptr(new ring_file_manager::RingFileWriteHandler(CONFIG_FILESYSTEM));
					this->ring_file_error = ring_ptr->remakering();
					boost::shared_ptr<comm_messages::NodeAdditionGl> node_add_obj(
										new comm_messages::NodeAdditionGl);
					node_add_obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					this->node_addition_msg_rcv(msg_comm_obj);
				}
				break;	
			case messageTypeEnum::typeEnums::OSD_START_MONITORING:
				{
					boost::shared_ptr<comm_messages::OsdStartMonitoring> strm_obj(
										new comm_messages::OsdStartMonitoring);
					strm_obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					this->monitor_mgr->start_monitoring( 
									msg_comm_obj->get_comm_object(), 
									msg_comm_obj->get_comm_object()->get_fd(), 
									strm_obj->get_service_id(),
									strm_obj->get_port(),
									strm_obj->get_ip()
									);
					OSDLOG(DEBUG, "OSD_START_MONITORING msg recvd from osd service : " << strm_obj->get_service_id());
					OSDLOG(DEBUG, "Start Monitoring : service ip = " << strm_obj->get_ip()
						<< ", service port =" << strm_obj->get_port() << ".");
				}
				break;
			case  messageTypeEnum::typeEnums::NODE_STOP_CLI:
				OSDLOG(DEBUG, "Handling osd node stop cli msg from cli user");
				{
					boost::shared_ptr<ErrorStatus> error_status;
					this->is_osd_services_stop = false;
					this->update_local_leader_status(network_messages::STOPPING);
					boost::shared_ptr<comm_messages::NodeStopCli> cli_obj(
										new comm_messages::NodeStopCli);
					cli_obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					OSDLOG(DEBUG, "Node id send from cli user is: "<<cli_obj->get_node_id());
					OSDLOG(DEBUG, "Send node retire request ll to gl");
					boost::shared_ptr<ServiceObject> node_info( new ServiceObject());
					GLUtils utils;
					std::string node_id = utils.get_my_node_id() + "_" + 
							boost::lexical_cast<std::string>(this->config_parser->osm_portValue());
					char hostname[255];
					memset(hostname, 0, sizeof(hostname));
					::gethostname(hostname, sizeof(hostname));
					node_info->set_service_id(node_id);
					node_info->set_ip(utils.get_node_ip(hostname));
					node_info->set_port(this->config_parser->osm_portValue());
					OsmUtils osm_ut;
					boost::tuple<std::string, uint32_t> gl_info = osm_ut.get_ip_and_port_of_gl();
					OSDLOG(INFO, "Connect to gl with ip and port: "<<boost::get<0>(gl_info)<<"  "<<boost::get<1>(gl_info));				

					if(gl_info.get<1>() == 0) {
						  PASSERT(false, "CORRUPTED ENTRIES IN GL INFO FILE");
					}

					boost::shared_ptr<Communication::SynchronousCommunication> comm( 
							new Communication::SynchronousCommunication(boost::get<0>(gl_info),
							boost::get<1>(gl_info)));
					/* check for communication is needed */
					// New Msg NodeStopLL is added 
					boost::shared_ptr<comm_messages::NodeStopLL> node_stop_obj(
										new comm_messages::NodeStopLL(node_info));
					boost::shared_ptr<MessageResultWrap> result1(
							new MessageResultWrap(messageTypeEnum::typeEnums::NODE_STOP_LL));
					/* block starts */
					if(!comm->is_connected())
					{
						OSDLOG(INFO, "Send the negative ack to cli user");	
						string msg = "Node stop failed";
						error_status.reset( 
								new ErrorStatus(OSDException::OSDErrorCode::NODE_STOP_CLI_FAILED, msg));
						result1.reset();
						boost::shared_ptr<comm_messages::NodeStopCliAck> cli_ack(
								new comm_messages::NodeStopCliAck(error_status));
						result1.reset(new MessageResultWrap(messageTypeEnum::typeEnums::NODE_STOP_CLI_ACK));
						MessageExternalInterface::sync_send_node_stop_ack(
								cli_ack.get(),
								msg_comm_obj->get_comm_object(),
								boost::bind(&LlMsgHandler::success_handler, this, result1),
								boost::bind(&LlMsgHandler::failure_handler, this)
								);
						continue;
					}
					/* block ends */
					/// sending node stop ll  msg to gl
					MessageExternalInterface::sync_send_node_stop_to_gl(
							node_stop_obj.get(),
                                        		comm,
                                        		boost::bind(&LlMsgHandler::success_handler, this, result1),
                                        		boost::bind(&LlMsgHandler::failure_handler, this)
					);
					OSDLOG(INFO, "Wait for node stop ack from gl to ll");
					boost::shared_ptr<MessageResult<comm_messages::NodeStopLLAck> >obj(
									new MessageResult<comm_messages::NodeStopLLAck>);	
					
					obj = MessageExternalInterface::sync_read_node_stop_ack_gl(comm, 60);
					OSDLOG(DEBUG, "Send the ack to cli user");
					std::string msg = "";
					bool status = obj->get_status();
					OSDLOG(INFO, "Recvd the node stop ll ack from gl");
					if( !status)
					{
						OSDLOG(INFO, "Send the negative ack to cli user");	
						msg = "Node stop failed";
						error_status.reset( 
							new ErrorStatus(OSDException::OSDErrorCode::NODE_STOP_CLI_FAILED, msg));
					}
					else
					{
						
						enum network_messages::NodeStatusEnum node_status = static_cast<enum network_messages::NodeStatusEnum> (obj->get_message_object()->get_node_status());
						if(node_status == network_messages::RETIRING) {
							OSDLOG(INFO, "Send the negative ack to cli user when  node status is RETIRING");
							msg = "Node stop retiring failed";
							error_status.reset(
									new ErrorStatus(OSDException::OSDErrorCode::NODE_STOP_CLI_RETIRING_FAILED, msg));
						}else { 
							OSDLOG(INFO, "Send the positive ack to cli user");
							msg = "Node stop success";
							error_status.reset(
									new ErrorStatus(OSDException::OSDSuccessCode::NODE_STOP_CLI_SUCCESS, msg));
						}
					}
					result1.reset();
					boost::shared_ptr<comm_messages::NodeStopCliAck> cli_ack(
							new comm_messages::NodeStopCliAck(error_status));
					result1.reset(new MessageResultWrap(messageTypeEnum::typeEnums::NODE_STOP_CLI_ACK));
					MessageExternalInterface::sync_send_node_stop_ack(
							cli_ack.get(),
							msg_comm_obj->get_comm_object(),
							boost::bind(&LlMsgHandler::success_handler, this, result1),
							boost::bind(&LlMsgHandler::failure_handler, this)
							);
					/*
					OSDLOG(INFO, "Wait for service request come from gl side to ll")
					boost::asio::io_service io_service;
					this->io_service.stop();
					boost::scoped_ptr<boost::thread>timer_thread(
						new boost::thread(boost::bind(&LlMsgHandler::start_io_service, this)));
					boost::asio::deadline_timer timer(io_service);
					uint32_t timeout = this->config_parser->service_stop_intervalValue();
					timer.expires_from_now(boost::posix_time::seconds(timeout));
					timer.wait();
					io_service.stop();
					if( !this->is_osd_services_stop)
					{
						OSDLOG(DEBUG, "Stop the all osd service");
						this->monitor_mgr->stop_all_service();
						this->update_local_leader_status(network_messages::STOP);
					}
					*/
				}
				break;
			case  messageTypeEnum::typeEnums::NODE_SYSTEM_STOP_CLI:
				OSDLOG(DEBUG, "Handling node stop system cli msg from cli user")
				{
					boost::shared_ptr<ErrorStatus> error_status;
					this->update_local_leader_status(network_messages::STOPPING);
					boost::shared_ptr<comm_messages::NodeSystemStopCli> obj(
										new comm_messages::NodeSystemStopCli);
					obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					std::string msg = "";	
					this->monitor_mgr->system_stop_operation();
					this->update_local_leader_status(network_messages::STOP); 
				}
				break;
			case messageTypeEnum::typeEnums::LOCAL_NODE_STATUS:
				OSDLOG(DEBUG, "Handling local node status msg")
				{
					boost::shared_ptr<ErrorStatus> error_status;
					boost::shared_ptr<comm_messages::LocalNodeStatus> obj(
										new comm_messages::LocalNodeStatus);
					obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					OSDLOG(DEBUG, "Sending local node status ack : " << this->get_local_leader_status());
					boost::shared_ptr<comm_messages::NodeStatusAck> ack_obj(
							new comm_messages::NodeStatusAck( this->get_local_leader_status()));
					boost::shared_ptr<MessageResultWrap> result1(
							new MessageResultWrap(messageTypeEnum::typeEnums::NODE_STATUS_ACK));
					OSDLOG(DEBUG, "Send node status ack to cli user");
					MessageExternalInterface::sync_send_node_status_ack(
							ack_obj.get(),
							msg_comm_obj->get_comm_object(),
							boost::bind(&LlMsgHandler::success_handler, this, result1),
							boost::bind(&LlMsgHandler::failure_handler, this),
							30
							);
				}
				break;				
			case messageTypeEnum::typeEnums::STOP_SERVICES:
				OSDLOG(DEBUG, "Handling the stop service request")
				{
					boost::shared_ptr<ErrorStatus> error_status;
					boost::shared_ptr<comm_messages::StopServices> obj(
                                                                                new comm_messages::StopServices);
					obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					std::string msg = "";
					this->update_local_leader_status(network_messages::STOPPING);
					if( this->monitor_mgr->stop_all_service())
					{
						OSDLOG(DEBUG, "Send the postive ack to gl");
						msg = "Stop service success";
						error_status.reset( 
							new ErrorStatus(OSDException::OSDSuccessCode::STOP_SERVICE_SUCCESS, msg ));
						this->is_osd_services_stop = true;
					}
					else
					{
						OSDLOG(DEBUG, "Send the negative ack to gl");
						msg = "Stop Service failed";
						error_status.reset( 
							new ErrorStatus(OSDException::OSDErrorCode::STOP_SERVICE_FAILED, msg));
						this->is_osd_services_stop = false;
					}
					this->update_local_leader_status(network_messages::STOP);
					boost::shared_ptr<comm_messages::StopServicesAck> ack_obj(
									new comm_messages::StopServicesAck(error_status));
					boost::shared_ptr<MessageResultWrap> result1(
						new MessageResultWrap(messageTypeEnum::typeEnums::STOP_SERVICES_ACK));
					OSDLOG(DEBUG, "Send the stop service ack to gl");
					MessageExternalInterface::sync_send_stop_services_ack(
                                                        ack_obj.get(),
                                                        msg_comm_obj->get_comm_object(),
                                                        boost::bind(&LlMsgHandler::success_handler, this, result1),
                                                        boost::bind(&LlMsgHandler::failure_handler, this),
							30);
				}
				break;
			default:
				OSDLOG(DEBUG, "Ignore this unhandle msg type "<< msg_type);	
		}
	}
}
} //namespace ll_parser
