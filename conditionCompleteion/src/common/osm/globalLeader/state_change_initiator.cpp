#include <iostream>
#include <poll.h>
#include <errno.h>

#include "communication/callback.h"
#include "osm/globalLeader/state_change_initiator.h"
#include "osm/globalLeader/component_manager.h"
#include "osm/globalLeader/journal_writer.h"
#include "communication/message_type_enum.pb.h"
#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/globalLeader/journal_reader.h"

using namespace object_storage_log_setup;
using namespace objectStorage;
namespace system_state
{

StateChangeMessageHandler::StateChangeMessageHandler(
	boost::shared_ptr<StateChangeThreadManager> state_thread_mgr,
	boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring
):
	state_thread_mgr(state_thread_mgr),
	component_map_mgr(component_map_mgr),
	monitoring(monitoring),
	stop(false)
{
}

void StateChangeMessageHandler::push(boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj)
{
	boost::mutex::scoped_lock lock(this->mtx);
	this->state_queue.push(msg_comm_obj);
	this->condition.notify_one();
}

boost::shared_ptr<MessageCommunicationWrapper> StateChangeMessageHandler::pop()
{
	boost::mutex::scoped_lock lock(this->mtx);
	while(this->state_queue.empty() and (!this->stop))
	{
		this->condition.wait(lock);
	}
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj;
	if(!this->state_queue.empty())
	{
		msg_comm_obj = this->state_queue.front();
		this->state_queue.pop();
	}
	return msg_comm_obj;
}

void StateChangeMessageHandler::abort()
{
	this->stop = true;
	this->condition.notify_one();
}

void StateChangeMessageHandler::handle_msg()
{
	OSDLOG(INFO, "State Change Msg Handler Thread Start");
	while(1)
	{
		boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj = this->pop();
		if(msg_comm_obj)
		{
			const uint32_t msg_type = msg_comm_obj->get_msg_result_wrap()->get_type();
			switch(msg_type)
			{
				case messageTypeEnum::typeEnums::GET_GLOBAL_MAP:
					this->component_map_mgr->send_get_global_map_ack(msg_comm_obj->get_comm_object());
					break;
				case messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT:
				{
					OSDLOG(INFO, "Get Service Components message received");
					boost::shared_ptr<comm_messages::GetServiceComponent> serv_comp_ptr(
											new comm_messages::GetServiceComponent);
					serv_comp_ptr->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					GLUtils util_obj;
					std::string service_id = serv_comp_ptr->get_service_id();
					std::string node_id = util_obj.get_node_name(service_id);
					OSDLOG(INFO, "Component ownership requested by: " << serv_comp_ptr->get_service_id());
					int node_status = this->monitoring->get_node_status(node_id);
					boost::shared_ptr<ErrorStatus> error_status;
					std::vector<uint32_t> owner_list;
					if(node_status == network_messages::FAILED)
					{
						OSDLOG(INFO, "Node found in failed state in Node Info File");
						error_status.reset(new ErrorStatus(10, "RECOVERY for this node in progress"));
					}
					else if(node_status == gl_monitoring::NODE_NOT_FOUND)
					{
						OSDLOG(INFO, "Node not found in Node Info File");
						error_status.reset(new ErrorStatus(
									0, "Node in new state. Balancing will begin after registration"));
					}
					else
					{
						owner_list = this->component_map_mgr->get_component_ownership(serv_comp_ptr->get_service_id());
					
						if(owner_list.empty())
						{
							if(node_status == network_messages::RECOVERED)
							{
								OSDLOG(INFO, "Node found in recovered state in Node Info File");
								error_status.reset(new ErrorStatus(0, "Node in recovered state. Balancing will begin now"));
								//error_status.reset(new ErrorStatus(10, "Node in recovered state. Balancing will begin after rcvng NODE_REJOIN")); //Services would be restarted after node_rejoin
							/*	this->state_thread_mgr->add_service_entry_for_balancing(
												service_id,
												NODE_ADDITION
											);
							*/	//TODO: Change status to REG in Monitoring component and node info file

								//initiate balancing
								//send positive ack to service
							}

							else if(node_status == network_messages::FAILED)
							{
								OSDLOG(INFO, "Node found in failed state in Node Info File");
								error_status.reset(new ErrorStatus(10, "RECOVERY for this node in progress"));
							}
							else 
							{
								OSDLOG(INFO, "Node found in Node Info File");
								error_status.reset(new ErrorStatus(0, "This is probably a new node"));
							}

						}	
						else	//in case of non 0 no of components
						{
							OSDLOG(INFO, "Returning ownership to service");
							error_status.reset(new ErrorStatus(0, "Returning components"));
							//send positive ack to service
						}
					}
					boost::shared_ptr<comm_messages::GetServiceComponentAck> ownership_ack_ptr(
								new comm_messages::GetServiceComponentAck(owner_list, error_status));
					this->component_map_mgr->send_get_service_component_ack(
								ownership_ack_ptr, msg_comm_obj->get_comm_object());
				}
					break;
				case messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING:
					this->state_thread_mgr->update_state_table(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::COMP_TRANSFER_INFO:
					this->state_thread_mgr->update_state_table(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT:
					this->state_thread_mgr->update_state_table(msg_comm_obj);
					break;
				case messageTypeEnum::typeEnums::NODE_FAILOVER:
				{
					boost::shared_ptr<comm_messages::NodeFailover> node_fail_msg(new comm_messages::NodeFailover);
					node_fail_msg->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					OSDLOG(INFO, "Node Failover msg rcvd from node " << node_fail_msg->get_node_id());
					int node_status = this->monitoring->get_node_status(node_fail_msg->get_node_id());
					bool success;
					
					if(node_status == network_messages::RETIRED or node_status == network_messages::RETIRED_RECOVERED or node_status == network_messages::INVALID_NODE)
					{
						success = false;
					}
					else 
					{
						if(node_status == network_messages::RETIRING)
						{
							this->monitoring->change_status_in_monitoring_component(node_fail_msg->get_node_id(), network_messages::RETIRING_FAILED);
						}
						else
						{
							this->monitoring->change_status_in_monitoring_component(node_fail_msg->get_node_id(), network_messages::FAILED);
						}
						success = this->state_thread_mgr->prepare_and_add_entry(node_fail_msg->get_node_id(), NODE_RECOVERY);
					}
					this->state_thread_mgr->send_node_failover_msg_ack(success, msg_comm_obj->get_comm_object());
				}
					break;
				case messageTypeEnum::typeEnums::NODE_STOP_LL:
				{
					boost::shared_ptr<comm_messages::NodeStopLL> node_stop_msg(new comm_messages::NodeStopLL);
					node_stop_msg->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					OSDLOG(INFO, "Node Stop Msg rcvd from node " << node_stop_msg->get_node_info()->get_service_id());
					int node_status = this->monitoring->get_node_status(node_stop_msg->get_node_info()->get_service_id());
					bool status = false;
					if(node_status == network_messages::REGISTERED)
					{
						//this->monitoring->change_status_in_memory(node_stop_msg->get_node_info()->get_service_id(), network_messages::NODE_STOPPING);
						//Writing NODE_STOPPING persistantly in Node Info File
						this->monitoring->change_status_in_monitoring_component(node_stop_msg->get_node_info()->get_service_id(), network_messages::NODE_STOPPING);
						status = this->state_thread_mgr->prepare_and_add_entry(node_stop_msg->get_node_info()->get_service_id(), NODE_DELETION);
					}
					this->state_thread_mgr->send_node_stop_msg_ack(node_stop_msg->get_node_info()->get_service_id(), msg_comm_obj->get_comm_object(), node_status, status);

				}
					break;
				case messageTypeEnum::typeEnums::NODE_RETIRE:
				{
					//Start balancing
					boost::shared_ptr<comm_messages::NodeRetire> ret_msg(new comm_messages::NodeRetire);
					ret_msg->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					OSDLOG(INFO, "Node Retire Msg rcvd from node " << ret_msg->get_node()->get_service_id());
					int node_status = this->monitoring->get_node_status(ret_msg->get_node()->get_service_id());
					bool success;
					if(node_status == network_messages::RETIRED or node_status == network_messages::RETIRED_RECOVERED)
					{
						success = true;
					}
					else if(node_status == network_messages::INVALID_NODE or node_status == network_messages::FAILED)
					{
						success = false;
					}
					else if(node_status == network_messages::RECOVERED)
					{
						success = true;
						this->monitoring->change_status_in_monitoring_component(ret_msg->get_node()->get_service_id(), network_messages::RETIRED_RECOVERED);
					}
					else
					{
						OSDLOG(INFO, "Node status is registered. Changing it to RETIRING.");
						success = this->state_thread_mgr->prepare_and_add_entry(
								ret_msg->get_node()->get_service_id(), NODE_DELETION);
						if(success)
						{
							this->monitoring->change_status_in_monitoring_component(ret_msg->get_node()->get_service_id(), network_messages::RETIRING);
						}
					}
					this->state_thread_mgr->send_node_retire_msg_ack(success, msg_comm_obj->get_comm_object(), ret_msg->get_node()->get_service_id());
				}
					break;
				default:
					OSDLOG(ERROR, "Wrong msg received in StateChangeMessageHandler thread");
					break;
			}
		}
		else
		{
			OSDLOG(INFO, "State Change Message Dispatcher Thread exiting");
			break;
		}
	}
}

StateChangeInitiatorMap::StateChangeInitiatorMap(
	boost::shared_ptr<recordStructure::RecoveryRecord> record,
	int64_t fd,
	bool balancing_planned
):
	record(record),
	fd(fd),
	balancing_planned(false)
{
}

StateChangeInitiatorMap::StateChangeInitiatorMap(
	boost::shared_ptr<recordStructure::RecoveryRecord> record,
	int64_t fd,
	time_t last_updated_time,
	int timeout,
	bool balancing_planned,
	int hrbt_failure_count
):
	record(record),
	fd(fd),
	last_updated_time(last_updated_time),
	timeout(timeout),
	balancing_planned(false),
	hrbt_failure_count(hrbt_failure_count)
{
}

bool StateChangeInitiatorMap::if_planned()
{
	return this->balancing_planned;
}

void StateChangeInitiatorMap::set_planned()
{
	this->balancing_planned = true;
}

void StateChangeInitiatorMap::reset_planned()
{
	this->balancing_planned = false;
}

boost::shared_ptr<recordStructure::RecoveryRecord> StateChangeInitiatorMap::get_recovery_record()
{
	return this->record;
}

void StateChangeInitiatorMap::set_fd(int64_t fd)
{
	this->fd = fd;
}

int64_t StateChangeInitiatorMap::get_fd()
{
	return this->fd;
}

void StateChangeInitiatorMap::set_expected_msg_type(int type)
{
	this->expected_msg_type = type;
}

void StateChangeInitiatorMap::set_timeout(int timeout_value)
{
	this->timeout = timeout_value;
}

int StateChangeInitiatorMap::get_timeout()
{
	return (this->timeout*this->hrbt_failure_count);
}

int StateChangeInitiatorMap::get_expected_msg_type()
{
	return this->expected_msg_type;
}

time_t StateChangeInitiatorMap::get_last_updated_time()
{
	return this->last_updated_time;
}

void StateChangeInitiatorMap::set_last_updated_time(time_t last_updated_time)
{
	this->last_updated_time = last_updated_time;
}

/* Only used while recovery from GL journal */
void StateChangeThreadManager::add_entry_in_map_while_recovery(
	std::vector<std::string> &nodes_failed,
	std::vector<std::string> &nodes_retired
)
{
	time_t now;
	time(&now);
	std::vector<std::string>::iterator itr = nodes_failed.begin();
	
	for(; itr != nodes_failed.end(); ++itr)
	{
		this->prepare_and_add_entry(*itr, NODE_RECOVERY);
	}

	itr = nodes_retired.begin();

	for(; itr != nodes_retired.end(); ++itr)
	{
		this->prepare_and_add_entry(*itr, NODE_DELETION);
	}

}

/* Used for adding entries for node addn/delen/recovery in map as well as journal */
/* This interface is now obsolete, since no entries on journal are being added */
//UNCOVERED_CODE_BEGIN:
void StateChangeThreadManager::add_entry_for_recovery(
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records,
	int operation
)
{
	bool ret_value = this->check_and_add_entry_in_map(node_rec_records, operation);
	if ( ret_value and operation == NODE_RECOVERY)
	{	
//		this->journal_lib_ptr->add_entry_in_journal(node_rec_records, boost::bind(&StateChangeThreadManager::notify, this));
//		this->wait_for_journal_write();
	}
}
//UNCOVERED_CODE_END

void StateChangeThreadManager::send_node_stop_msg_ack(
	std::string node_id,
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj,
	int node_status,
	bool status
)
{
	OSDLOG(INFO, "Sending node stop request ack to LL");
	boost::shared_ptr<ErrorStatus> err;
	if(status)
	{
		err.reset(new ErrorStatus(0, "Node registered for stop"));
	}
	else
	{
		err.reset(new ErrorStatus(10, "Node in in-consistent state to stop"));
	}
	boost::shared_ptr<comm_messages::NodeStopLLAck> stop_ack(new comm_messages::NodeStopLLAck(node_id, err, node_status));
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_node_stop_ack(
				stop_ack.get(),
				comm_obj,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				30
				);
}

void StateChangeThreadManager::send_node_failover_msg_ack(
	bool success,
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj
)
{
	OSDLOG(INFO, "Sending node failover request ack from GL");
	boost::shared_ptr<ErrorStatus> err;
	if(success)
	{
		err.reset(new ErrorStatus(0, "Node registered for failover"));
	}
	else
	{
		err.reset(new ErrorStatus(1, "Could not add node for failover"));
	}
	boost::shared_ptr<comm_messages::NodeFailoverAck> failover_ack(new comm_messages::NodeFailoverAck(err));
	SuccessCallBack callback_obj;

	MessageExternalInterface::sync_send_node_failover_ack(
				failover_ack.get(),
				comm_obj,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				30
				);
}

void StateChangeThreadManager::send_node_retire_msg_ack(
	bool success,
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj,
	std::string node_id
)
{
	OSDLOG(INFO, "Sending node retire ack from GL");
	boost::shared_ptr<ErrorStatus> err;
	if(success)
	{
		err.reset(new ErrorStatus(0, "Node registered for retiring"));
	}
	else
	{
		err.reset(new ErrorStatus(1, "Could not add node for retiring"));
	}
	boost::shared_ptr<comm_messages::NodeRetireAck> retire_ack(new comm_messages::NodeRetireAck(node_id, err));
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_node_retire_ack(
				retire_ack.get(),
				comm_obj,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				30
				);

}

void StateChangeThreadManager::start_recovery(
	std::vector<std::string> &nodes_registered,
        std::vector<std::string> &nodes_failed,
	std::vector<std::string> &nodes_retired,
	std::vector<std::string> &nodes_recovered,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
)
{
	boost::shared_ptr<gl_journal_read_handler::GlobalLeaderRecovery> gl_recovery_ptr(
			new gl_journal_read_handler::GlobalLeaderRecovery(
				this->journal_path,
				this->component_map_mgr,
				parser_ptr,
				nodes_failed
			)
		);

	try
	{
		gl_recovery_ptr->perform_recovery();				//In this TMAP will be prepared	
	}
	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(DEBUG, "Global Leader Journal File not found");
		this->journal_lib_ptr->get_journal_writer()->set_active_file_id(gl_recovery_ptr->get_next_file_id());
		OSDLOG(DEBUG, "Journal file id set is : " << gl_recovery_ptr->get_next_file_id());
	}

	this->component_map_mgr->recover_from_component_info_file();
	this->component_map_mgr->merge_basic_and_transient_map();	//Flush Transient and Global Map on Component Info File
	this->component_map_mgr->update_object_component_info_file(nodes_registered, nodes_failed);

	this->journal_lib_ptr->get_journal_writer()->set_active_file_id(gl_recovery_ptr->get_next_file_id());
	gl_recovery_ptr->clean_journal_files();
	this->add_entry_in_map_while_recovery(nodes_failed, nodes_retired);
	std::vector<std::string> unhealthy_node_list;
	unhealthy_node_list.reserve(nodes_failed.size() + nodes_retired.size());
	unhealthy_node_list.insert(unhealthy_node_list.end(), nodes_failed.begin(), nodes_failed.end());
	unhealthy_node_list.insert(unhealthy_node_list.end(), nodes_retired.begin(), nodes_retired.end());
	this->component_balancer_ptr->set_unhealthy_node_list(unhealthy_node_list);
	this->component_balancer_ptr->set_recovered_node_list(nodes_recovered);
	this->component_balancer_ptr->set_healthy_node_list(nodes_registered);
	this->initiate_component_balancing(nodes_registered, nodes_failed, nodes_retired);

}

void StateChangeThreadManager::initiate_component_balancing(
	std::vector<std::string> &nodes_registered,
        std::vector<std::string> &nodes_failed,
	std::vector<std::string> &nodes_retired
)
{
	this->initiate_component_balancing_for_service(CONTAINER_SERVICE, nodes_registered, nodes_failed, nodes_retired);
	this->initiate_component_balancing_for_service(ACCOUNT_SERVICE, nodes_registered, nodes_failed, nodes_retired);
	this->initiate_component_balancing_for_service(ACCOUNT_UPDATER, nodes_registered, nodes_failed, nodes_retired);
}

void StateChangeThreadManager::initiate_component_balancing_for_service(
	std::string service,
	std::vector<std::string> &nodes_registered,
        std::vector<std::string> &nodes_failed,
	std::vector<std::string> &nodes_retired
)
{
	std::vector<std::string> services_registered;
	std::vector<std::string> services_failed;
	std::vector<std::string> services_retired;
	std::vector<std::string>::iterator it = nodes_registered.begin();
	for(; it != nodes_registered.end(); ++it)
	{
		services_registered.push_back((*it) + "_" + service);

	}
	for(it = nodes_failed.begin(); it != nodes_failed.end(); ++it)
	{
		services_failed.push_back((*it) + "_" + service);
	}
	for(it = nodes_retired.begin(); it != nodes_retired.end(); ++it)
	{
		services_retired.push_back((*it) + "_" + service);
	}

	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> node_info_file_reader;
	node_info_file_reader.reset(new osm_info_file_manager::NodeInfoFileReadHandler(
		this->node_info_file_path,
		boost::bind(
			&component_balancer::ComponentBalancer::push_node_info_file_records_into_memory, this->component_balancer_ptr, _1
			)
		)
	);
	node_info_file_reader->recover_record(); //brings node_info_file records into memory
	OSDLOG(DEBUG, "Services registered list size: " << services_registered.size());
	OSDLOG(DEBUG, "Services retired list size: " << services_retired.size());
	OSDLOG(DEBUG, "Services failed list size: " << services_failed.size());
	std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> new_planned_map_pair = 
					this->component_balancer_ptr->balance_osd_cluster(
						service,
						services_registered,
						services_retired,
						services_failed
						);
	std::vector<std::string> services_added; 	//This will always be passed empty to modify_sci_map_for_new_plan interface
	OSDLOG(INFO, "New planned map size returned by Balancer is " << new_planned_map_pair.first.size());
	if(new_planned_map_pair.second)
		this->modify_sci_map_for_new_plan(new_planned_map_pair.first, services_added, services_retired, services_failed);
}

void StateChangeThreadManager::notify()
{
	boost::mutex::scoped_lock lock(this->mtx);
	this->work_done = true;
	this->condition.notify_one();
}

void StateChangeThreadManager::wait_for_journal_write()
{
	boost::mutex::scoped_lock lock(this->mtx);
	while(!this->work_done)
	{
		this->condition.wait(lock);	
	}
	this->work_done = false;
}

bool StateChangeThreadManager::check_if_node_can_be_retired(
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records
)
{
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> >::iterator it = node_rec_records.begin();
	for(; it != node_rec_records.end(); ++it)
	{
		std::string service_id = (*it)->get_state_change_node();
		if(this->state_map.find(service_id) != this->state_map.end())
			return false;
	}
	return true;
}

bool StateChangeThreadManager::check_and_add_entry_in_map(
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records,
	int operation
)
{
	OSDLOG(INFO, "Adding entry in state map");
	boost::mutex::scoped_lock lock(this->mtx);
	bool ready_to_retire = true;

	if(operation == NODE_DELETION)
	{
		ready_to_retire = this->check_if_node_can_be_retired(node_rec_records);
	}

	if(!ready_to_retire)
		return ready_to_retire;
	return this->add_entry_in_map(node_rec_records);
}

bool StateChangeThreadManager::add_entry_in_map(
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records
)
{
	time_t now;
	time(&now);
	GLUtils util_obj;
	bool ret_value = true, force_change = false;
	std::string node_id;
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> >::iterator it = node_rec_records.begin();

	std::pair<std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator,bool> ret;

	for ( ; it != node_rec_records.end(); ++it )
	{
		OSDLOG(INFO, "State Change node while adding entry in map: " << (*it)->get_state_change_node() << " time: " << now);
		node_id = util_obj.get_node_id((*it)->get_state_change_node());

		ret = this->state_map.insert(std::make_pair((*it)->get_state_change_node(),
				boost::shared_ptr<StateChangeInitiatorMap>(
					new StateChangeInitiatorMap(
						*it, -1, now, this->strm_timeout, false, this->hrbt_failure_count)
						)
					)
				);
		if ( !ret.second )	//TODO may become incorrect in case some node is added for na, but failure entry comes fro m HML
		{
			OSDLOG(WARNING, "State Change Process is already in progress for service id " << (*it)->get_state_change_node());

			std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator itr1 =
									this->state_map.find((*it)->get_state_change_node());

			if( (itr1->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING or
			itr1->second->get_recovery_record()->get_operation() == NODE_ADDITION or
			itr1->second->get_recovery_record()->get_operation() == NODE_DELETION) and (*it)->get_operation() == NODE_RECOVERY )
			{
				OSDLOG(INFO, "Changing operation of " << itr1->first << " to NODE_RECOVERY");
				itr1->second->get_recovery_record()->set_operation(NODE_RECOVERY);
				itr1->second->get_recovery_record()->set_status(RECOVERY_NEW);
				itr1->second->reset_planned();
				ret_value = true;
				force_change = true;
			}
			else if( (itr1->second->get_recovery_record()->get_operation() == (*it)->get_operation()) )
			{
				ret_value = true;
			}
			else
			{
				ret_value = false;
			}
		}

	}

	if(force_change)
	{
		//Change entry of PS for ND if exists
		std::string proxy_id = node_id + "_" + PROXY_SERVICE;
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator itr1 = this->state_map.find(proxy_id);
		if(itr1 != this->state_map.end() and 
						itr1->second->get_recovery_record()->get_operation() == NODE_DELETION)
		{
			OSDLOG(INFO, "Changing operation type of Proxy to NODE_RECOVERY");
			itr1->second->get_recovery_record()->set_operation(NODE_RECOVERY);
			itr1->second->get_recovery_record()->set_status(RECOVERY_CLOSED);
		}
	}

	OSDLOG(DEBUG, "Ret value is " << ret_value);
	return ret_value;
}

bool StateChangeThreadManager::send_ack_for_rec_strm(
	std::string service_id,
	boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr,
	bool success
)
{
	std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map;
	recordStructure::ComponentInfoRecord comp_obj;
	if(success)
	{
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_plan = it->second->get_recovery_record()->get_planned_map();
		service_map = this->get_map_from_tuple(comp_plan);
		GLUtils util_obj;
		std::string node_id = util_obj.get_node_id(it->second->get_recovery_record()->get_state_change_node());
		boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3(
						new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path));
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd =
			rhandler3->get_record_from_id(node_id);
		if(node_info_file_rcrd)
		{
			comp_obj.set_service_id(node_info_file_rcrd->get_node_id());
			comp_obj.set_service_ip(node_info_file_rcrd->get_node_ip());
			comp_obj.set_service_port(node_info_file_rcrd->get_localLeader_port());
			OSDLOG(DEBUG, "Fetched service object for failed node: " << comp_obj.get_service_ip() << ", " << comp_obj.get_service_port());
		}
		else
		{
			success = false;
			OSDLOG(WARNING, "Could not find failed node id in Node Info File");
		}
	}
	OSDLOG(INFO, "Sending " << success << " as RSTRM status");
	boost::shared_ptr<comm_messages::RecvProcStartMonitoringAck> rp_ack(new comm_messages::RecvProcStartMonitoringAck(service_map, comp_obj, success));
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_recovery_proc_start_monitoring_ack(
			rp_ack.get(),
			comm_ptr,
			boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
			boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
			30
		);
	OSDLOG(INFO, "Sent recovery start mon ack with status: " << callback_obj.get_result());
	return callback_obj.get_result();
}

bool StateChangeThreadManager::update_map(boost::shared_ptr<MessageCommunicationWrapper> msg)
{
	OSDLOG(DEBUG, "update_map called for StateChangeThreadManager");
	bool status = true;
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it;
	std::string service_id;
	std::vector<boost::shared_ptr<recordStructure::Record> > node_rec_records;
	int type = msg->get_msg_result_wrap()->get_type();
	switch(type)
	{
		case messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING:
			{
				OSDLOG(INFO, "Received RECV_PROC_START_MONITORING message");
				boost::shared_ptr<comm_messages::RecvProcStartMonitoring> decoded_msg(new comm_messages::RecvProcStartMonitoring);
				decoded_msg->deserialize(msg->get_msg_result_wrap()->get_payload(), msg->get_msg_result_wrap()->get_size());
				service_id = decoded_msg->get_proc_id();
				OSDLOG(INFO, "Rec STRM rcvd for " << service_id);
				boost::mutex::scoped_lock lock(this->mtx);
				it = this->state_map.find(service_id);
				if(it != this->state_map.end())
				{
					it->second->get_recovery_record()->set_status(RECOVERY_IN_PROGRESS);
					node_rec_records.push_back(it->second->get_recovery_record());
					status = this->send_ack_for_rec_strm(service_id, msg->get_comm_object(), true);

				}
				else
				{
					OSDLOG(INFO, "Recovery Start Mon is not expected for this node : " << service_id);
					status = this->send_ack_for_rec_strm(service_id, msg->get_comm_object(), false);
				}
				break;
			}
		case messageTypeEnum::typeEnums::COMP_TRANSFER_INFO:
			{
				boost::shared_ptr<comm_messages::CompTransferInfo> decoded_msg(new comm_messages::CompTransferInfo);
				decoded_msg->deserialize(msg->get_msg_result_wrap()->get_payload(), msg->get_msg_result_wrap()->get_size());
				service_id = decoded_msg->get_service_id();
				OSDLOG(INFO, "Intermediate comp transfer response rcvd from: " << service_id);
				//boost::mutex::scoped_lock lock(this->mtx);
				this->mtx.lock();
				it = this->state_map.find(service_id);
				if(it != this->state_map.end())
				{
					this->mtx.unlock();
					GLUtils util_obj;
					std::string service_name = util_obj.get_service_name(service_id);
					std::pair<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, bool>
						actual_transferred_comp = this->component_map_mgr->update_transient_map(
								service_name,
								decoded_msg->get_comp_service_list()
								);
					this->journal_lib_ptr->prepare_transient_record_for_journal(service_id, node_rec_records);

					this->journal_lib_ptr->add_entry_in_journal(node_rec_records, boost::bind(&StateChangeThreadManager::notify, this));
					this->wait_for_journal_write();
					this->mtx.lock();
					//it->second->get_recovery_record()->remove_from_planned_map(decoded_msg->get_comp_service_list());
					it->second->get_recovery_record()->remove_from_planned_map(actual_transferred_comp.first);
					this->mtx.unlock();
					if(actual_transferred_comp.second)
						status = this->component_map_mgr->send_ack_for_comp_trnsfr_msg(
								msg->get_comm_object(),
								messageTypeEnum::typeEnums::COMP_TRANSFER_INFO_ACK
								);
					if(!status)
					{
						//May be try again
					}
				}
				else
				{
					this->mtx.unlock();
				}
				break;
			}
		case messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT:	//This will be first msg only when none of the destinations for component transfer were available(will be sent by only RP)
			{
				boost::shared_ptr<comm_messages::CompTransferFinalStat> decoded_msg(new comm_messages::CompTransferFinalStat);
				decoded_msg->deserialize(msg->get_msg_result_wrap()->get_payload(), msg->get_msg_result_wrap()->get_size());
				service_id = decoded_msg->get_service_id();
				OSDLOG(INFO, "Comp transfer final response rcvd from: " << service_id);
				this->mtx.lock();
				it = this->state_map.find(service_id);
				if(it != this->state_map.end())
				{
					GLUtils util_obj;
					std::string service_name = util_obj.get_service_name(service_id);
					if ( !it->second->get_recovery_record()->get_planned_map().empty() )
					{
						OSDLOG(INFO, "Planned map is not empty when encountered End Monitoring");
						if ( it->second->get_recovery_record()->get_operation() == NODE_RECOVERY )
						{
							it->second->get_recovery_record()->set_status(RECOVERY_ABORTED);
						}
					}
				}
				this->mtx.unlock();
				this->component_map_mgr->send_ack_for_comp_trnsfr_msg(
							msg->get_comm_object(),
							messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT_ACK
							);
				break;
			}
		default:
			OSDLOG(INFO, "State Change Initiator rcvd invalid msg type for updating map");
			return false;
	}
	OSDLOG(INFO, "Updating SCI map for service id " << service_id << " after rcving msg: " << type);
	time_t now;
	time(&now);
	this->mtx.lock();
	if (it != this->state_map.end())
	{
		it->second->set_last_updated_time(now);
		it->second->set_timeout(this->hrbt_timeout);
		it->second->set_fd(msg->get_comm_object()->get_fd());
	}
	this->mtx.unlock();
//	this->journal_lib_ptr->add_entry_in_journal(node_rec_records, boost::bind(&StateChangeThreadManager::notify, this));
//	this->wait_for_journal_write();
	return status;
}


StateChangeThreadManager::StateChangeThreadManager(
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr,
	boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
	boost::function<bool(std::string, int)> change_status_in_monitoring_component
):
	component_map_mgr(component_map_mgr),
	work_done(false),
	change_status_in_monitoring_component(change_status_in_monitoring_component)
{
	this->journal_path = parser_ptr->journal_pathValue();
	this->journal_lib_ptr.reset(new global_leader::JournalLibrary(
						this->journal_path,
						this->component_map_mgr,
						boost::bind(&StateChangeThreadManager::get_state_change_ids, this)
					)
				);
	this->component_balancer_ptr.reset(
		new component_balancer::ComponentBalancer(component_map_mgr, parser_ptr)
	);
	this->node_info_file_path = parser_ptr->node_info_file_pathValue();
	this->journal_lib_ptr->get_journal_writer()->set_active_file_id(1);
	this->cont_rec_script_location = parser_ptr->container_recovery_script_locationValue();
	this->acc_rec_script_location = parser_ptr->account_recovery_script_locationValue();
	this->updater_rec_script_location = parser_ptr->updater_recovery_script_locationValue();
	this->object_rec_script_location = parser_ptr->object_recovery_script_locationValue();
	this->script_executor = parser_ptr->recovery_script_wrapperValue();
	this->umount_cmd = parser_ptr->UMOUNT_HYDRAFSValue();

	GLUtils util_obj;
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> parser_ptr1 = util_obj.get_osm_timeout_config();

	this->hrbt_timeout = parser_ptr1->heartbeat_timeoutValue();
	this->strm_timeout = parser_ptr1->ll_strm_timeoutValue();
	this->hrbt_failure_count = parser_ptr1->hrbt_failure_countValue();
	this->fs_mount_timeout = parser_ptr1->fs_mount_timeoutValue();
}

std::vector<std::string> StateChangeThreadManager::get_state_change_ids()
{
	boost::mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.begin();
	std::vector<std::string> state_change_ids;
	for ( ; it != this->state_map.end(); ++it )
	{
		state_change_ids.push_back(it->first);
	}
	return state_change_ids;
}

/* Called only if any service has 0 components */
void StateChangeThreadManager::add_service_entry_for_balancing(std::string service_id, int operation)
{
	int operation_status;
	if ( operation == NODE_ADDITION )
	{
		operation_status = NA_NEW;
	}
	else
	{
		return;
	}
	GLUtils util_obj;
	std::string service_name = util_obj.get_service_name(service_id);
	boost::shared_ptr<recordStructure::RecoveryRecord> add_record(
				new recordStructure::RecoveryRecord(
					operation,
					service_id,
					operation_status
					)
				);
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records;
	node_rec_records.push_back(add_record);
	this->check_and_add_entry_in_map(node_rec_records, operation);
	
}

//bool StateChangeThreadManager::refresh_states_for_cluster_recovery(std::vector<std::pair<std::string, int> > node_list)
bool StateChangeThreadManager::refresh_states_for_cluster_recovery()
{
	boost::mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.begin();
	OSDLOG(INFO, "Changing status of every failed node to RECOVERY_INTERRUPTED");
	for ( ; it != this->state_map.end(); ++it )
	{
		int state = it->second->get_recovery_record()->get_status();
		switch(state)
		{
				
			case RECOVERY_NEW:
			case RECOVERY_FAILED:
			case RECOVERY_ABORTED:
			case RECOVERY_PLANNED:
			case RECOVERY_IN_PROGRESS:
				this->clean_recovery_process(
					it->second->get_recovery_record()->get_state_change_node(),
					it->second->get_recovery_record()->get_destination_node()
				);

			case RECOVERY_COMPLETED:
			{
				GLUtils util_obj;
				std::string service_name = util_obj.get_service_name(it->first);
				std::string node_name = util_obj.get_node_name(it->first);
				if(service_name == CONTAINER_SERVICE)
				{
					if(!this->unmount_journal_filesystems(it->first))
					{
						OSDLOG(ERROR, "Unable to unmount journal filesystems of node " << it->first);
						return false;
					}
				}
				if(service_name == OBJECT_SERVICE)
				{
					this->component_map_mgr->remove_object_entry_from_map(it->first);
				}
				it->second->get_recovery_record()->set_status(RECOVERY_INTERRUPTED);
				break;
			}	
		}
	}
	this->component_balancer_ptr->reinstantiate_balancer();
	if(this->component_map_mgr->get_transient_map_size() != 0)
	{
		OSDLOG(INFO, "Merging transient and basic map in case of cluster recovery");
		this->component_map_mgr->merge_basic_and_transient_map();
	}
	this->state_map.clear();
	OSDLOG(INFO, "Cleared all states in state map");
	return true;
	//flush all state changes going on in the system
//	this->prepare_and_add_entries(node_list);
}

void StateChangeThreadManager::prepare_and_add_entries(std::vector<std::pair<std::string, int> > node_list)
{
	std::vector<std::pair<std::string, int> >::iterator it = node_list.begin();
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records;
	for(; it != node_list.end(); ++it)
	{
		int operation, operation_status;
		if((*it).second == system_state::NODE_RECOVERY)
		{
			operation = NODE_RECOVERY;
			operation_status = RECOVERY_NEW;
		}
		else
		{
			operation = NODE_ADDITION;
			operation_status = NA_NEW;
		}
		std::string node_id = (*it).first;
		std::string cont_service_id(CONTAINER_SUFFIX);
		std::string acc_service_id(ACCOUNT_SUFFIX);
		std::string updater_service_id(UPDATER_SUFFIX);
		boost::shared_ptr<recordStructure::RecoveryRecord> con_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + CONTAINER_SERVICE),
					operation_status
					)
				);
		boost::shared_ptr<recordStructure::RecoveryRecord> acc_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + ACCOUNT_SERVICE),
					operation_status
					)
				);
		boost::shared_ptr<recordStructure::RecoveryRecord> updater_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + ACCOUNT_UPDATER),
					operation_status
					)
				);
		boost::shared_ptr<recordStructure::RecoveryRecord> obj_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + OBJECT_SERVICE),
					operation_status
					)
				);
	
		node_rec_records.push_back(con_rec_record);
		node_rec_records.push_back(acc_rec_record);		//Commented only for testing purpose
		node_rec_records.push_back(obj_rec_record);
		node_rec_records.push_back(updater_rec_record);
	}

	this->add_entry_in_map(node_rec_records);
}

/* this will be called when new entry is added in State Change Table by GL_msg_handler or Monitoring */
bool StateChangeThreadManager::prepare_and_add_entry(std::string node_id, int operation)
{
	int operation_status;
	if ( operation == NODE_RECOVERY )
	{
		operation_status = RECOVERY_NEW;
	}
	else if ( operation == NODE_ADDITION )
	{
		operation_status = NA_NEW;
	}
	else if ( operation == NODE_DELETION )
	{
		operation_status = ND_NEW;
	}
	else
	{
		return false;
	}
	std::string cont_service_id(CONTAINER_SUFFIX);
	std::string acc_service_id(ACCOUNT_SUFFIX);
	std::string updater_service_id(UPDATER_SUFFIX);
	boost::shared_ptr<recordStructure::RecoveryRecord> con_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + CONTAINER_SERVICE),
					operation_status
					)
				);
	boost::shared_ptr<recordStructure::RecoveryRecord> acc_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + ACCOUNT_SERVICE),
					operation_status
					)
				);
	boost::shared_ptr<recordStructure::RecoveryRecord> updater_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + ACCOUNT_UPDATER),
					operation_status
					)
				);
	boost::shared_ptr<recordStructure::RecoveryRecord> obj_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + OBJECT_SERVICE),
					operation_status
					)
				);
	
	std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records;
	node_rec_records.push_back(con_rec_record);
	node_rec_records.push_back(acc_rec_record);		//Commented only for testing purpose
	node_rec_records.push_back(obj_rec_record);
	node_rec_records.push_back(updater_rec_record);
	if(operation == NODE_DELETION)		//In ND case, http requests need to be sent to all services
	{
		
		boost::shared_ptr<recordStructure::RecoveryRecord> proxy_rec_record(
				new recordStructure::RecoveryRecord(
					operation,
					(node_id + "_" + PROXY_SERVICE),
					operation_status
					)
				);
		node_rec_records.push_back(proxy_rec_record);
	}

//	this->add_entry_for_recovery(node_rec_records, operation);
	return this->check_and_add_entry_in_map(node_rec_records, operation);
}

void StateChangeThreadManager::create_bucket()
{
	boost::shared_ptr<Bucket> bucket_ptr(new Bucket());
	bucket_ptr->set_fd_limit(FAX_FD_LIMIT);
	this->bucket_fd_ratio.push_back(std::tr1::tuple<boost::shared_ptr<Bucket>, int64_t>(bucket_ptr, 0));
	boost::shared_ptr<StateChangeMonitorThread> mon_thread_ptr(
				new StateChangeMonitorThread(
					bucket_ptr,
					this->state_map,
					this->journal_lib_ptr,
					this->component_map_mgr,
					this->component_balancer_ptr,
					this->mtx
					)
				);
	boost::shared_ptr<boost::thread> mon_thread(new boost::thread(&StateChangeMonitorThread::run, mon_thread_ptr));
	this->state_map_monitor.push_back(mon_thread);
}

void StateChangeThreadManager::stop_all_state_mon_threads()
{
	std::vector<boost::shared_ptr<boost::thread> >::iterator thread_it = this->state_map_monitor.begin();
	for( ; thread_it != this->state_map_monitor.end(); ++thread_it )
	{
		(*thread_it)->interrupt();
		(*thread_it)->join();
	}
}

/* this will be external interface for handling strm or component transfer msg */
//void StateChangeThreadManager::update_state_table(boost::shared_ptr<message::Message> msg)
bool StateChangeThreadManager::update_state_table(boost::shared_ptr<MessageCommunicationWrapper> msg)
{
	bool status = this->update_map(msg);
	if(status)
	{
		OSDLOG(INFO, "Updated rcvd packet in State Change Table, adding fd in bucket now");
		if(this->bucket_fd_ratio.empty()) // TODO also check if bucket max limit reached
			this->create_bucket();
//		Communication::SynchronousCommunication *comm_obj = msg->get_comm_object();
//		boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr(comm_obj);
		this->update_bucket(msg->get_comm_object());
	}
	return status;
}

void StateChangeThreadManager::update_bucket(boost::shared_ptr<Communication::SynchronousCommunication> comm_obj)
{
	std::vector<std::tr1::tuple<boost::shared_ptr<Bucket>, int64_t> >::iterator it = this->bucket_fd_ratio.begin();
	for (; it != this->bucket_fd_ratio.end(); ++it)
	{
		if ((std::tr1::get<0>(*it))->get_fd_limit() > std::tr1::get<1>(*it))
		{
			break;
		}
	}
	if(it != this->bucket_fd_ratio.end())
	{
		(std::tr1::get<0>(*it))->add_fd(comm_obj->get_fd(), comm_obj);
	}
}

void StateChangeThreadManager::clean_recovery_process(std::string rec_process_src, std::string rec_process_destination)
{
	GLUtils util_obj;
//	recordStructure::ComponentInfoRecord destination_service_info_ptr = this->component_map_mgr->get_service_info(rec_process_destination);
	std::string dest_node_id = util_obj.get_node_id(rec_process_destination);
	//std::string rec_process_name = "RECOVERY_" + rec_process_destination;	//TODO: Create macro or global variable for same
	std::string rec_process_name = "RECOVERY_" + rec_process_src;	//TODO: Create macro or global variable for same
	CommandExecutor ex_obj;
	std::string cmd = "sudo -u hydragui hydra_ssh " + dest_node_id + " sudo pkill -9 -f " + rec_process_name;
	OSDLOG(INFO, "Cmd to clean recovery process: " << cmd);
	int ret = ex_obj.execute_command(cmd);
}

bool StateChangeThreadManager::start_node_recovery_process(std::string failed_node_id, std::string rec_process_destination)
{
	OSDLOG(INFO, "Starting recovery for " << failed_node_id);
	GLUtils util_obj;
//	boost::shared_ptr<recordStructure::ComponentInfoRecord> destination_service_info_ptr = this->component_map_mgr->get_service_object(rec_process_destination);
	std::string service_name = util_obj.get_service_name(failed_node_id);
	std::string script_loc;
	if(service_name == CONTAINER_SERVICE)
		script_loc = this->cont_rec_script_location;
	else if(service_name == ACCOUNT_SERVICE)
		script_loc = this->acc_rec_script_location;
	else if(service_name == ACCOUNT_UPDATER)
		script_loc = this->updater_rec_script_location;
	else
		script_loc = this->object_rec_script_location;

	std::string dest_node_id = util_obj.get_node_id(rec_process_destination);
	OSDLOG(INFO, "Cleaning in case any previous recovery process with same name exists");
	this->clean_recovery_process(failed_node_id, rec_process_destination);
	CommandExecutor ex_obj;

	std::string cmd = "sudo -u hydragui hydra_ssh " + dest_node_id + " sudo sh " + this->script_executor + " " + script_loc + " " + failed_node_id + " &";


	OSDLOG(INFO, "Executing command to fork rec process " << cmd);
	int retry_count = 3;
	bool success = false;
	while(retry_count != 0)
	{
		int ret = ex_obj.execute_command_in_bg(cmd);
		if(ret != 0)
		{
			OSDLOG(WARNING, "Unable to create recovery process");
		}
		else
		{
			success = true;
			break;
		}
		retry_count -= 1;
	}
	return success;
}

void StateChangeThreadManager::get_info_about_unhealthy_nodes(
	std::vector<std::string> &unhealthy_nodes,
	std::vector<std::string> &nodes_as_rec_destination
)
{
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.begin();
	for ( ; it != this->state_map.end(); ++it )
	{
		if(it->second->get_recovery_record()->get_operation() == NODE_RECOVERY)
		{
			GLUtils util_obj;
		
			unhealthy_nodes.push_back(util_obj.get_node_id(it->second->get_recovery_record()->get_state_change_node()));
			nodes_as_rec_destination.push_back(util_obj.get_node_id(it->second->get_recovery_record()->get_destination_node()));
		}
	}

}

/* interface to check if recovery/component transfer of all services for a node completed so that entry in node info file can be changed */
bool StateChangeThreadManager::check_if_recovery_completed_for_this_node(std::string node_name, int current_operation)	
{
	GLUtils util_obj;
	uint32_t complete_count = 0;
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.begin();
	int required_status;
	if(current_operation == NODE_RECOVERY)
		required_status = RECOVERY_CLOSED;
	else	// current_operation == NODE_DELETION
		required_status = ND_COMPLETED;
	std::vector<std::string> service_ids;
	for(; it != this->state_map.end(); ++it)
	{
		if((it->first).find(node_name) != std::string::npos)
		{
			if(it->second->get_recovery_record()->get_operation() == current_operation)
			{
				service_ids.push_back(it->first);
				if(it->second->get_recovery_record()->get_status() == required_status)
					complete_count++;
			}
		}
	}
	OSDLOG(INFO, complete_count << " services out of " << service_ids.size() << " have completed state change for node " << node_name);
	//node name is HN0101_61014
	int status;
	bool ret_val = false;
	if (current_operation == NODE_DELETION)
	{
		status = network_messages::NODE_STOPPED;
		// this is only in current phase, in future it will change
		// Remove this node from unhealthy node list
	}
	if (current_operation == NODE_RECOVERY)
	{
		status = network_messages::RECOVERED;
	}

	if(service_ids.size() == complete_count)
	//Recovery completed or Node deleted has been marked for every service of that node
	{
		OSDLOG(INFO, "State Change completed for node " << node_name);
		std::string node = util_obj.get_node_id(node_name);	//HN0101
		if(current_operation == NODE_RECOVERY)
		{
			CLOG(SYSMSG::OSD_SERVICE_RECOVER, node.c_str());
		}
		else if(current_operation == NODE_DELETION)     //When all the components of node being stopped have been transferred
		{
			CLOG(SYSMSG::OSD_SERVICE_STOP, node.c_str());
		}
		change_status_in_monitoring_component(node_name, status);
		std::vector<std::string>::iterator it1 = service_ids.begin();
		for(; it1 != service_ids.end(); ++it1)
		{
			it = this->state_map.find(*it1);
			if(it != this->state_map.end())
			{
				OSDLOG(DEBUG, "Removing entry " << *it1 << " from state map");
				ret_val = true;
				this->state_map.erase(it);
			}
		}
		this->component_balancer_ptr->remove_entry_from_unhealthy_node_list(node_name);
	}
	else
	{
		return ret_val;
	}
	if (current_operation == NODE_DELETION)
	{
		//Send STOP_SERVICES msg to LL
		//Remove object service from comp info file
		osm_info_file_manager::ComponentInfoFileWriteHandler info_obj(this->node_info_file_path);
		info_obj.delete_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, node_name + "_" + OBJECT_SERVICE);

		boost::shared_ptr<comm_messages::StopServices> stop_req(new comm_messages::StopServices(node_name));
		osm_info_file_manager::NodeInfoFileReadHandler read_hndlr(this->node_info_file_path);
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> service_info_ptr = read_hndlr.get_record_from_id(node_name);
		if(service_info_ptr)
		{
		std::string ip = service_info_ptr->get_node_ip();
		uint64_t port = service_info_ptr->get_localLeader_port();
		OSDLOG(INFO, "Sending Stop Service request to LL: " << ip << ", " << port);
		boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication(ip, port));
		SuccessCallBack callback_obj;
		MessageExternalInterface::sync_send_stop_services_to_ll(
				stop_req.get(),
				comm,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				30
				);
		}
		else
		{
			OSDLOG(ERROR, "Couldn't find LL IP Port");
		}

	}

	OSDLOG(DEBUG, "Returning from this function with ret val: " << ret_val);
	return ret_val;
}

std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > StateChangeThreadManager::get_map_from_tuple(
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_plan
)
{
	std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map;
	recordStructure::ComponentInfoRecord comp_obj;

	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator itr1 = comp_plan.begin();
	int i = 0;
	for(; itr1 != comp_plan.end(); ++itr1)
	{
		std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> >::iterator itr2 = service_map.find((*itr1).second);
		if(itr2 == service_map.end())
		{
			OSDLOG(DEBUG, "Pushing in map from tuple: " << (*itr1).first << ", " << (*itr1).second.get_service_ip() << ", " << (*itr1).second.get_service_port());
			//service_map.insert(std::make_pair((*itr1).second, (*itr1).first));
			service_map[(*itr1).second].push_back((*itr1).first);
		}
		else
		{
			itr2->second.push_back((*itr1).first);
		}
	i++;
	}
	return service_map;
}


/* Interface used for sending http request during NA and ND */
void StateChangeThreadManager::start_component_transfer(std::string source_service_id)
{
	OSDLOG(INFO, "Starting component transfer from " << source_service_id);
	boost::mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(source_service_id);
	try
	{
		if ( it != this->state_map.end() )
		{
			float global_map_version = this->component_map_mgr->get_global_map_version();

			std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map = this->get_map_from_tuple(it->second->get_recovery_record()->get_planned_map());
			this->mtx.unlock();
			std::map<std::string, std::string> dict;
			dict["X-GLOBAL-MAP-VERSION"] = boost::lexical_cast<std::string>(global_map_version);
			boost::shared_ptr<comm_messages::TransferComponents> trans_comp_ptr(
					new comm_messages::TransferComponents(
							service_map,
							dict
							)
						);
			boost::shared_ptr<recordStructure::ComponentInfoRecord> service_info_ptr =
					this->component_map_mgr->get_service_object(source_service_id);
			if(service_info_ptr)
			{
				OSDLOG(INFO, "Got service object from Global Map: " << service_info_ptr->get_service_ip() << ", " << service_info_ptr->get_service_port());
				boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr(
							new Communication::SynchronousCommunication(
									service_info_ptr->get_service_ip(),
									service_info_ptr->get_service_port()
									)
								);
				OSDLOG(DEBUG, "Established communication channel");
				boost::shared_ptr<MessageResult<comm_messages::TransferComponentsAck> > tc_ack = 
								MessageExternalInterface::send_transfer_component_http_request(
									comm_ptr,
									trans_comp_ptr.get(),
									service_info_ptr->get_service_ip(),
									service_info_ptr->get_service_port(),
									0
									);

				if(tc_ack->get_return_code() == Communication::SOCKET_NOT_CONNECTED)		//error
				{
					OSDLOG(WARNING, "Source not available. Return code of http req is: " << tc_ack->get_return_code());
					boost::mutex::scoped_lock lock(this->mtx);
					std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(source_service_id);

					if ( it != this->state_map.end() )
					{
						if ( it->second->get_fd() < 0 and 
							it->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING)
						{
							OSDLOG(INFO, "Setting status BALANCING_PLANNED");
							it->second->get_recovery_record()->set_status(BALANCING_PLANNED);
						}
						else if ( it->second->get_fd() < 0 and
							it->second->get_recovery_record()->get_operation() == NODE_DELETION)
						{
							OSDLOG(INFO, "Setting status ND_PLANNED");
							it->second->get_recovery_record()->set_status(ND_PLANNED);
						}
						it->second->reset_planned();
					}
				}
				else if(tc_ack->get_return_code() != 0)
				{
					OSDLOG(INFO, "Return code of http req is: " << tc_ack->get_return_code());
					boost::mutex::scoped_lock lock(this->mtx);
					std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(source_service_id);
					if ( it != this->state_map.end() )
					{
						if(it->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING)
						{
							if(it->second->get_recovery_record()->get_planned_map().empty())	
							{
								OSDLOG(INFO, "Setting status BALANCING_COMPLETED");
								it->second->get_recovery_record()->set_status(BALANCING_COMPLETED);
							}
							else
							{
								OSDLOG(INFO, "Setting status BALANCING_PLANNED");
								it->second->get_recovery_record()->set_status(BALANCING_PLANNED);
								it->second->reset_planned();
							}
						}
						else if(it->second->get_recovery_record()->get_operation() == NODE_DELETION)
						{
							if(it->second->get_recovery_record()->get_planned_map().empty())
							{
								OSDLOG(INFO, "Setting status ND_COMPLETED");
								it->second->get_recovery_record()->set_status(ND_COMPLETED);
							}
							else
							{
								it->second->reset_planned();
								if(it->second->get_fd() < 0)
								{
									OSDLOG(INFO, "Setting status ND_PLANNED");
									it->second->get_recovery_record()->set_status(ND_PLANNED);
								}
								else
								{
									OSDLOG(INFO, "Setting status ND_FAILED");
									it->second->get_recovery_record()->set_status(ND_FAILED);
								}
							}
						}
					}

				}
				else
				{
					OSDLOG(INFO, "Return code of http req is 0");
					boost::mutex::scoped_lock lock(this->mtx);
					std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(source_service_id);
					if ( it != this->state_map.end() )
					{
						if(it->second->get_recovery_record()->get_planned_map().size() == 0)
						{
							OSDLOG(DEBUG, "Setting status COMPLETED");
							if(it->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING)
								it->second->get_recovery_record()->set_status(BALANCING_COMPLETED);
							else if(it->second->get_recovery_record()->get_operation() == NODE_DELETION)
								it->second->get_recovery_record()->set_status(ND_COMPLETED);
						}
						else
						{
							if(it->second->get_fd() < 0 and
								it->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING)
							{
								OSDLOG(INFO, "Setting status PLANNED");
								//it->second->get_recovery_record()->set_status(BALANCING_PLANNED);
								it->second->get_recovery_record()->set_status(BALANCING_RESCHEDULE);
								it->second->reset_planned();
							}
							else if(it->second->get_fd() > 0 and
								it->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING)
							{
								OSDLOG(INFO, "Setting status ABORTED");
								it->second->get_recovery_record()->set_status(BALANCING_ABORTED);
								it->second->reset_planned();
							}
							else if(it->second->get_recovery_record()->get_operation() == NODE_DELETION)
							{
								OSDLOG(INFO, "Setting status ABORTED");
								it->second->get_recovery_record()->set_status(ND_ABORTED);
							}
						}
					}
				}
			
			}
			else
			{
				boost::mutex::scoped_lock lock(this->mtx);
				std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(source_service_id);
				if ( it != this->state_map.end() )
				{
					it->second->reset_planned();
				}
				OSDLOG(ERROR, "Could not find service info details in Basic or Transient Map");
			}
		}
	}
	catch(...)
	{
		OSDLOG(WARNING, "Exception raised in Balancing Thread");
		boost::mutex::scoped_lock lock(this->mtx);
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(source_service_id);
		if ( it != this->state_map.end() )
		{
			if ( it->second->get_recovery_record()->get_status() == BALANCING_PLANNED )
			{
				it->second->reset_planned();
			}
		}
	}
}

//Called only for Proxy and Object Service
void StateChangeThreadManager::block_service_request(std::string service_id)
{
	OSDLOG(INFO, "Requesting " << service_id << " to block new requests");
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
	try
	{
		if ( it != this->state_map.end() )
		{
			std::map<std::string, std::string> dict;

			GLUtils util_obj;
			std::string node_id = util_obj.get_node_name(service_id);
			boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3(
							new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path));
			boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd =
				rhandler3->get_record_from_id(node_id);
			if(node_info_file_rcrd)
			{
				std::string service_name = util_obj.get_service_name(service_id);
				int32_t port;
				if(service_name == PROXY_SERVICE)
				{
					port = node_info_file_rcrd->get_proxy_port();
					dict["X-GLOBAL-MAP-VERSION"] = "-1";
				}
				else if(service_name == OBJECT_SERVICE)
				{
					port = node_info_file_rcrd->get_objectService_port();
					float g_map_version = this->component_map_mgr->get_global_map_version();
					g_map_version = ceil(g_map_version);
					std::string version = boost::lexical_cast<std::string>(g_map_version);
					dict["X-GLOBAL-MAP-VERSION"] = version;
					OSDLOG(INFO, "Sending Global Map Version : " << version);
				}
				else
				{
					OSDLOG(INFO, "Invalid service name for calling stop service");
					return;
				}
				OSDLOG(INFO, "Got service object from Node Info File");
				boost::shared_ptr<comm_messages::BlockRequests> block_msg(new comm_messages::BlockRequests(dict));
				boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr(
							new Communication::SynchronousCommunication(
									node_info_file_rcrd->get_node_ip(),
									port
									)
								);
				boost::shared_ptr<MessageResult<comm_messages::BlockRequestsAck> > block_req_ack = 
								MessageExternalInterface::send_stop_service_http_request(
									comm_ptr,
									block_msg.get(),
									node_info_file_rcrd->get_node_ip(),
									port,
									0
									);
				if(block_req_ack->get_return_code() == 0)
				{
					boost::mutex::scoped_lock lock(this->mtx);
					std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
					if ( it != this->state_map.end() )
					{
						OSDLOG(INFO, service_id << " blocked request successfully");
						if(it->second->get_recovery_record()->get_operation() == NODE_DELETION)
							it->second->get_recovery_record()->set_status(ND_COMPLETED);
					}
				}
				else
				{
					boost::mutex::scoped_lock lock(this->mtx);
					std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
					if ( it != this->state_map.end() )
					{
						OSDLOG(ERROR, "Communication channel broke between GL and " << service_id);
						it->second->reset_planned();
						if(it->second->get_recovery_record()->get_operation() == NODE_DELETION)
							it->second->get_recovery_record()->set_status(ND_PLANNED); 
							//Done to ignore infinite looping in case destination service is down
							//OSDLOG(INFO, "Stop Service Return Code: "<<block_req_ack->get_return_code()<<". Assumiing it to be down/unreachable");
							//it->second->get_recovery_record()->set_status(ND_COMPLETED); 
					}
				}
				
			}
			else
			{
				OSDLOG(ERROR, "Could not get IP PORT of " << service_id << " in node info file");
			}
		}
	}
	catch(...)
	{
		OSDLOG(WARNING, "Exception raised in Balancing Thread");
		boost::mutex::scoped_lock lock(this->mtx);
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
		if ( it != this->state_map.end() )
		{
			if ( it->second->get_recovery_record()->get_status() == BALANCING_PLANNED )
			{
				it->second->reset_planned();
			}
		}
	}
}

bool StateChangeThreadManager::unmount_journal_filesystems(std::string service_id)
{
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
	if(it != this->state_map.end())
	{
		
		GLUtils util_obj;
		std::string failed_node_id = util_obj.get_node_id(service_id);
		std::string dest_node_name = util_obj.get_node_name(it->second->get_recovery_record()->get_destination_node());
		std::string journal_name = "." + failed_node_id + "_" + "container_journal";
		CommandExecutor exe;
		std::string umount_full_cmd = "sudo -u hydragui hydra_ssh " + dest_node_name + " " + this->umount_cmd + " -d " + journal_name;
		OSDLOG(INFO, "Unmounting container journal fs: "<< umount_full_cmd);
		boost::system_time timeout = boost::get_system_time()+ boost::posix_time::seconds(this->fs_mount_timeout);
		int status = exe.execute_cmd_in_thread(umount_full_cmd, timeout);
		if(status == 255)
			status = 0;
		if(status != 0)
			if(status != 12)
				return false;
		journal_name = "." + failed_node_id + "_" + "transaction_journal";
		umount_full_cmd = "sudo -u hydragui hydra_ssh " + dest_node_name + " " + this->umount_cmd + " -d " + journal_name;
		OSDLOG(INFO, "Unmounting transaction journal fs: "<< umount_full_cmd);
		status = exe.execute_cmd_in_thread(umount_full_cmd, timeout);
		if(status == 255)
			status = 0;

		if(status != 0)
			if (status != 12)
				return false;
	}
	return true;
}

/* This function checks for any new entry in table, creates recovery process,
sends http request for na/nd, and monitors state table for any failure */
void StateChangeThreadManager::monitor_table()
{
	OSDLOG(INFO, "SCI thread started");
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it;
	try
	{
		while(1)
		{
			boost::this_thread::interruption_point();
			std::vector<boost::shared_ptr<recordStructure::Record> > node_rec_records;
			std::vector<std::pair<std::vector<std::string>, int> > na_nd_container;	//List of pair of (list of nodes being added/deleted and operation)
			std::vector<std::string> na_account;
			std::vector<std::string> na_container;
			std::vector<std::string> na_updater;
			std::vector<std::string> nd_account;
			std::vector<std::string> nd_container;
			std::vector<std::string> nd_updater;
			std::vector<std::string> rec_account;
			std::vector<std::string> rec_container;
			std::vector<std::string> rec_updater;
			std::vector<std::pair<std::vector<std::string>, int> > na_nd_updater;		
			bool balancing_ongoing_na = false;
			bool balancing_ongoing_nd = false;
			this->service_entry_to_be_deleted.clear();
			bool if_deleted = false;
			if(this->state_map.empty() and (this->component_map_mgr->get_transient_map().size() != 0))
			{
				//Merge TMap and BasicMap
				OSDLOG(INFO, "System in stable state, deleting old journal files");
				this->component_map_mgr->merge_basic_and_transient_map();
				//Delete old journal and create new
				int curent_active_id = this->journal_lib_ptr->get_journal_writer()->get_current_journal_offset().first;
				this->journal_lib_ptr->get_journal_writer()->remove_all_journal_files();
				this->journal_lib_ptr->get_journal_writer()->set_active_file_id(curent_active_id + 1);
			}
			{
			boost::mutex::scoped_lock lock(this->mtx);
			for ( it = this->state_map.begin(); it != this->state_map.end(); ++it )
			{
				
				time_t now;
				time(&now);
				int status = it->second->get_recovery_record()->get_status();
				switch(status)
				{
					case NA_NEW:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						OSDLOG(INFO, "SCI thread encountered NA_NEW case: " << it->first);
						std::string node_added = it->second->get_recovery_record()->get_state_change_node();

						GLUtils util_obj;
						std::string service_name = util_obj.get_service_name(node_added);
						if ( service_name == CONTAINER_SERVICE )
						{
							na_container.push_back(node_added);
						}
						if ( service_name == ACCOUNT_SERVICE )
						{
							na_account.push_back(node_added);
						}
						if ( service_name == ACCOUNT_UPDATER )
						{
							na_updater.push_back(node_added);
						}
						if ( service_name == OBJECT_SERVICE)
						{
							this->component_map_mgr->add_object_entry_in_basic_map(service_name, it->first);
							OSDLOG(INFO, "SCI: Record added in Object Component Info File successfully");
							it->second->get_recovery_record()->set_status(NA_COMPLETED);
						}
//						it->second->get_recovery_record()->set_status(NA_COMPLETED);
					}
					break;
					case NA_COMPLETED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						OSDLOG(INFO, "SCI thread encountered NA_COMPLETED case for " << it->first);
						this->service_entry_to_be_deleted.insert(std::make_pair(it->first, NODE_ADDITION));
						//this->state_map.erase(it);
						//if_deleted = true;
					}
					break;
					case ND_NEW:
					{
						std::string node_deleted = it->second->get_recovery_record()->get_state_change_node();

						OSDLOG(INFO, "SCI thread encountered ND_NEW case for " << it->first);
						it->second->reset_planned();
						GLUtils util_obj;
						std::string service_name = util_obj.get_service_name(node_deleted);
						if ( service_name == CONTAINER_SERVICE )
						{
							nd_container.push_back(node_deleted);
						}
						if ( service_name == ACCOUNT_SERVICE )
						{
							nd_account.push_back(node_deleted);
						}
						if ( service_name == ACCOUNT_UPDATER )
						{
							nd_updater.push_back(node_deleted);
						}
						if ( service_name == OBJECT_SERVICE )
						{
							
//							boost::mutex::scoped_lock lock(this->mtx);
							it->second->get_recovery_record()->set_status(ND_PLANNED);
						}
						if ( service_name == PROXY_SERVICE )
						{
//							boost::mutex::scoped_lock lock(this->mtx);
							it->second->get_recovery_record()->set_status(ND_PLANNED);
							
						}

					}
					break;
					case ND_PLANNED:
					{
						
//						boost::mutex::scoped_lock lock(this->mtx);
						balancing_ongoing_nd = true;
						GLUtils util_obj;
						std::string service_name = util_obj.get_service_name(it->first);
						if ( !it->second->if_planned() )
						{
							OSDLOG(INFO, "SCI thread encountered ND_PLANNED case for " << it->first);
							if ( service_name == CONTAINER_SERVICE or service_name == ACCOUNT_SERVICE 
									or service_name == ACCOUNT_UPDATER )
							{
								boost::thread bal_thread(boost::bind(&StateChangeThreadManager::start_component_transfer, this, it->first));
							}
							else if ( service_name == OBJECT_SERVICE or service_name == PROXY_SERVICE )
							{
								boost::thread bal_thread(boost::bind(&StateChangeThreadManager::block_service_request, this, it->first));
							}
							it->second->set_planned();
						}
						
					}
					break;
					case ND_COMPLETED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);						
						GLUtils util_obj;
						std::string node_name = util_obj.get_node_name(it->first);
						OSDLOG(INFO, "SCI thread encountered ND_COMPLETED case for " << it->first);
						this->service_entry_to_be_deleted.insert(std::make_pair(node_name, NODE_DELETION));
//						if_deleted = this->check_if_recovery_completed_for_this_node(
//								it->first, it->second->get_recovery_record()->get_operation()
//								);
						
					}
					break;
					case ND_ABORTED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						OSDLOG(INFO, "SCI thread encountered ND_ABORTED case for " << it->first);
						it->second->get_recovery_record()->set_status(ND_NEW);
						//node_rec_records.push_back(it->second->get_recovery_record());	//Write about REC_NEW in journal
					}
					break;
					case BALANCING_NEW:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						
						OSDLOG(INFO, "SCI thread encountered BALANCING_NEW case for " << it->first);
						balancing_ongoing_na = true;
						it->second->get_recovery_record()->set_status(BALANCING_PLANNED);
						//node_rec_records.push_back(it->second->get_recovery_record());
						
					}
					break;
					case BALANCING_PLANNED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						balancing_ongoing_na = true;
						if ( !it->second->if_planned() )
						{
							OSDLOG(INFO, "SCI thread encountered BALANCING_PLANNED case for " << it->first);
							boost::thread bal_thread(boost::bind(
								&StateChangeThreadManager::start_component_transfer, this, it->first)
								);
							it->second->set_planned();
						}
					}
					break;
					case BALANCING_COMPLETED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						OSDLOG(INFO, "SCI thread encountered BALANCING_COMPLETED case for " << it->first);
						//this->service_entry_to_be_deleted.push_back(it->first);
						this->service_entry_to_be_deleted.insert(std::make_pair(it->first, COMPONENT_BALANCING));
						//this->state_map.erase(it);
						if_deleted = true;
					}
					break;
					case BALANCING_FAILED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						if(it->second->get_fd() < 0)
						{
							it->second->get_recovery_record()->set_status(BALANCING_PLANNED);
						}

					}
					break;
					case BALANCING_RESCHEDULE:
					{
						OSDLOG(INFO, "Handling BALANCING_RESCHEDULE case for " << it->first);
						std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > aborted_list = it->second->get_recovery_record()->get_planned_map();
						std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator failed_itr = aborted_list.begin();
						std::vector<std::string> aborted_node_list;
						for ( ; failed_itr != aborted_list.end() ; ++failed_itr )
						{
							aborted_node_list.push_back((*failed_itr).second.get_service_id());
						}
						bool if_any_recovered = this->component_balancer_ptr->check_if_nodes_already_recovered(aborted_node_list);
						it->second->reset_planned();
						if (if_any_recovered)	//If rec for any failed dest has been done, then ask for new balancing from CompBalancer
						{	
							GLUtils util_obj;
							std::string service_type = util_obj.get_service_name(it->first);
							std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > new_planned_for_src =
								this->component_balancer_ptr->get_new_balancing_for_source(
										service_type,
										it->first,
										it->second->get_recovery_record()->get_planned_map()
										);
							it->second->get_recovery_record()->set_planned_map(new_planned_for_src);
							OSDLOG(INFO, "Planned map size while Balancing aborted case of " << it->first << " is " << new_planned_for_src.size());
							if(new_planned_for_src.empty())
							{
								it->second->get_recovery_record()->set_status(BALANCING_COMPLETED);
							}
							else
							{
								it->second->get_recovery_record()->set_status(BALANCING_NEW);
							}
							//this->update_status(itr->first, node_rec_records, BALANCING_NEW);
						}
						else			//Else move from Aborted to planned state
						{
							//this->update_status(itr->first, node_rec_records, BALANCING_PLANNED);
							it->second->get_recovery_record()->set_status(BALANCING_PLANNED);
						}

					}
					break;
					case RECOVERY_NEW:
					{
						std::vector<std::string> nodes_as_rec_destination, unhealthy_nodes;
						this->get_info_about_unhealthy_nodes(unhealthy_nodes, nodes_as_rec_destination);
//						boost::mutex::scoped_lock lock(this->mtx);
						OSDLOG(INFO, "SCI thread encountered RECOVERY_NEW case for " << it->first);
						it->second->set_last_updated_time(now);
						std::string failed_service_id = it->second->get_recovery_record()->get_state_change_node();
						//failed_node is actually service id, to be changed by gmishra
						//std::string rec_process_dest = this->component_balancer_ptr->get_target_node_for_recovery(unhealthy_nodes, nodes_as_rec_destination);
						std::string rec_process_dest =
							this->component_balancer_ptr->get_target_node_for_recovery(failed_service_id);
						if (rec_process_dest != "NO_NODE_FOUND")	//TODO find better mechanism
						{
						it->second->get_recovery_record()->set_destination(rec_process_dest);

						GLUtils util_obj;
						std::string service_name = util_obj.get_service_name(failed_service_id);
						std::string failed_node_id = util_obj.get_node_id(failed_service_id);
						std::string dest_node_name = util_obj.get_node_id(rec_process_dest);
						if ( service_name == CONTAINER_SERVICE )
						{
							rec_container.push_back(failed_service_id);

							boost::shared_ptr<recordStructure::Record> mount_rec(
								new recordStructure::GLMountInfoRecord(
									failed_node_id,
									dest_node_name
									)
							);
							node_rec_records.push_back(mount_rec);
						}
						if ( service_name == ACCOUNT_SERVICE )
						{
							rec_account.push_back(failed_service_id);
						}
						if ( service_name == ACCOUNT_UPDATER )
						{
							rec_updater.push_back(failed_service_id);
						}
						if ( service_name == OBJECT_SERVICE )
						{
							it->second->get_recovery_record()->set_status(RECOVERY_PLANNED);
						}
						if ( service_name == PROXY_SERVICE)
						{
							it->second->get_recovery_record()->set_status(RECOVERY_CLOSED);
						}
						}

					}
					break;
					case RECOVERY_PLANNED:
					{
						
//						boost::mutex::scoped_lock lock(this->mtx);
						if((now - it->second->get_last_updated_time()) > it->second->get_timeout() and
												it->second->if_planned())
						{
							//Clean previous recovery process
							OSDLOG(INFO, "Timeout occurred for Rec Process. Killing it.");
							this->clean_recovery_process(it->second->get_recovery_record()->get_state_change_node(), it->second->get_recovery_record()->get_destination_node());
							OSDLOG(INFO, "Changing the recovery state from planned to new");
							it->second->get_recovery_record()->set_status(RECOVERY_NEW);
							it->second->reset_planned();
							GLUtils util_obj;
							this->component_balancer_ptr->update_recovery_dest_map(
									it->second->get_recovery_record()->get_destination_node(),
									util_obj.get_service_name(it->first)
								);
						}
						else if(!it->second->if_planned())
						{
							OSDLOG(INFO, "SCI thread encountered RECOVERY_PLANNED:" << it->first);
							OSDLOG(INFO, "Operation type is " << it->second->get_recovery_record()->get_operation());
							this->start_node_recovery_process(
									it->second->get_recovery_record()->get_state_change_node(),
									it->second->get_recovery_record()->get_destination_node()
								);
							it->second->set_planned();
						}
						//node_rec_records.push_back(it->second->get_recovery_record());

					}
					break;
					case RECOVERY_IN_PROGRESS:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						if((now - it->second->get_last_updated_time()) > it->second->get_timeout())
						{
							OSDLOG(INFO, "Didn't receive hrbt from Rec Process of " << it->first << ". Times are: " << (now - it->second->get_last_updated_time()) << ", " << it->second->get_timeout() << ". Setting RECOVERY_FAILED");
							OSDLOG(INFO, "Now is : " << now << " and last updated time : " << it->second->get_last_updated_time());

							GLUtils util_obj;
							std::string service_name = util_obj.get_service_name(it->first);
							if(service_name == CONTAINER_SERVICE)
							{
								if(!this->unmount_journal_filesystems(it->first))
									continue;
							}

							it->second->get_recovery_record()->set_status(RECOVERY_FAILED);
							//node_rec_records.push_back(it->second->get_recovery_record());	//Write about REC_FAILED in journal
						}
					}
					break;
					case RECOVERY_ABORTED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						OSDLOG(INFO, "SCI thread encountered RECOVERY_ABORTED for " << it->first);
						it->second->get_recovery_record()->set_status(RECOVERY_NEW);
						GLUtils util_obj;
						this->component_balancer_ptr->update_recovery_dest_map(
									it->second->get_recovery_record()->get_destination_node(),
									util_obj.get_service_name(it->first)
									);
						std::string service_name = util_obj.get_service_name(it->first);
						if(service_name == CONTAINER_SERVICE)
						{
							if(!this->unmount_journal_filesystems(it->first))
								continue;
						}
						//node_rec_records.push_back(it->second->get_recovery_record());	//Write about REC_NEW in journal
					}
					break;
					case RECOVERY_COMPLETED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
						OSDLOG(INFO, "SCI thread encountered RECOVERY_COMPLETED for " << it->first);
						GLUtils util_obj;
						std::string service_name = util_obj.get_service_name(it->first);
						std::string node_name = util_obj.get_node_name(it->first);
						if(service_name == CONTAINER_SERVICE)
						{
							if(!this->unmount_journal_filesystems(it->first))
								continue;
						}
						if(service_name == OBJECT_SERVICE)
						{
							this->component_map_mgr->remove_object_entry_from_map(it->first);
						}
						this->component_balancer_ptr->update_recovery_dest_map(it->second->get_recovery_record()->get_destination_node(), service_name);
						this->service_entry_to_be_deleted.insert(std::make_pair(node_name, NODE_RECOVERY));
//						if_deleted = this->check_if_recovery_completed_for_this_node(it->first, it->second->get_recovery_record()->get_operation());
						it->second->get_recovery_record()->set_status(RECOVERY_CLOSED);
				
					}
					break;
					case RECOVERY_CLOSED:
					{
//						boost::mutex::scoped_lock lock(this->mtx);
//						if_deleted = this->check_if_recovery_completed_for_this_node(it->first, it->second->get_recovery_record()->get_operation());
					}
					break;
					default:
					break;
				}
			}
			}
			if(!this->service_entry_to_be_deleted.empty())
			{
				OSDLOG(INFO, "Some entries need to be cleaned up from state table");
				std::map<std::string, int>::iterator it_del = this->service_entry_to_be_deleted.begin();
//				boost::mutex::scoped_lock lock(this->mtx);
				for(; it_del != this->service_entry_to_be_deleted.end(); ++it_del)
				{
					if(it_del->second == NODE_RECOVERY or it_del->second == NODE_DELETION)
					{
						this->check_if_recovery_completed_for_this_node(it_del->first, it_del->second);
					}
					else // if(it_del->second == COMPONENT_BALANCING)
					{
						this->state_map.erase(it_del->first);
					}
				}
			}
			if(!node_rec_records.empty())
			{
				this->journal_lib_ptr->add_entry_in_journal(node_rec_records, boost::bind(&StateChangeThreadManager::notify, this));
				this->wait_for_journal_write();
			}
			if(balancing_ongoing_na)	//To only entertain NA and ND in case balancing is not in progress
			{
				OSDLOG(DEBUG, "Balancing is already in progress");

				na_account.clear();
				na_container.clear();
				na_updater.clear();
			}
			if(balancing_ongoing_nd)
			{	
				nd_account.clear();
				nd_container.clear();
				nd_updater.clear();
			}
//			node_rec_records.clear();	//Re-use node_rec_records list for flushing in journal
			if (!na_account.empty() or !nd_account.empty() or !rec_account.empty())
			{

				std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> new_planned_map_pair = this->component_balancer_ptr->get_new_planned_map(ACCOUNT_SERVICE, na_account, nd_account, rec_account);
				if(new_planned_map_pair.second)
				{
				OSDLOG(INFO, "Account planned map size is: " << new_planned_map_pair.first.size());
				this->modify_sci_map_for_new_plan(new_planned_map_pair.first,na_account,nd_account,rec_account);
				}
			}

			if (!na_container.empty() or !nd_container.empty() or !rec_container.empty())
			{
				std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> new_planned_map_pair = this->component_balancer_ptr->get_new_planned_map(CONTAINER_SERVICE, na_container, nd_container, rec_container);
				if(new_planned_map_pair.second)
				{
				OSDLOG(INFO, "Container planned map size is: " << new_planned_map_pair.first.size());
				this->modify_sci_map_for_new_plan(new_planned_map_pair.first, na_container, nd_container, rec_container);
				}
			}

			if (!na_updater.empty() or !nd_updater.empty() or !rec_updater.empty())
			{
				std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> new_planned_map_pair = this->component_balancer_ptr->get_new_planned_map(ACCOUNT_UPDATER, na_updater, nd_updater, rec_updater);
				if(new_planned_map_pair.second)
				{
				OSDLOG(INFO, "Updater planned map size is: " << new_planned_map_pair.first.size());
				this->modify_sci_map_for_new_plan(new_planned_map_pair.first, na_updater, nd_updater, rec_updater);
				}
			}
			boost::this_thread::sleep( boost::posix_time::seconds(8));      //TODO: Just for testing purpose
		}
	}
	catch(const boost::thread_interrupted&)
	{
		this->journal_lib_ptr->get_journal_writer()->close_journal();
		OSDLOG(INFO, "Closing State Change Initiator Thread");
	}

}


void StateChangeThreadManager::modify_sci_map_for_new_plan(
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map,
	std::vector<std::string> na_list,
	std::vector<std::string> nd_list,
	std::vector<std::string> rec_list
)
{
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator map_itr = new_planned_map.begin();
	for ( ; map_itr != new_planned_map.end() ; ++map_itr )
	{
		OSDLOG(INFO, "(" << map_itr->first << ", " << map_itr->second.size() << ")");
	}
	for (map_itr = new_planned_map.begin() ; map_itr != new_planned_map.end() ; ++map_itr )
	{
		boost::mutex::scoped_lock lock(this->mtx);
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator itr2 = 
									this->state_map.find(map_itr->first);
		if ( itr2 != this->state_map.end() )
		{
			
			//Entry already exists, that means ND or REC case
			itr2->second->get_recovery_record()->set_planned_map(map_itr->second);
			time_t now;
			time(&now);
			itr2->second->set_last_updated_time(now);
			if( itr2->second->get_recovery_record()->get_operation() == NODE_DELETION)
			{
				//itr2->second->get_recovery_record()->set_operation(COMPONENT_BALANCING);
				OSDLOG(INFO, "Changing the state to ND_PLANNED");
				itr2->second->get_recovery_record()->set_status(ND_PLANNED);
				itr2->second->set_expected_msg_type(messageTypeEnum::typeEnums::COMP_TRANSFER_INFO);
				itr2->second->reset_planned();
				nd_list.erase(std::remove(nd_list.begin(), nd_list.end(), map_itr->first), nd_list.end());
			}
			else if(itr2->second->get_recovery_record()->get_operation() == NODE_RECOVERY)
			{
				OSDLOG(INFO, "Changing the state to RECOVERY_PLANNED");
				itr2->second->get_recovery_record()->set_status(RECOVERY_PLANNED);
				rec_list.erase(std::remove(rec_list.begin(), rec_list.end(), map_itr->first), rec_list.end());
				itr2->second->reset_planned();
				//node_rec_records.push_back(itr2->second->get_recovery_record());
			}
			else	//This section will only be executed in case of cluster recovery
			{
				OSDLOG(INFO, "Changing the operation to BALANCING and state to BALANCING_NEW");
				itr2->second->get_recovery_record()->set_operation(COMPONENT_BALANCING);
				itr2->second->get_recovery_record()->set_status(BALANCING_NEW);
				na_list.erase(std::remove(na_list.begin(), na_list.end(), map_itr->first), na_list.end());
				itr2->second->reset_planned();

			}
			//node_rec_records.push_back(itr2->second->get_recovery_record());
		}
		else
		{
			//NA case
			time_t now;
			time(&now);
			OSDLOG(DEBUG, "Changing the state from Node Addition to Balancing");
			boost::shared_ptr<recordStructure::RecoveryRecord> ob(
					new recordStructure::RecoveryRecord(COMPONENT_BALANCING, map_itr->first, BALANCING_NEW)
					);
		
			ob->set_planned_map(map_itr->second);
			OSDLOG(INFO, "Inserting new entry " << map_itr->first << " in sci map while balancing");
			this->state_map.insert(std::make_pair(map_itr->first,
				boost::shared_ptr<StateChangeInitiatorMap>(
					new StateChangeInitiatorMap(
						ob, -1, now, this->strm_timeout, false, this->hrbt_failure_count)
						)
					)
				);
		}
//		node_rec_records.push_back(ob);
	}
	if (!na_list.empty())	//Mark NA_COMPLETED for all nodes added
	{
		std::vector<std::string>::iterator it_vec = na_list.begin();
		for(; it_vec != na_list.end(); ++it_vec)
		{
			boost::mutex::scoped_lock lock(this->mtx);
			std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator itrr = this->state_map.find(*it_vec);
			if(itrr != this->state_map.end())
			{
				OSDLOG(DEBUG, "Marking NA_COMPLETED for " << itrr->first);
				itrr->second->get_recovery_record()->set_status(NA_COMPLETED);
//				node_rec_records.push_back(itrr->second->get_recovery_record());
			}
		}
	}
	if (!rec_list.empty())	//Means failed node doesn't has any component, so complete recovery
	{
		
		std::vector<std::string>::iterator it = rec_list.begin();
		for( ; it != rec_list.end(); ++it)
		{
			boost::mutex::scoped_lock lock(this->mtx);
			std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator itrr =
								this->state_map.find(*it);
			if(itrr != this->state_map.end())
			{
				if (itrr->second->get_recovery_record()->get_planned_map().empty())
				{
					OSDLOG(INFO, itrr->first << " doesn't has any component. Completing the RECOVERY"); 
					
					itrr->second->get_recovery_record()->set_status(RECOVERY_COMPLETED);
					//node_rec_records.push_back(itrr->second->get_recovery_record());
				}
			}
		}
	}
	if (!nd_list.empty())	//Means these nodes donot have any components on them, so complete ND
	{
		std::vector<std::string>::iterator it = nd_list.begin();
		for( ; it != nd_list.end(); ++it)
		{
			boost::mutex::scoped_lock lock(this->mtx);
			std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator itrr =
								this->state_map.find(*it);
			if(itrr != this->state_map.end())
			{
				if (itrr->second->get_recovery_record()->get_planned_map().empty())
				{
					OSDLOG(INFO, itrr->first << " doesn't has any component. Completing the Node Deletion");
					itrr->second->get_recovery_record()->set_status(ND_COMPLETED);
//						node_rec_records.push_back(itrr->second->get_recovery_record());
				}
			}
		}
	}
}

StateChangeMonitorThread::StateChangeMonitorThread(
	boost::shared_ptr<Bucket> bucket_ptr,
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> > &state_map,
	boost::shared_ptr<global_leader::JournalLibrary> journal_lib_ptr,
	boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
	boost::shared_ptr<component_balancer::ComponentBalancer> component_balancer_ptr,
	boost::mutex &mtx
):
	bucket_ptr(bucket_ptr),
	state_map(state_map),
	journal_lib_ptr(journal_lib_ptr),
	component_map_mgr(component_map_mgr),
	component_balancer_ptr(component_balancer_ptr),
	work_done(false),
	mtx(mtx)
{
	
}

boost::shared_ptr<MessageResultWrap> StateChangeMonitorThread::receive_data(int64_t fd)
{

	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj = this->bucket_ptr->get_comm_obj(fd);
	boost::shared_ptr<MessageResultWrap> msg_wrap = MessageExternalInterface::sync_read_any_message(comm_obj, 100);
	if(msg_wrap->get_error_code() != Communication::SUCCESS)	//if any error(socket error or type error), remove fd from bucket
	{
		OSDLOG(ERROR, "Communication channel corrupted");
		this->bucket_ptr->remove_fd(fd);
		msg_wrap.reset();
	}
	return msg_wrap;
}

void StateChangeMonitorThread::run()
{
	GLUtils utils_obj;
	while(1)
	{
		int error = 0;
		std::set<int64_t> failed_node_fds = this->bucket_ptr->get_fd_list();
		std::vector<boost::shared_ptr<recordStructure::Record> > node_rec_records;
		if ( !failed_node_fds.empty() )
		{
			std::set<int64_t> active_fds = utils_obj.selectOnSockets(failed_node_fds, error, 100);
			if ( error != 0 )
			{
				//check for fatal error else continue
			}
			for (std::set<int64_t>::iterator it = active_fds.begin(); it != active_fds.end(); ++it)
			{
				boost::shared_ptr<MessageResultWrap> msg = this->receive_data(*it);
				if(msg)
				{
					switch(msg->get_type())
					{
						case messageTypeEnum::typeEnums::HEART_BEAT:
							{
							boost::shared_ptr<comm_messages::HeartBeat> hb_ptr(new comm_messages::HeartBeat);
							hb_ptr->deserialize(msg->get_payload(), msg->get_size());
							this->update_time_in_map(hb_ptr->get_service_id());
						
							}
							break;
						case messageTypeEnum::typeEnums::COMP_TRANSFER_INFO:
							{
//							this->move_planned_to_transient(msg->get_service_id());	//Only for testing purpose
							boost::shared_ptr<comm_messages::CompTransferInfo> comp_info_ptr(new comm_messages::CompTransferInfo);
							comp_info_ptr->deserialize(msg->get_payload(), msg->get_size());
							bool ret = this->update_planned_components(comp_info_ptr->get_service_id(), comp_info_ptr->get_comp_service_list(), node_rec_records);
							this->update_time_in_map(comp_info_ptr->get_service_id());
							if(ret)
							{
								this->component_map_mgr->send_ack_for_comp_trnsfr_msg(
									(this->bucket_ptr->get_comm_obj(*it)),
									messageTypeEnum::typeEnums::COMP_TRANSFER_INFO_ACK
									);
							}
							else
							{
								OSDLOG(INFO, "Planned map changed from the time when Rec Process was created. Closing this channel");      
								this->bucket_ptr->remove_fd(*it);
							}
							}
							break;
						case messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT: //End Monitoring
							{
							boost::shared_ptr<comm_messages::CompTransferFinalStat> comp_info_final_ptr(new comm_messages::CompTransferFinalStat);
							comp_info_final_ptr->deserialize(msg->get_payload(), msg->get_size());
							std::vector<std::pair<int32_t, bool> > final_status = comp_info_final_ptr->get_component_pair_list();
							this->handle_end_monitoring(comp_info_final_ptr->get_service_id(), final_status, node_rec_records);
							//this->remove_entry_from_map(comp_info_final_ptr->get_service_id());
							this->component_map_mgr->send_ack_for_comp_trnsfr_msg((this->bucket_ptr->get_comm_obj(*it)), messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT_ACK);
							this->bucket_ptr->remove_fd(*it);
							}
							break;
						default:
							OSDLOG(ERROR, "Wrong msg received by StateChangeMonitorThread: " << msg->get_type());
							this->bucket_ptr->remove_fd(*it);
							break;
					}
				}
			}
		}
		{
		boost::mutex::scoped_lock lock(this->mtx);
		
		OSDLOG(INFO, "Size of state table in Mon Thread: " << this->state_map.size());
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator itr = this->state_map.begin();
		for ( ; itr != this->state_map.end() ; ++itr )
		{
			if (itr->second->get_recovery_record()->get_status() == RECOVERY_FAILED)
			{
				this->bucket_ptr->remove_fd(itr->second->get_fd());
				itr->second->set_fd(-1);
				if ( itr->second->get_recovery_record()->get_planned_map().empty() )
				{
					OSDLOG(INFO, "Planned map during recovery failed is empty, so changing the state to RECOVERY COMPLETE");
					itr->second->get_recovery_record()->set_status(RECOVERY_COMPLETED);
				}
				else
				{
					OSDLOG(INFO, "Changing RECOVERY_FAILED for service " << itr->first << " to RECOVERY_NEW");
					GLUtils util_obj;
					this->component_balancer_ptr->update_recovery_dest_map(
							itr->second->get_recovery_record()->get_destination_node(),
							util_obj.get_service_name(itr->first)
							);
					itr->second->set_fd(-1);

					itr->second->get_recovery_record()->set_status(RECOVERY_NEW);
					//this->update_status(itr->first, node_rec_records, RECOVERY_NEW);
				}
			}
			else if (itr->second->get_recovery_record()->get_status() == ND_FAILED)
			{
				this->bucket_ptr->remove_fd(itr->second->get_fd());
				itr->second->set_fd(-1);
				if ( itr->second->get_recovery_record()->get_planned_map().empty() )
				{
					OSDLOG(INFO, "Planned map during balancing is empty, so changing the state to ND_COMPLETED");
					itr->second->get_recovery_record()->set_status(ND_COMPLETED);
				}
				else
				{
					itr->second->set_fd(-1);
					itr->second->get_recovery_record()->set_status(ND_PLANNED);
					OSDLOG(DEBUG, "Changing status from ND_FAILED to PLANNED for " << itr->first);
				}
			}
			else if (itr->second->get_recovery_record()->get_status() == BALANCING_FAILED)
			{
				this->bucket_ptr->remove_fd(itr->second->get_fd());
				itr->second->set_fd(-1);
				itr->second->reset_planned();
				if ( itr->second->get_recovery_record()->get_planned_map().empty() )
				{
					OSDLOG(INFO, "Planned map during balancing is empty, so changing the state to BALANCING_COMPLETED");
					itr->second->get_recovery_record()->set_status(BALANCING_COMPLETED);
				}
				else
				{
					itr->second->set_fd(-1);
					itr->second->get_recovery_record()->set_status(BALANCING_PLANNED);
					OSDLOG(DEBUG, "Changing status from balancing FAILED to PLANNED for " << itr->first);
					//this->update_status(itr->first, node_rec_records, BALANCING_PLANNED);
				}	
			}
			else if (itr->second->get_recovery_record()->get_status() == BALANCING_ABORTED)
			{
				this->bucket_ptr->remove_fd(itr->second->get_fd());
				itr->second->set_fd(-1);
				OSDLOG(DEBUG, "Handling BALANCING_ABORTED case for " << itr->first);
				std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > aborted_list = itr->second->get_recovery_record()->get_planned_map();
				std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator failed_itr = aborted_list.begin();
				std::vector<std::string> aborted_node_list;
				for ( ; failed_itr != aborted_list.end() ; ++failed_itr )
				{
					aborted_node_list.push_back((*failed_itr).second.get_service_id());
				}
				bool if_any_recovered = this->component_balancer_ptr->check_if_nodes_already_recovered(aborted_node_list);
				itr->second->reset_planned();
				if (if_any_recovered)	//If rec for any failed dest has been done, then ask for new balancing from CompBalancer
				{	
					GLUtils util_obj;
					std::string service_type = util_obj.get_service_name(itr->first);
					std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > new_planned_for_src =
						this->component_balancer_ptr->get_new_balancing_for_source(
								service_type,
								itr->first,
								itr->second->get_recovery_record()->get_planned_map()
								);
					itr->second->get_recovery_record()->set_planned_map(new_planned_for_src);
					OSDLOG(INFO, "Planned map size while Balancing aborted case of " << itr->first << " is " << new_planned_for_src.size());
					if(new_planned_for_src.empty())
					{
						itr->second->get_recovery_record()->set_status(BALANCING_COMPLETED);
					}
					else
					{
						itr->second->get_recovery_record()->set_status(BALANCING_NEW);
					}
					//this->update_status(itr->first, node_rec_records, BALANCING_NEW);
				}
				else			//Else move from Aborted to planned state
				{
					//this->update_status(itr->first, node_rec_records, BALANCING_PLANNED);
					itr->second->get_recovery_record()->set_status(BALANCING_PLANNED);
				}
			}
		}
		}
		boost::this_thread::sleep( boost::posix_time::seconds(5));	//TODO: Just for testing purpose
	}
}

void StateChangeMonitorThread::notify()
{
	boost::mutex::scoped_lock lock(this->mtx);
	this->work_done = true;
	this->condition.notify_one();
}

void StateChangeMonitorThread::move_planned_to_transient(std::string service_id)	//Only for testing purpose
{
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
	GLUtils util_obj;
	std::string service_name = util_obj.get_service_name(service_id);
	if ( it != this->state_map.end() )
	{
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > p_vec = it->second->get_recovery_record()->get_planned_map();
		this->component_map_mgr->update_transient_map(service_name, p_vec);
		it->second->get_recovery_record()->clear_planned_map();
		//Move planned map of this key to transient map
	}
}

void StateChangeMonitorThread::wait_for_journal_write()
{
	boost::mutex::scoped_lock lock(this->mtx);
	while(!this->work_done)
	{
		this->condition.wait(lock);
	}
	this->work_done = false;
}

void StateChangeMonitorThread::remove_entry_from_map(std::string service_id)
{
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
	if ( it != this->state_map.end() )
	{
		this->state_map.erase(it);
	}
	else
	{
		OSDLOG(INFO, "No service with id " << service_id << " found in state change table");
	}
}

bool StateChangeMonitorThread::update_planned_components(
	std::string service_id,
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > intermediate_state,
	std::vector<boost::shared_ptr<recordStructure::Record> > &node_rec_records
)
{
	bool ret = true;
	this->mtx.lock();
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
	if ( it != this->state_map.end() )
	{
		OSDLOG(INFO, "Intermediate response received for service " << service_id << " for operation " << it->second->get_recovery_record()->get_operation());
		GLUtils util_obj;
		std::string service_name = util_obj.get_service_name(service_id);
		std::pair<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, bool> actual_transferred_comp = 
							this->component_map_mgr->update_transient_map(service_name, intermediate_state);
		it->second->get_recovery_record()->remove_from_planned_map(actual_transferred_comp.first);
		ret = actual_transferred_comp.second;
		this->mtx.unlock();
		this->journal_lib_ptr->prepare_transient_record_for_journal(service_id, node_rec_records);
		if (!node_rec_records.empty())
		{
			this->journal_lib_ptr->add_entry_in_journal(node_rec_records, boost::bind(&StateChangeMonitorThread::notify, this));
			this->wait_for_journal_write();
		}
	}
	else
	{
		this->mtx.unlock();
	}
	return ret;
}

void StateChangeMonitorThread::handle_end_monitoring(
	std::string service_id,
	std::vector<std::pair<int32_t, bool> > final_status,
	std::vector<boost::shared_ptr<recordStructure::Record> > &node_rec_records
)
{
	cool::unused(node_rec_records);
	OSDLOG(INFO, "Rcvd End Mon from " << service_id);

	boost::mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
	if ( it != this->state_map.end() )
	{
		if ( it->second->get_recovery_record()->get_planned_map().empty() )
		{
			if ( it->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING )
			{
				OSDLOG(INFO, "Node Addition balancing operation complete for " << service_id);
				it->second->get_recovery_record()->set_status(BALANCING_COMPLETED);	//For both NA and ND
			}
			else if ( it->second->get_recovery_record()->get_operation() == NODE_RECOVERY )
			{
				OSDLOG(INFO, "Node Recovery for " << service_id << " completed");
				it->second->get_recovery_record()->set_status(RECOVERY_COMPLETED);
				//node_rec_records.push_back(it->second->get_recovery_record());
			}
			else if ( it->second->get_recovery_record()->get_operation() == NODE_DELETION )
			{
				OSDLOG(INFO, "Node Deletion for " << service_id << " completed");
				it->second->get_recovery_record()->set_status(ND_COMPLETED);
			}
		}
		else
		{
			OSDLOG(INFO, "Planned map is not empty when encountered End Monitoring");
			if ( it->second->get_recovery_record()->get_operation() == COMPONENT_BALANCING )
			{
				it->second->get_recovery_record()->set_status(BALANCING_ABORTED);
			}
			else if ( it->second->get_recovery_record()->get_operation() == NODE_RECOVERY )
			{
				it->second->get_recovery_record()->set_status(RECOVERY_ABORTED);
				//node_rec_records.push_back(it->second->get_recovery_record());
			}
			else if ( it->second->get_recovery_record()->get_operation() == NODE_DELETION )
			{
				it->second->get_recovery_record()->set_status(ND_ABORTED);
			}
		}
//		node_rec_records.push_back(it->second->get_recovery_record());
	}
	//TODO: May be matching of final_status needs to be performed b/w Global Planned Map and Transient Map
}

/* This interface is not used anymore */
void StateChangeMonitorThread::update_status(
	std::string service_id,
	std::vector<boost::shared_ptr<recordStructure::Record> > &journal_records,
	int status
)
{
	boost::mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(service_id);
	if ( it != this->state_map.end() )
	{
		it->second->get_recovery_record()->set_status(status);
		journal_records.push_back(it->second->get_recovery_record());
	}
}

void StateChangeMonitorThread::update_time_in_map(std::string node_id)
{
	boost::mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator it = this->state_map.find(node_id);
	if ( it != this->state_map.end() )
	{
		time_t now;
		time(&now);
		it->second->set_last_updated_time(now);
		OSDLOG(DEBUG, "Updating time in state map for service " << it->first);
	}
	else
	{
		OSDLOG(DEBUG, "Entry with node id " << node_id << " doesnot exist in the State Change Map");
	}
}

} // namespace system_state
