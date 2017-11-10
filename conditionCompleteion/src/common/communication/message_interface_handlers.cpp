#include <boost/bind.hpp>
#include "communication/message_interface_handlers.h"
#include "communication/callback.h"
#include "boost/date_time/posix_time/posix_time.hpp"

void success_handler(boost::shared_ptr<MessageResultWrap> result)
{
	boost::shared_ptr<MessageResultWrap> unsed = result;
	std::cout << "default success handler executed" << std::endl;	//TODO: devesh: replace with debug logger
}

void failure_handler()
{
	std::cout << "default failure handler executed" << std::endl;	//TODO: devesh: replace with debug logger
}

void MessageExternalInterface::async_send_heartbeat(
	comm_messages::Message * msg,
	boost::shared_ptr<Communication::AsynchronousCommunication> com,
	success_handler_signature success,
	boost::function<void()> failure
)
{
	MessageInterface<comm_messages::HeartBeat> interface;
	interface.async_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_heartbeat(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeat> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_heartbeat_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeatAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_get_global_map_info(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetGlobalMap> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_global_map_info(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GlobalMapInfo> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}


void MessageExternalInterface::sync_send_local_leader_start_monitoring(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoring> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_local_leader_start_monitoring_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoringAck > interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_osd_start_monitoring(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoring> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_osd_start_monitoring_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoringAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_recovery_proc_start_monitoring(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::RecvProcStartMonitoring> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_recovery_proc_start_monitoring_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::RecvProcStartMonitoringAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_component_transfer_info(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferInfo> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_get_version_id_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetVersionIDAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_component_transfer_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferInfoAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_component_transfer_final_status(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferFinalStat> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_component_transfer_final_status_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferFinalStatAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_get_service_component(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetServiceComponent> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_get_service_component_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetServiceComponentAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

boost::shared_ptr<MessageResultWrap> MessageExternalInterface::sync_read_any_message(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	//MessageInterface<comm_messages::HeartBeatAck> interface;
	MessageInterface<comm_messages::Message> interface;
	return interface.sync_read_any_message(com, timeout);
}

void MessageExternalInterface::sync_send_node_deletion_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeDeleteCliAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_addition_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionGl> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_add_final_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionFinalAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

// Msg Interface for Node Rejoin Recovery


void MessageExternalInterface::sync_send_node_rejoin_recovery_msg_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeAdditionGlAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_addition_ack_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeAdditionGlAck> interface;
        interface.sync_send_message(msg, com, success, failure, timeout);
}


void MessageExternalInterface::sync_send_node_addition_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionCliAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_initiate_cluster_recovery_message(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::InitiateClusterRecovery> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_retire_to_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetire> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_retire_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetireAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_get_cluster_status_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetOsdClusterStatusAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::async_send_node_system_stop_to_ll(comm_messages::Message * msg, boost::shared_ptr<Communication::AsynchronousCommunication> com, success_handler_signature success, boost::function<void()> failure)
{
        MessageInterface<comm_messages::NodeSystemStopCli> interface;
        interface.async_send_message(msg, com, success, failure);
}


void MessageExternalInterface::sync_send_local_node_status_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalNodeStatus> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_status_to_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatus> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_status_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatusAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_stop_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCli> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_stop_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCliAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

// sync send node stop to gl

void MessageExternalInterface::sync_send_node_stop_to_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeStopLL> interface;
        interface.sync_send_message(msg, com, success, failure, timeout);
}

// sync read node stop from gl


boost::shared_ptr<MessageResult<comm_messages::NodeStopLLAck> > MessageExternalInterface::sync_read_node_stop_ack_gl(boost::shared_ptr<sync_com> com, uint32_t timeout)                                 
{                                                       
        MessageInterface<comm_messages::NodeStopLLAck> interface;
        return interface.sync_read_message(com, timeout);
}   

boost::shared_ptr<MessageResult<comm_messages::NodeAdditionFinalAck> > MessageExternalInterface::sync_read_node_add_final_ack_gl(boost::shared_ptr<sync_com> com, uint32_t timeout)                                 
{                                                       
        MessageInterface<comm_messages::NodeAdditionFinalAck> interface;
        return interface.sync_read_message(com, timeout);
}   

void MessageExternalInterface::sync_send_stop_services_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServices> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_stop_services_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServicesAck> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_failover(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeFailover> interface;
        interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_node_failover_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeFailoverAck> interface;
        interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_take_gl_ownership(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::TakeGlOwnership> interface;
	interface.sync_send_message(msg, com, success, failure, timeout);
}

void MessageExternalInterface::sync_send_take_gl_ownership_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::TakeGlOwnershipAck> interface;
        interface.sync_send_message(msg, com, success, failure, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::HeartBeatAck> > MessageExternalInterface::sync_read_heartbeatack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeatAck> interface;
	return interface.sync_read_message(com, timeout);	
}

boost::shared_ptr<MessageResult<comm_messages::HeartBeat> > MessageExternalInterface::sync_read_heartbeat(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeat> interface;
	return interface.sync_read_message(com, timeout);	
}

boost::shared_ptr<MessageResult<comm_messages::GetGlobalMap> > MessageExternalInterface::sync_read_get_global_map_info(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::GetGlobalMap> interface;
	return interface.sync_read_message(com, timeout);	
}

boost::shared_ptr<MessageResult<comm_messages::GlobalMapInfo> > MessageExternalInterface::sync_read_global_map_info(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::GlobalMapInfo> interface;
	return interface.sync_read_message(com, timeout);	
}

boost::shared_ptr<MessageResult<comm_messages::NodeAdditionGlAck> > MessageExternalInterface::sync_read_node_addition_ack_from_ll(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionGlAck> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeRetire> > MessageExternalInterface::sync_read_node_retire(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetire> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeRetireAck> > MessageExternalInterface::sync_read_node_retire_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetireAck> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeSystemStopCli> > MessageExternalInterface::sync_read_node_system_stop(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeSystemStopCli> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::LocalNodeStatus> > MessageExternalInterface::sync_read_local_node_status(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalNodeStatus> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStatus> > MessageExternalInterface::sync_read_node_status(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatus> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStatusAck> > MessageExternalInterface::sync_read_node_status_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatusAck> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStopCli> > MessageExternalInterface::sync_read_node_stop(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCli> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStopCliAck> > MessageExternalInterface::sync_read_node_stop_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCliAck> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::StopServices> > MessageExternalInterface::sync_read_stop_services(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServices> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::StopServicesAck> > MessageExternalInterface::sync_read_stop_services_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServicesAck> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeFailover> > MessageExternalInterface::sync_read_node_failover(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeFailover> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeFailoverAck> > MessageExternalInterface::sync_read_node_failover_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeFailoverAck> interface;
        return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnership> > MessageExternalInterface::sync_read_take_gl_ownership(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::TakeGlOwnership> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnershipAck> > MessageExternalInterface::sync_read_take_gl_ownership_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::TakeGlOwnershipAck> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoring> > MessageExternalInterface::sync_read_local_leader_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoring> interface;
	return interface.sync_read_message(com, timeout);	
}

boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoringAck> > MessageExternalInterface::sync_read_local_leader_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoringAck> interface;
	return interface.sync_read_message(com, timeout);	
}

// read sync readnode rejoin gl ack

boost::shared_ptr<MessageResult<comm_messages::NodeRejoinAfterRecovery> > MessageExternalInterface::sync_read_node_rejoin_recovery_msg_gl_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRejoinAfterRecovery> interface;
	return interface.sync_read_message(com, timeout);
}



boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoring> > MessageExternalInterface::sync_read_osd_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoring> interface;
	return interface.sync_read_message(com, timeout);	
}

boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoringAck> > MessageExternalInterface::sync_read_osd_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoringAck> interface;
	return interface.sync_read_message(com, timeout);	
}

boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoring> > MessageExternalInterface::sync_read_recovery_proc_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::RecvProcStartMonitoring> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoringAck> > MessageExternalInterface::sync_read_recovery_proc_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::RecvProcStartMonitoringAck> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeAdditionCli> > MessageExternalInterface::sync_read_node_addition_cli(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionCli> interface;
	return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::CompTransferInfo> > MessageExternalInterface::sync_read_component_transfer_info(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::CompTransferInfo> interface;
        return interface.sync_read_message(com, timeout);
}

		
boost::shared_ptr<MessageResult<comm_messages::CompTransferFinalStat> > MessageExternalInterface::sync_read_component_transfer_final_status(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::CompTransferFinalStat> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::GetServiceComponent> > MessageExternalInterface::sync_read_get_service_component(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::GetServiceComponent> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::GetServiceComponentAck> > MessageExternalInterface::sync_read_get_service_component_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::GetServiceComponentAck> interface;
        return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResultWrap> MessageExternalInterface::send_start_monitoring_message_to_local_leader_synchronously(
	std::string service_ip,
	uint32_t service_port,
	std::string service_id,
	boost::shared_ptr<sync_com> comm_sock,
	uint32_t timeout
)
{
	boost::posix_time::ptime start_time = boost::posix_time::second_clock::local_time();
	comm_messages::Message * msg = new comm_messages::OsdStartMonitoring(service_id, service_port, service_ip);

	SuccessCallBack * callback = new SuccessCallBack();

	MessageInterface<comm_messages::OsdStartMonitoring> interface;
	interface.sync_send_message(
		msg,
		comm_sock,
		boost::bind(
			&SuccessCallBack::success_handler,
			callback,
			_1
		),
		boost::bind(
			&SuccessCallBack::failure_handler,
			callback
		),
		timeout
	);

	boost::shared_ptr<MessageResultWrap> result = callback->get_object();

	if (!callback->get_result()){
		delete msg;
		delete callback;
		return result;
		
	}

	boost::posix_time::ptime interim_time = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_duration elapsed_time = interim_time - start_time;

	if (timeout > 0 && timeout - elapsed_time.total_seconds() <= 0){
		delete msg;
		delete callback;
		result->set_error_message("Timeout occurred");
		result->set_error_code(Communication::TIMEOUT_OCCURED);
		return result;
	}

	uint32_t diff_time; 
	if (timeout == 0){
		diff_time = 0;
	}
	else{
		diff_time = timeout - elapsed_time.total_seconds();
	}

	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.get_message_object(comm_sock, success_handler, failure_handler, diff_time);

	delete msg;
	delete callback;

	return msg_handler.get_msg();
}

//-----------------Sanchit
boost::shared_ptr<MessageResultWrap> MessageExternalInterface::send_heartbeat_message_synchronously(std::string service_id, boost::shared_ptr<sync_com> comm_sock, uint32_t timeout)
{
	boost::posix_time::ptime start_time = boost::posix_time::second_clock::local_time();
	comm_messages::Message * msg = new comm_messages::HeartBeat(service_id, 123);
	
	SuccessCallBack * callback = new SuccessCallBack();
	
	MessageInterface<comm_messages::HeartBeat> interface;
	interface.sync_send_message(
		msg, 
		comm_sock, 
		boost::bind(
			&SuccessCallBack::success_handler, 
			callback, 
			_1
			), 
		boost::bind(
			&SuccessCallBack::failure_handler, 
			callback
			), 
		timeout);

	boost::shared_ptr<MessageResultWrap> result = callback->get_object();
	
	if (!callback->get_result()){
		delete msg;
		delete callback;
		return result;
	}

	boost::posix_time::ptime interim_time = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_duration elapsed_time = interim_time - start_time;

	if (timeout > 0 && timeout - elapsed_time.total_seconds() <= 0){
		delete msg;
		delete callback;
		result->set_error_message("Timeout occurred");
		result->set_error_code(Communication::TIMEOUT_OCCURED);
		return result;
	}

	uint32_t diff_time; 
	if (timeout == 0){
		diff_time = 0;
	}
	else{
		diff_time = timeout - elapsed_time.total_seconds();
	}


	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.get_message_object(comm_sock, success_handler, failure_handler, diff_time);

	delete msg;
	delete callback;

	return msg_handler.get_msg();
}
//-----------Sanchit

bool MessageExternalInterface::send_heartbeat_message_asynchronously(std::string service_id, boost::shared_ptr<sync_com> comm_sock, uint32_t timeout)
{
	comm_messages::Message * msg = new comm_messages::HeartBeat(service_id, 123);
	SuccessCallBack * callback = new SuccessCallBack();
	MessageInterface<comm_messages::HeartBeat> interface;
	interface.sync_send_message(msg, comm_sock, boost::bind(&SuccessCallBack::success_handler, callback, _1), boost::bind(&SuccessCallBack::failure_handler, callback), timeout);
	return callback->get_result();
}

//HTTP Interfaces
boost::shared_ptr<MessageResult<comm_messages::TransferComponentsAck> > MessageExternalInterface::send_transfer_component_http_request(
	boost::shared_ptr<sync_com> com,
	comm_messages::Message * msg,
	const std::string & ip,
	const uint32_t port,
	uint32_t timeout
)
{
	const std::string http_request_method = "TRANSFER_COMPONENTS";
	MessageInterface<comm_messages::TransferComponentsAck> interface;
	return interface.sync_http_message(com, msg, ip, port, http_request_method, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::BlockRequestsAck> > MessageExternalInterface::send_stop_service_http_request(
	boost::shared_ptr<sync_com> com,
	comm_messages::Message * msg,
	const std::string & ip,
	const uint32_t port,
	uint32_t timeout
)
{
	const std::string http_request_method = "STOP_SERVICE";
	MessageInterface<comm_messages::BlockRequestsAck> interface;
	return interface.sync_http_message(com, msg, ip, port, http_request_method, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::StatusAck> > MessageExternalInterface::sync_send_update_container(
	boost::shared_ptr<sync_com> com,
	comm_messages::Message * msg,
	const std::string & ip,
	const uint32_t port,
	uint32_t timeout
)
{
	const std::string http_request_method = "UPDATE_CONTAINER";
	MessageInterface<comm_messages::StatusAck> interface;
	return interface.sync_http_message(com, msg, ip, port, http_request_method, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::StatusAck> > MessageExternalInterface::sync_send_release_lock(
	boost::shared_ptr<sync_com> com,
	comm_messages::Message * msg,
	const std::string & ip,
	const uint32_t port,
	uint32_t timeout
)
{
	const std::string http_request_method = "RELEASE_LOCK";
	MessageInterface<comm_messages::StatusAck> interface;
	return interface.sync_http_message(com, msg, ip, port, http_request_method, timeout);
}
