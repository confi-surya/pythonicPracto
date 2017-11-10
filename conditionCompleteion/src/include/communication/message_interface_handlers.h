#ifndef __MESSAGE_INTERFACE_HANDLER_321__
#define __MESSAGE_INTERFACE_HANDLER_321__

#include <boost/function.hpp>
#include "communication/message.h"
#include "communication/message_format_handler.h"
#include "communication/message_result_wrapper.h"
#include "communication/message_status.h"
#include "communication/communication.h"

//using namespace comm_messages;

typedef Communication::SynchronousCommunication sync_com;
typedef boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler_signature;

void success_handler(boost::shared_ptr<MessageResultWrap> result);
void failure_handler();

template <typename T>
class MessageInterface
{
	public:
		MessageInterface(){};
		~MessageInterface(){};

		boost::shared_ptr<MessageResultWrap> sync_read_any_message(boost::shared_ptr<sync_com>, uint32_t timeout = 0);
		boost::shared_ptr<MessageResult<T> > sync_read_message(boost::shared_ptr<sync_com>, uint32_t = 0);
		void sync_send_message(
				comm_messages::Message *,
				boost::shared_ptr<sync_com>,
				boost::function<void(boost::shared_ptr<MessageResultWrap>)> success,
				boost::function<void()> failure,
				uint32_t = 0
			);

		void async_read_any_message(
				boost::shared_ptr<Communication::AsynchronousCommunication>,
				boost::function<void(boost::shared_ptr<MessageResultWrap>)> success,
				boost::function<void()> failure
			);
		void async_send_message(
				comm_messages::Message *,
				boost::shared_ptr<Communication::AsynchronousCommunication>,
				boost::function<void(boost::shared_ptr<MessageResultWrap>)> success,
				boost::function<void()> failure
			);

		boost::shared_ptr<MessageResult<T> > sync_http_message(
				boost::shared_ptr<sync_com>,
				comm_messages::Message *,
				const std::string &,
				const uint32_t,
				const std::string &,
				uint32_t
			);

};

template <typename T>
void MessageInterface<T>::async_read_any_message(
	boost::shared_ptr<Communication::AsynchronousCommunication> com,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success,
	boost::function<void()> failure
)
{
	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.get_message_object(com, success, failure);
}

template <typename T>
void MessageInterface<T>::async_send_message(
	comm_messages::Message * msg,
	boost::shared_ptr<Communication::AsynchronousCommunication> com,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success,
	boost::function<void()> failure
)
{
	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.send_message(com, msg, success, failure);
}

template <typename T>
boost::shared_ptr<MessageResultWrap> MessageInterface<T>::sync_read_any_message(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	boost::shared_ptr<MessageResultWrap> hb;
	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.get_message_object(com, success_handler, failure_handler, timeout);

	hb = msg_handler.get_msg();
	return hb;
}

template <typename T>
boost::shared_ptr<MessageResult<T> > MessageInterface<T>::sync_read_message(boost::shared_ptr<sync_com> com, uint32_t timeout)
{
	boost::shared_ptr<MessageResultWrap> hb;
	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.get_message_object(com, success_handler, failure_handler, timeout);

	hb = msg_handler.get_msg();
	boost::shared_ptr<MessageResult<T> > result(new MessageResult<T>());

	if (hb->get_error_code() == 0){
		T * actual_msg = new T();
		if (!actual_msg->deserialize(hb->get_payload(), hb->get_size())){
			result->set_return_code(Communication::SERIALIZATION_FAILED);
			result->set_msg("Deserialization failed");
			result->set_status(false);
		}
		else{
			result->set_message_object(actual_msg);
			result->set_return_code(hb->get_error_code());
			result->set_msg(hb->get_error_message());
			result->set_status(true);
		}
	}
	else{
		result->set_return_code(hb->get_error_code());
		result->set_msg(hb->get_error_message());
		result->set_status(false);		
	}

	hb.reset();
	return result;
}

template <typename T>
void MessageInterface<T>::sync_send_message(
	comm_messages::Message * msg,
	boost::shared_ptr<sync_com> com,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success,
	boost::function<void()> failure,
	uint32_t timeout
)
{
	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.send_message(com, msg, success, failure, timeout);
}

template <typename T>
boost::shared_ptr<MessageResult<T> > MessageInterface<T>::sync_http_message(
	boost::shared_ptr<sync_com> com,
	comm_messages::Message * msg,
	const std::string & ip,
	const uint32_t port,
	const std::string & http_request_method,
	uint32_t timeout
)
{
	boost::shared_ptr<MessageResultWrap> hb;
	ProtocolStructure::MessageFormatHandler msg_handler;
	msg_handler.send_http_request(
				com,
				msg,
				ip,
				port,
				http_request_method,
				success_handler,
				failure_handler,
				timeout
			);

	hb = msg_handler.get_msg();
	boost::shared_ptr<MessageResult<T> > result(new MessageResult<T>());

	if (hb->get_error_code() == 0){
		T * actual_msg = new T();
		if (hb->get_size() ==0){
			result->set_message_object(NULL);
			result->set_return_code(hb->get_error_code());
			result->set_status(true);
			hb.reset();
			return result;
		}
		if (!actual_msg->deserialize(hb->get_payload(), hb->get_size())){
			result->set_return_code(Communication::SERIALIZATION_FAILED);
			result->set_msg("Deserialization failed in http request response");
			result->set_status(false);
		}
		else{
			result->set_message_object(actual_msg);
			result->set_return_code(hb->get_error_code());
			result->set_msg(hb->get_error_message());
			result->set_status(true);
		}
	}
	else{
		result->set_return_code(hb->get_error_code());
		result->set_msg(hb->get_error_message());
		result->set_status(false);		
	}

	hb.reset();
	return result;
}

class MessageExternalInterface
{
	public:
		static void async_send_heartbeat(
			comm_messages::Message *,
			boost::shared_ptr<Communication::AsynchronousCommunication>,
			success_handler_signature success,
			boost::function<void()> failure
			);

		static void sync_send_heartbeat(
			comm_messages::Message *,
			boost::shared_ptr<sync_com>,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_heartbeat_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_get_global_map_info(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_global_map_info(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_local_leader_start_monitoring(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_local_leader_start_monitoring_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_node_add_final_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout
			);
		static void sync_send_osd_start_monitoring(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_osd_start_monitoring_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_recovery_proc_start_monitoring(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_recovery_proc_start_monitoring_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_component_transfer_info(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_component_transfer_final_status(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_get_service_component(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_get_service_component_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void  sync_send_node_deletion_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_node_addition_ack(
			comm_messages::Message*,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_node_addition_to_ll(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_node_addition_ack_gl(
			comm_messages::Message * msg, 
			boost::shared_ptr<sync_com> com, 
			success_handler_signature success, 
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_component_transfer_ack(
			comm_messages::Message * msg, 
			boost::shared_ptr<sync_com> com, 
			success_handler_signature success, 
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
			
		static void sync_send_node_retire_to_gl(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_node_retire_ack(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
			uint32_t timeout = 30
                        );
		static void async_send_node_system_stop_to_ll(
			comm_messages::Message *,
                        boost::shared_ptr<Communication::AsynchronousCommunication>,
                        success_handler_signature success,
                        boost::function<void()> failure
                        );
		static void sync_send_local_node_status_to_ll(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_get_cluster_status_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static void sync_send_node_status_to_gl(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        ); 
		static void sync_send_node_status_ack(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_node_stop_to_ll(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_node_stop_ack(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_stop_services_to_ll(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_stop_services_ack(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_node_failover(
			comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_node_failover_ack(
                        comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_take_gl_ownership(
                        comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_take_gl_ownership_ack(
                        comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );
		static void sync_send_component_transfer_final_status_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static boost::shared_ptr<MessageResult<comm_messages::TransferComponentsAck> > send_transfer_component_http_request(
			boost::shared_ptr<sync_com> com,
			comm_messages::Message * msg,
			const std::string & ip,
			const uint32_t port,
			uint32_t timeout = 0
			);
		static void sync_send_get_version_id_ack(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static boost::shared_ptr<MessageResult<comm_messages::BlockRequestsAck> > send_stop_service_http_request(
			boost::shared_ptr<sync_com> com,
			comm_messages::Message * msg,
			const std::string & ip,
			const uint32_t port,
			uint32_t timeout = 0
			);

		static boost::shared_ptr<MessageResult<comm_messages::StatusAck> > sync_send_update_container(
			boost::shared_ptr<sync_com> com,
			comm_messages::Message * msg,
			const std::string & ip,
			const uint32_t port,
			uint32_t timeout = 0
			);
		static void sync_send_initiate_cluster_recovery_message(
			comm_messages::Message * msg,
			boost::shared_ptr<sync_com> com,
			success_handler_signature success,
			boost::function<void()> failure,
			uint32_t timeout = 30
			);
		static boost::shared_ptr<MessageResult<comm_messages::StatusAck> > sync_send_release_lock(
			boost::shared_ptr<sync_com> com,
			comm_messages::Message * msg,
			const std::string & ip,
			const uint32_t port,
			uint32_t timeout = 0
			);

		static void sync_send_node_rejoin_recovery_msg_gl(
			comm_messages::Message * msg, 
			boost::shared_ptr<sync_com> com, 
			success_handler_signature success, 
			boost::function<void()> failure,
			uint32_t timeout = 30
			);

		static void sync_send_node_stop_to_gl(
                        comm_messages::Message * msg,
                        boost::shared_ptr<sync_com> com,
                        success_handler_signature success,
                        boost::function<void()> failure,
                        uint32_t timeout = 30
                        );


		static boost::shared_ptr<MessageResultWrap> sync_read_any_message(boost::shared_ptr<sync_com> com, uint32_t timeout = 0);
		static boost::shared_ptr<MessageResult<comm_messages::HeartBeatAck> > sync_read_heartbeatack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::HeartBeat> > sync_read_heartbeat(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeAdditionCli> > sync_read_node_addition_cli(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeAdditionGlAck> > sync_read_node_addition_ack_from_ll(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeRetire> > sync_read_node_retire(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeRetireAck> > sync_read_node_retire_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeSystemStopCli> > sync_read_node_system_stop(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::LocalNodeStatus> > sync_read_local_node_status(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeStatus> > sync_read_node_status(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeStatusAck> > sync_read_node_status_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeStopCli> > sync_read_node_stop(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeStopCliAck> > sync_read_node_stop_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::StopServices> > sync_read_stop_services(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::StopServicesAck> > sync_read_stop_services_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeFailover> > sync_read_node_failover(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeFailoverAck> > sync_read_node_failover_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnership> > sync_read_take_gl_ownership(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::TakeGlOwnershipAck> > sync_read_take_gl_ownership_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::GetGlobalMap> > sync_read_get_global_map_info(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::GlobalMapInfo> > sync_read_global_map_info(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoring> > sync_read_local_leader_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);

		static boost::shared_ptr<MessageResult<comm_messages::LocalLeaderStartMonitoringAck> > sync_read_local_leader_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);

// node rejoin recovery
		static boost::shared_ptr<MessageResult<comm_messages::NodeRejoinAfterRecovery> > sync_read_node_rejoin_recovery_msg_gl_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);

//node stop ack from gl

		static boost::shared_ptr<MessageResult<comm_messages::NodeStopLLAck> > sync_read_node_stop_ack_gl(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::NodeAdditionFinalAck> > sync_read_node_add_final_ack_gl(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);

		static boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoring> > sync_read_osd_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::OsdStartMonitoringAck> > sync_read_osd_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoring> > sync_read_recovery_proc_start_monitoring(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::RecvProcStartMonitoringAck> > sync_read_recovery_proc_start_monitoring_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::CompTransferInfo> > sync_read_component_transfer_info(boost::shared_ptr<sync_com> com, uint32_t timeout = 360);
		static boost::shared_ptr<MessageResult<comm_messages::CompTransferFinalStat> > sync_read_component_transfer_final_status(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::GetServiceComponent> > sync_read_get_service_component(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResult<comm_messages::GetServiceComponentAck> > sync_read_get_service_component_ack(boost::shared_ptr<sync_com> com, uint32_t timeout = 30);

		// TODO: devesh : implement below interfaces
		static boost::shared_ptr<MessageResultWrap> send_start_monitoring_message_to_local_leader_synchronously(std::string ip, uint32_t port, std::string service_id, boost::shared_ptr<sync_com>, uint32_t timeout = 30);
		static boost::shared_ptr<MessageResultWrap> send_heartbeat_message_synchronously(std::string service_id, boost::shared_ptr<sync_com> comm_sock, uint32_t timeout = 30);
		static bool send_heartbeat_message_asynchronously(std::string, boost::shared_ptr<sync_com>, uint32_t timeout = 30);
};

#endif
