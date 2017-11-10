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

void MessageExternalInterface::sync_send_initiate_cluster_recovery_message(
	comm_messages::Message * msg,
	boost::shared_ptr<sync_com> com,
	success_handler_signature success,
	boost::function<void()> failure,uint32_t timeout
)
{
}

void MessageExternalInterface::async_send_heartbeat(
	comm_messages::Message * msg,
	boost::shared_ptr<Communication::AsynchronousCommunication> com,
	success_handler_signature success,
	boost::function<void()> failure
)
{
	MessageInterface<comm_messages::HeartBeat> interface;
	std::cout<< "Sending Async Heartbeat - MessageExternalInterface::async_send_heartbeat" << std::endl;
	//interface.async_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_add_final_ack(
	comm_messages::Message * msg,
	boost::shared_ptr<sync_com> com,
	success_handler_signature success,
	boost::function<void()> failure,
	uint32_t timeout
)
{
	MessageInterface<comm_messages::NodeAdditionFinalAck> interface;
	std::cout<< "Sending sync node addition final ack" << std::endl;
}

void MessageExternalInterface::sync_send_heartbeat(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeat> interface;
	std::cout<< "Sending Sync Heartbeat - MessageExternalInterface::sync_send_heartbeat" << std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_heartbeat_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeatAck> interface;
	std::cout<< "Getting Sync Heartbeat Ack - MessageExternalInterface::sync_send_heartbeat_ack" << std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_get_global_map_info(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetGlobalMap> interface;
	std::cout << "MessageExternalInterface::sync_send_get_global_map_info" <<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_global_map_info(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GlobalMapInfo> interface;
	std::cout<<"MessageExternalInterface::sync_send_global_map_info"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}


void MessageExternalInterface::sync_send_local_leader_start_monitoring(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoring> interface;
	std::cout<<"MessageExternalInterface::sync_send_local_leader_start_monitoring"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_local_leader_start_monitoring_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoringAck > interface;
	std::cout<<"MessageExternalInterface::sync_send_local_leader_start_monitoring_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_osd_start_monitoring(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoring> interface;
	std::cout<<"MessageExternalInterface::sync_send_osd_start_monitoring"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_osd_start_monitoring_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoringAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_osd_start_monitoring_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_recovery_proc_start_monitoring(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::RecvProcStartMonitoring> interface;
	std::cout<<"MessageExternalInterface::sync_send_recovery_proc_start_monitoring"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_recovery_proc_start_monitoring_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::RecvProcStartMonitoringAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_recovery_proc_start_monitoring_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_component_transfer_info(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferInfo> interface;
	std::cout<<"MessageExternalInterface::sync_send_component_transfer_info"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_get_version_id_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetVersionIDAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_get_version_id_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_component_transfer_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferInfoAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_component_transfer_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_component_transfer_final_status(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferFinalStat> interface;
	std::cout<<"MessageExternalInterface::sync_send_component_transfer_final_status"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_component_transfer_final_status_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::CompTransferFinalStatAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_component_transfer_final_status_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_get_service_component(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetServiceComponent> interface;
	std::cout<<"MessageExternalInterface::sync_send_get_service_component"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_get_service_component_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetServiceComponentAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_get_service_component_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

boost::shared_ptr<MessageResultWrap> MessageExternalInterface::sync_read_any_message(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	//MessageInterface<comm_messages::HeartBeatAck> interface;
	MessageInterface<comm_messages::Message> interface;
	std::cout<<"MessageExternalInterface::sync_read_any_message"<<std::endl;
	boost::shared_ptr<MessageResultWrap> msg_res_wrap;
	return msg_res_wrap;
	//return interface.sync_read_any_message(com);
}

void MessageExternalInterface::sync_send_node_deletion_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeDeleteCliAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_deletion_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

// Msg Interface for Node Rejoin Recovery


void MessageExternalInterface::sync_send_node_rejoin_recovery_msg_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeAdditionGlAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_rejoin_recovery_msg_gl"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure, timeout);
}


// read sync readnode rejoin gl ack

boost::shared_ptr<MessageResult<comm_messages::NodeRejoinAfterRecovery> > MessageExternalInterface::sync_read_node_rejoin_recovery_msg_gl_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeRejoinAfterRecovery> interface;
        return interface.sync_read_message(com);
}


// sync send node stop to gl

void MessageExternalInterface::sync_send_node_stop_to_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeStopLL> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_stop_to_gl"<<std::endl;
        interface.sync_send_message(msg, com, success, failure, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::NodeAdditionFinalAck> > MessageExternalInterface::sync_read_node_add_final_ack_gl(boost::shared_ptr<sync_com> com, uint32_t timeout)                                 
{                                                       
        MessageInterface<comm_messages::NodeAdditionFinalAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_add_final_ack_gl"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeAdditionFinalAck> > nodAddFinalAck(new MessageResult<comm_messages::NodeAdditionFinalAck>());

	return nodAddFinalAck;
}
// sync read node stop ack from gl


boost::shared_ptr<MessageResult<comm_messages::NodeStopLLAck> > MessageExternalInterface::sync_read_node_stop_ack_gl(boost::shared_ptr<sync_com> com, uint32_t timeout)                                 
{                                                       
        MessageInterface<comm_messages::NodeStopLLAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_stop_ack_gl"<<std::endl;
	boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(12, "dsad"));
	comm_messages::NodeStopLLAck *obj(new comm_messages::NodeStopLLAck("dsadsa", error_status, 466));
	boost::shared_ptr<MessageResult<comm_messages::NodeStopLLAck> > nodestopllack(new MessageResult<comm_messages::NodeStopLLAck>(obj, 1, true, "dasds"));
	return nodestopllack;
//        return interface.sync_read_message(com);
}


void MessageExternalInterface::sync_send_node_addition_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionGl> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_addition_to_ll"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_addition_ack_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeAdditionGlAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_addition_ack_gl"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}


void MessageExternalInterface::sync_send_node_addition_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionCliAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_addition_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_retire_to_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetire> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_retire_to_gl"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);

}

void MessageExternalInterface::sync_send_node_retire_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetireAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_retire_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::async_send_node_system_stop_to_ll(comm_messages::Message * msg, boost::shared_ptr<Communication::AsynchronousCommunication> com, success_handler_signature success, boost::function<void()> failure)
{
        MessageInterface<comm_messages::NodeSystemStopCli> interface;
	std::cout<<"MessageExternalInterface::async_send_node_system_stop_to_ll"<<std::endl;
	//interface.async_send_message(msg, com, success, failure);
}


void MessageExternalInterface::sync_send_local_node_status_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalNodeStatus> interface;
	std::cout<<"MessageExternalInterface::sync_send_local_node_status_to_ll"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_status_to_gl(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatus> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_status_to_gl"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_status_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatusAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_status_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_stop_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCli> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_stop_to_ll"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_stop_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCliAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_stop_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_stop_services_to_ll(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServices> interface;
	std::cout<<"MessageExternalInterface::sync_send_stop_services_to_ll"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_stop_services_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServicesAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_stop_services_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_failover(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeFailover> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_failover"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_node_failover_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeFailoverAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_node_failover_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_take_gl_ownership(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::TakeGlOwnership> interface;
	std::cout<<"MessageExternalInterface::sync_send_take_gl_ownership"<<std::endl;
	boost::shared_ptr<MessageResultWrap>  m1;
	success(m1);
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_take_gl_ownership_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
        MessageInterface<comm_messages::TakeGlOwnershipAck> interface;
	std::cout<<"MessageExternalInterface::sync_send_take_gl_ownership_ack"<<std::endl;
	//interface.sync_send_message(msg, com, success, failure);
}

void MessageExternalInterface::sync_send_get_cluster_status_ack(comm_messages::Message * msg, boost::shared_ptr<sync_com> com, success_handler_signature success, boost::function<void()> failure, uint32_t timeout)
{
	MessageInterface<comm_messages::GetOsdClusterStatusAck> interface;
	std::cout << "MessageExternalInterface::sync_send_get_cluster_status_ack\n";
}

boost::shared_ptr<MessageResult<comm_messages::HeartBeatAck> > MessageExternalInterface::sync_read_heartbeatack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeatAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_heartbeatack"<<std::endl;
	comm_messages::HeartBeatAck *obj(new comm_messages::HeartBeatAck(45));
	boost::shared_ptr<MessageResult<comm_messages::HeartBeatAck> > hbrt_ack(new MessageResult<comm_messages::HeartBeatAck>(obj, true, 1, "dsds"));
	return hbrt_ack;
	//return interface.sync_read_message(com);	
}
	
boost::shared_ptr<MessageResult<comm_messages::HeartBeat> > MessageExternalInterface::sync_read_heartbeat(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::HeartBeat> interface;
	std::cout<<"MessageExternalInterface::sync_read_heartbeatack"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::HeartBeat> > hbrt (new MessageResult<comm_messages::HeartBeat> );
	comm_messages::HeartBeat *hb = new comm_messages::HeartBeat;
	hbrt->set_status(true);
	hbrt->set_message_object(hb);
	return hbrt;
	//return interface.sync_read_message(com);	
}

boost::shared_ptr<MessageResult<comm_messages::GetGlobalMap> > MessageExternalInterface::sync_read_get_global_map_info(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::GetGlobalMap> interface;
	std::cout<<"MessageExternalInterface::sync_read_get_global_map_info"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::GetGlobalMap> > get_glbl_map;
	return get_glbl_map;
	//return interface.sync_read_message(com);	
}

boost::shared_ptr<MessageResult<comm_messages::GlobalMapInfo> > MessageExternalInterface::sync_read_global_map_info(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::GlobalMapInfo> interface;
	std::cout<<"MessageExternalInterface::sync_read_global_map_info"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::GlobalMapInfo> > glbl_map_info;
	return glbl_map_info;
	//return interface.sync_read_message(com);	
}

boost::shared_ptr<MessageResult<comm_messages::NodeAdditionGlAck> > MessageExternalInterface::sync_read_node_addition_ack_from_ll(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionGlAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_addition_ack_from_ll"<<std::endl;
	
	boost::shared_ptr<ServiceObject> node_cont(new ServiceObject());
	node_cont->set_service_id("HN0101_61014_container-server");
	node_cont->set_ip("192.168.101.1");
	node_cont->set_port(61017);
	boost::shared_ptr<ServiceObject> node_acc(new ServiceObject());
	node_acc->set_service_id("HN0101_61014_account-server");
	node_acc->set_ip("192.168.101.1");
	node_acc->set_port(61006);
	boost::shared_ptr<ServiceObject> node_pxy(new ServiceObject());
	node_pxy->set_service_id("HN0101_61014_proxy-server");
	node_pxy->set_ip("192.168.101.1");
	node_pxy->set_port(61005);
	boost::shared_ptr<ServiceObject> node_obj(new ServiceObject());
	node_obj->set_service_id("HN0101_61014_object-server");
	node_obj->set_ip("192.168.101.1");
	node_obj->set_port(61008);
	boost::shared_ptr<ServiceObject> node_acup(new ServiceObject());
	node_acup->set_service_id("HN0101_61014_account-updater-server");
	node_acup->set_ip("192.168.101.1");
	node_acup->set_port(61014);

	std::list<boost::shared_ptr<ServiceObject> > node_list;
	node_list.push_back(node_cont);
	node_list.push_back(node_acc);
	node_list.push_back(node_pxy);
	node_list.push_back(node_obj);
	node_list.push_back(node_acup);

	boost::shared_ptr<MessageResult<comm_messages::NodeAdditionGlAck> > node_add_gl_ack (new MessageResult<comm_messages::NodeAdditionGlAck>);
	boost::shared_ptr<ErrorStatus> err_stat (new ErrorStatus);
	err_stat->set_code(OSDException::OSDSuccessCode::NODE_ADDITION_SUCCESS);
	err_stat->set_msg("OSDException::OSDSuccessCode::NODE_ADDITION_SUCCESS");
	comm_messages::NodeAdditionGlAck *na_ack = new comm_messages::NodeAdditionGlAck (node_list, err_stat);
	node_add_gl_ack->set_status(true);
	node_add_gl_ack->set_message_object(na_ack);
	node_add_gl_ack->set_return_code(Communication::SUCCESS);
	return node_add_gl_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeRetire> > MessageExternalInterface::sync_read_node_retire(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetire> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_retire"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeRetire> > node_ret;
	return node_ret;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeRetireAck> > MessageExternalInterface::sync_read_node_retire_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeRetireAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_retire_ack"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeRetireAck> > node_ret_ack;
	return node_ret_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeSystemStopCli> > MessageExternalInterface::sync_read_node_system_stop(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeSystemStopCli> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_system_stop"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeSystemStopCli> > node_system_stop_cli;
	return node_system_stop_cli;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::LocalNodeStatus> > MessageExternalInterface::sync_read_local_node_status(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalNodeStatus> interface;
	std::cout<<"MessageExternalInterface::sync_read_local_node_status"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::LocalNodeStatus> > local_node_status;
	return local_node_status;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStatus> > MessageExternalInterface::sync_read_node_status(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatus> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_status"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeStatus> > node_status;
	return node_status;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStatusAck> > MessageExternalInterface::sync_read_node_status_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStatusAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_status_ack"<<std::endl;
	network_messages::NodeStatusEnum node_status = network_messages::INVALID_NODE;
	comm_messages::NodeStatusAck *obj(new comm_messages::NodeStatusAck(node_status));
	boost::shared_ptr<MessageResult<comm_messages::NodeStatusAck> > node_status_ack(new MessageResult<comm_messages::NodeStatusAck>(obj, true, 1, "dsds"));
	return node_status_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStopCli> > MessageExternalInterface::sync_read_node_stop(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCli> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_stop"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeStopCli> > node_stop_cli;
	return node_stop_cli;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeStopCliAck> > MessageExternalInterface::sync_read_node_stop_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeStopCliAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_stop_ack"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeStopCliAck> > node_stop_cli_ack;
	return node_stop_cli_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::StopServices> > MessageExternalInterface::sync_read_stop_services(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServices> interface;
	std::cout<<"MessageExternalInterface::sync_read_stop_services"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::StopServices> > stop_services;
	return stop_services;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::StopServicesAck> > MessageExternalInterface::sync_read_stop_services_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::StopServicesAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_stop_services_ack"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::StopServicesAck> > stop_services_ack;
	return stop_services_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeFailover> > MessageExternalInterface::sync_read_node_failover(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeFailover> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_failover"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::NodeFailover> > node_failover;
	return node_failover;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeFailoverAck> > MessageExternalInterface::sync_read_node_failover_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::NodeFailoverAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_node_failover_ack"<<std::endl;
	boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(12, "dsad"));
//	boost::shared_ptr<comm_messages::NodeFailoverAck> obj(new comm_messages::NodeFailoverAck(error_status));	
	comm_messages::NodeFailoverAck *obj(new comm_messages::NodeFailoverAck(error_status));
	boost::shared_ptr<MessageResult<comm_messages::NodeFailoverAck> > node_fail_ack(new MessageResult<comm_messages::NodeFailoverAck>(obj, true, 1, "dsads"));

	return node_fail_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnership> > MessageExternalInterface::sync_read_take_gl_ownership(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::TakeGlOwnership> interface;
	std::cout<<"MessageExternalInterface::sync_read_take_gl_ownership"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnership> > take_gl_own;
	return take_gl_own;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnershipAck> > MessageExternalInterface::sync_read_take_gl_ownership_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::TakeGlOwnershipAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_take_gl_ownership_ack"<<std::endl;
	boost::shared_ptr<ErrorStatus> error_status(new ErrorStatus(98, "dsad"));
	comm_messages::TakeGlOwnershipAck *obj(new comm_messages::TakeGlOwnershipAck("new gl", error_status));
	boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnershipAck> > take_gl_own_ack(new MessageResult<comm_messages::TakeGlOwnershipAck>(obj, true, 98, "dsads"));
	return take_gl_own_ack;
	//return interface.sync_read_message(com, timeout);
}

boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoring> > MessageExternalInterface::sync_read_local_leader_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoring> interface;
	std::cout<<"MessageExternalInterface::sync_read_local_leader_start_monitoring"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoring> > ll_strm;
	return ll_strm;
	//return interface.sync_read_message(com);	
}

boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoringAck> > MessageExternalInterface::sync_read_local_leader_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::LocalLeaderStartMonitoringAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_local_leader_start_monitoring_ack"<<std::endl;
	comm_messages::LocalLeaderStartMonitoringAck *obj(new comm_messages::LocalLeaderStartMonitoringAck("strings"));
	boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoringAck> > ll_strm_ack(new MessageResult<comm_messages::LocalLeaderStartMonitoringAck>(obj, true, 1, "dsads"));
	return ll_strm_ack;
	//return interface.sync_read_message(com);	
}

boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoring> > MessageExternalInterface::sync_read_osd_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoring> interface;
	std::cout<<"MessageExternalInterface::sync_read_osd_start_monitoring"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoring> > osd_strm;
	return osd_strm;
	//return interface.sync_read_message(com);	
}

boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoringAck> > MessageExternalInterface::sync_read_osd_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::OsdStartMonitoringAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_osd_start_monitoring_ack"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoringAck> > osd_strm_ack;
	return osd_strm_ack;
	//return interface.sync_read_message(com);	
}

boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoring> > MessageExternalInterface::sync_read_recovery_proc_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::RecvProcStartMonitoring> interface;
	std::cout<<"MessageExternalInterface::sync_read_recovery_proc_start_monitoring"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoring> > rec_proc_strm;
	return rec_proc_strm;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoringAck> > MessageExternalInterface::sync_read_recovery_proc_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::RecvProcStartMonitoringAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_recovery_proc_start_monitoring_ack"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoringAck> > rec_proc_strm_ack;
	return rec_proc_strm_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::NodeAdditionCli> > MessageExternalInterface::sync_read_node_addition_cli(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	MessageInterface<comm_messages::NodeAdditionCli> interface;
	return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::CompTransferInfo> > MessageExternalInterface::sync_read_component_transfer_info(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::CompTransferInfo> interface;
	std::cout<<"MessageExternalInterface::sync_read_component_transfer_info"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::CompTransferInfo> > comp_transfer_info;
	return comp_transfer_info;
	//return interface.sync_read_message(com);
}

		
boost::shared_ptr<MessageResult<comm_messages::CompTransferFinalStat> > MessageExternalInterface::sync_read_component_transfer_final_status(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::CompTransferFinalStat> interface;
	std::cout<<"MessageExternalInterface::sync_read_component_transfer_final_status"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::CompTransferFinalStat> > comp_trans_final_stat;
	return comp_trans_final_stat;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::GetServiceComponent> > MessageExternalInterface::sync_read_get_service_component(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::GetServiceComponent> interface;
	std::cout<<"MessageExternalInterface::sync_read_get_service_component"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::GetServiceComponent> > get_serv_comp (new MessageResult<comm_messages::GetServiceComponent>);
	//boost::shared_ptr<comm_messages::GetServiceComponent> serv_comp(new comm_messages::GetServiceComponent ("HN0101_61014_container-server"));
	comm_messages::GetServiceComponent *serv_comp = new comm_messages::GetServiceComponent ("HN0101_61014_container-server");
	get_serv_comp->set_status(true);
	get_serv_comp->set_message_object(serv_comp);
	return get_serv_comp;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResult<comm_messages::GetServiceComponentAck> > MessageExternalInterface::sync_read_get_service_component_ack(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
        MessageInterface<comm_messages::GetServiceComponentAck> interface;
	std::cout<<"MessageExternalInterface::sync_read_get_service_component_ack"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::GetServiceComponentAck> > get_serv_comp_ack;
	return get_serv_comp_ack;
	//return interface.sync_read_message(com);
}

boost::shared_ptr<MessageResultWrap> MessageExternalInterface::send_heartbeat_message_synchronously(std::string service_id, boost::shared_ptr<sync_com> comm_sock, uint32_t timeout)
{
	std::cout<<"MessageExternalInterface::send_heartbeat_message_synchronously"<<std::endl;
	boost::shared_ptr<MessageResultWrap> msg_resilt_wrap;
	return msg_resilt_wrap;
}

boost::shared_ptr<MessageResultWrap> MessageExternalInterface::send_start_monitoring_message_to_local_leader_synchronously(
	std::string service_ip,
	uint32_t service_port,
	std::string service_id,
	boost::shared_ptr<sync_com> comm_sock,
	uint32_t timeout
)
{
	std::cout<<"MessageExternalInterface::send_start_monitoring_message_to_local_leader_synchronously"<<std::endl;
	boost::shared_ptr<MessageResultWrap> msg_resilt_wrap;
	return msg_resilt_wrap;
	/*
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

	if (timeout - elapsed_time.total_seconds() <= 0){
		delete msg;
		delete callback;
		result->set_error_message("Timeout occurred");
		result->set_error_code(Communication::TIMEOUT_OCCURED);
		return result;
	}

	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.get_message_object(comm_sock, success_handler, failure_handler, timeout - elapsed_time.total_seconds());

	delete msg;
	delete callback;

	return msg_handler.get_msg();
	*/
}

bool MessageExternalInterface::send_heartbeat_message_asynchronously(std::string service_id, boost::shared_ptr<sync_com> comm_sock, uint32_t timeout)
{
	return true;
	/*
	comm_messages::Message * msg = new comm_messages::HeartBeat(service_id, 123);
	SuccessCallBack * callback = new SuccessCallBack();
	MessageInterface<comm_messages::HeartBeat> interface;
	interface.sync_send_message(msg, comm_sock, boost::bind(&SuccessCallBack::success_handler, callback, _1), boost::bind(&SuccessCallBack::failure_handler, callback), timeout);
	return callback->get_result();
	*/
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
	std::cout<<"MessageExternalInterface::send_transfer_component_http_request"<<std::endl;
	boost::shared_ptr<MessageResult<comm_messages::TransferComponentsAck> > trans_comp_ack;
	return trans_comp_ack;
	//return interface.sync_http_message(com, msg, ip, port, http_request_method, timeout);
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
	std::cout<<"MessageExternalInterface::send_stop_service_http_request"<<std::endl;
	comm_messages::BlockRequestsAck *obj(new comm_messages::BlockRequestsAck(true));
	boost::shared_ptr<MessageResult<comm_messages::BlockRequestsAck> > block_req_ack(new MessageResult<comm_messages::BlockRequestsAck>(obj, true, 77, "dfaslda"));
	return block_req_ack;
	//return interface.sync_http_message(com, msg, ip, port, http_request_method, timeout);
}

