#include "osm/globalLeader/cluster_recovery_component.h"
#include "communication/callback.h"

#include <vector>
#include <string>

namespace cluster_recovery_component
{

ClusterRecoveryInitiator::ClusterRecoveryInitiator(
	boost::shared_ptr<gl_monitoring::Monitoring> &monitoring,
	boost::shared_ptr<system_state::StateChangeThreadManager> &states_thread_mgr,
	std::string node_info_file_path,
	uint32_t node_add_response_timeout

):
	monitoring(monitoring),
	states_thread_mgr(states_thread_mgr),
	node_info_file_path(node_info_file_path),
	node_add_response_timeout(node_add_response_timeout)

{
}

void ClusterRecoveryInitiator::initiate_cluster_recovery()
{
	std::vector<std::string> unknown_status_node_list; 
	std::vector<std::string> totally_failed_node_list;
	std::vector<std::string> stopping_node_list;
	this->monitoring->get_node_status_while_cluster_recovery(unknown_status_node_list, totally_failed_node_list, stopping_node_list);
	std::vector<std::string>::iterator it = unknown_status_node_list.begin();
	for(; it != unknown_status_node_list.end(); ++it)
	{
		// OBB-696 fix
		this->thr.reset(new boost::thread(boost::bind(
				&ClusterRecoveryInitiator::send_node_reclaim_request,
				this,
				*it
				)
			)
		);
		this->reclaim_thread_queue.push_back(thr);
	}
	std::vector<boost::shared_ptr<boost::thread> >::iterator itr = this->reclaim_thread_queue.begin();
	for(; itr != this->reclaim_thread_queue.end(); ++itr)
	{
		(*itr)->join();
	}
	it = totally_failed_node_list.begin();
	for( ; it != totally_failed_node_list.end(); ++it)
	{
		OSDLOG(INFO, "Node " << *it << " could not be reclaimed, initiating its recovery");
		this->node_status_list_after_reclaim.push_back(std::make_pair(*it, system_state::NODE_RECOVERY));
	}
	it = stopping_node_list.begin();
	for( ; it != stopping_node_list.end(); ++it)
	{
		OSDLOG(INFO, "Node " << *it << " is in list of nodes being stopped");
		this->node_status_list_after_reclaim.push_back(std::make_pair(*it, system_state::NODE_DELETION));
	}
	int count = 0;
	std::vector<std::pair<std::string, int> >::iterator itr1 = this->node_status_list_after_reclaim.begin();
	for(; itr1 != this->node_status_list_after_reclaim.end(); ++itr1)
	{
		if((*itr1).second == system_state::COMPONENT_BALANCING)
			count++;
	}
	if(count != 0)
	{
		this->states_thread_mgr->prepare_and_add_entries(this->node_status_list_after_reclaim);
	}
	else
	{
		OSDLOG(INFO, "Since no node is in healthy state, so no need to initiate balancing");
	}
	this->monitoring->reset_cluster_recovery_details();
}

void ClusterRecoveryInitiator::send_node_reclaim_request(std::string node_id)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3(
		new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd = rhandler3->get_record_from_id(node_id);
	std::string node_ip = node_info_file_rcrd->get_node_ip();
	int32_t node_port = node_info_file_rcrd->get_localLeader_port();
	boost::shared_ptr<Communication::SynchronousCommunication> sync_ll_obj(
		new Communication::SynchronousCommunication(node_ip, node_port)
	);
	if(sync_ll_obj->is_connected())
	{
		boost::shared_ptr<comm_messages::NodeAdditionGl> na_gl_ptr(
						new comm_messages::NodeAdditionGl(node_id)
					);
		OSDLOG(INFO, "Sending node reclaim request to LL: (" << node_ip << ", " << node_port << ")");
		SuccessCallBack callback_obj;
		MessageExternalInterface::sync_send_node_addition_to_ll(
				na_gl_ptr.get(),
				sync_ll_obj, 
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				0
				//this->node_add_response_timeout
		);

		boost::shared_ptr<MessageResult<comm_messages::NodeAdditionGlAck> > ack_frm_ll_msg =
				MessageExternalInterface::sync_read_node_addition_ack_from_ll(sync_ll_obj, this->node_add_response_timeout);
		if(ack_frm_ll_msg->get_return_code() == Communication::SUCCESS)
		{
			if(ack_frm_ll_msg->get_message_object()->get_error_status()->get_code() == OSDException::OSDSuccessCode::NODE_ADDITION_SUCCESS)
			{
				OSDLOG(INFO, "Rcvd Node Reclaim success from LL");
				bool success = this->monitoring->change_status_in_monitoring_component(node_id, network_messages::REGISTERED);	
				if(success)
				{
					boost::shared_ptr<comm_messages::NodeAdditionFinalAck> final_ack_ptr(
							new comm_messages::NodeAdditionFinalAck(true)
							);
					SuccessCallBack callback_obj1;
					MessageExternalInterface::sync_send_node_add_final_ack(
						final_ack_ptr.get(),
						sync_ll_obj,
						boost::bind(&SuccessCallBack::success_handler, &callback_obj1, _1),
						boost::bind(&SuccessCallBack::failure_handler, &callback_obj1),
						60
					);
					this->node_status_list_after_reclaim.push_back(std::make_pair(node_id, system_state::COMPONENT_BALANCING));
					OSDLOG(INFO, "Node Reclaim Success for node: " << node_id);
				}
				
			}
			else
			{
				OSDLOG(INFO, "Rcvd Node Reclaim failure from LL");
				this->node_status_list_after_reclaim.push_back(std::make_pair(node_id, system_state::NODE_RECOVERY));
				this->monitoring->change_status_in_monitoring_component(node_id, network_messages::FAILED);
			}
		}
		else
		{
			OSDLOG(INFO, "Couldn't rcv Node Rejoin Ack from LL");
			this->node_status_list_after_reclaim.push_back(std::make_pair(node_id, system_state::NODE_RECOVERY));
			this->monitoring->change_status_in_monitoring_component(node_id, network_messages::FAILED);
		}
	}	
	else
	{
		OSDLOG(ERROR, "Could not connect to node " << node_ip);
		this->node_status_list_after_reclaim.push_back(std::make_pair(node_id, system_state::NODE_RECOVERY));
		this->monitoring->change_status_in_monitoring_component(node_id, network_messages::FAILED);


	}
}

} // namespace cluster_recovery_component
