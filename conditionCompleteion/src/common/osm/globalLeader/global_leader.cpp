#include <boost/bind.hpp>
#include <sys/stat.h>
#include <boost/format.hpp>
#include <sys/types.h>
#include <fcntl.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <utime.h>

#include "communication/callback.h"
#include "communication/message_type_enum.pb.h"
#include "osm/globalLeader/global_leader.h"
#include "osm/osmServer/utils.h"
#include "osm/globalLeader/cluster_recovery_component.h"

namespace global_leader
{

//UNCOVERED_CODE_BEGIN:
cool::SharedPtr<config_file::OSMConfigFileParser> const get_config_parser()
{	
	cool::config::ConfigurationBuilder<config_file::OSMConfigFileParser> config_file_builder;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/osm.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/osm.conf");
	return config_file_builder.buildConfiguration();
}
//UNCOVERED_CODE_END

GlobalLeader::GlobalLeader():stop(false)
{
	GLUtils util_obj;
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = util_obj.get_config();
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	this->gfs_write_interval = parser_ptr->gfs_write_intervalValue();
	this->gfs_write_failure_count = parser_ptr->gfs_write_failure_countValue();
	this->gl_port = parser_ptr->osm_portValue();
	this->node_info_file_path = parser_ptr->node_info_file_pathValue();

	cool::SharedPtr<config_file::OsdServiceConfigFileParser> timeout_config_parser = util_obj.get_osm_timeout_config();
	this->node_add_response_timeout = timeout_config_parser->node_add_ack_timeout_for_glValue();
	std::string cmd;
	if (osd_config)
		this->node_info_file_writer.reset(new osm_info_file_manager::NodeInfoFileWriteHandler(this->node_info_file_path));
	else
		this->node_info_file_writer.reset(new osm_info_file_manager::NodeInfoFileWriteHandler(CONFIG_FILESYSTEM)); 

	boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr(new component_manager::ComponentMapManager(parser_ptr));
	this->monitoring.reset(new gl_monitoring::MonitoringImpl(
			this->node_info_file_writer
			)
		);

	this->states_thread_mgr.reset(new system_state::StateChangeThreadManager(
				parser_ptr,
				component_map_mgr,
				boost::bind(&gl_monitoring::Monitoring::change_status_in_monitoring_component, this->monitoring, _1, _2)
				)
			);
	this->monitoring->initialize_prepare_and_add_entry(boost::bind(&system_state::StateChangeThreadManager::prepare_and_add_entry, this->states_thread_mgr, _1, _2));
	this->state_msg_ptr.reset(new system_state::StateChangeMessageHandler(this->states_thread_mgr, component_map_mgr, this->monitoring));
	this->mon_thread_mgr.reset(new gl_monitoring::MonitoringThreadManager(this->monitoring));
	this->msg_handler.reset(
			new gl_parser::GLMsgHandler(
				boost::bind(&GlobalLeader::push, this, _1),
				boost::bind(&system_state::StateChangeMessageHandler::push, this->state_msg_ptr, _1),
				boost::bind(&gl_monitoring::MonitoringThreadManager::monitor_service, this->mon_thread_mgr, _1, _2),
				boost::bind(&gl_monitoring::Monitoring::retrieve_node_status, this->monitoring, _1, _2)
/*				this->state_msg_ptr,
				this->mon_thread_mgr,
				this->monitoring
*/			)
		);
	this->cluster_recovery_in_progress = false;
	
}

boost::shared_ptr<gl_parser::GLMsgHandler> GlobalLeader::get_gl_msg_handler()
{
	return this->msg_handler;
}

void GlobalLeader::recover_active_node_list()
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> reader;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	reader.reset(new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path, this->monitoring));
	if(!reader->recover_record())
		OSDLOG(INFO, "No records to recover");
}

void GlobalLeader::start()
{
	this->gfs_writer_thread.reset(new boost::thread(&GlobalLeader::gfs_file_writer, this));
	this->recover_active_node_list();
	this->start_gl_recovery();
	this->state_mgr_thread.reset(new boost::thread(&system_state::StateChangeThreadManager::monitor_table, this->states_thread_mgr));
	this->rec_identifier_thread.reset(new boost::thread(&gl_monitoring::Monitoring::monitor_for_recovery, this->monitoring));
	this->msg_handler_thread.reset(new boost::thread(&gl_parser::GLMsgHandler::handle_msg, this->msg_handler));
	this->state_msg_handler_thread.reset(new boost::thread(&system_state::StateChangeMessageHandler::handle_msg, this->state_msg_ptr));
//	this->gfs_writer_thread.reset(new boost::thread(&GlobalLeader::gfs_file_writer, this));
//	this->update_gl_info_file();
	this->handle_main_queue_msg();
}

void GlobalLeader::start_gl_recovery()
{
	OSDLOG(DEBUG, "Starting GL Recovery");
	std::vector<std::string> nodes_registered;
	std::vector<std::string> nodes_failed;
	std::vector<std::string> nodes_retired;
	std::vector<std::string> nodes_recovered;
	GLUtils util_obj;
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = util_obj.get_journal_config();
	this->monitoring->get_nodes_status(nodes_registered, nodes_failed, nodes_retired, nodes_recovered);
	std::vector<std::string>::iterator it = nodes_registered.begin();

	for(; it != nodes_registered.end(); ++it)
	{
		OSDLOG(INFO, "Nodes registered: " << *it);
	}
	for(it = nodes_failed.begin(); it != nodes_failed.end(); ++it)
	{
		OSDLOG(INFO, "Nodes failed: " << *it);
	}
	for(it = nodes_retired.begin(); it != nodes_retired.end(); ++it)
	{
		OSDLOG(INFO, "Nodes retired: " << *it);
	}
	for(it = nodes_recovered.begin(); it != nodes_recovered.end(); ++it)
	{
		OSDLOG(INFO, "Nodes recovered: " << *it);
	}
	this->states_thread_mgr->start_recovery(nodes_registered, nodes_failed, nodes_retired, nodes_recovered, parser_ptr);
	OSDLOG(DEBUG, "GL Recovery Completed");
}

void GlobalLeader::gfs_file_writer()
{
	OSDLOG(INFO, "GL File writer thread started");
	GLUtils util_obj;
	this->gfs_available = true;
	int failure_count = 0;
	std::pair<std::string,bool> ret = util_obj.create_and_rename_GL_specific_file(GLOBAL_LEADER_STATUS_FILE);
	this->status_file = ret.first;
	while(this->gfs_available)
	{
		if(utime(this->status_file.c_str(), 0))
		{
			OSDLOG(INFO, "GL is unable to touch file on GFS");
			failure_count += 1;
			if(failure_count >= this->gfs_write_failure_count)
			{
				this->abort();
				this->gfs_available = false;
			}
		}
		else
		{
			failure_count = 0;
		}
		boost::this_thread::sleep( boost::posix_time::seconds(this->gfs_write_interval));
	}
}

void GlobalLeader::push(boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj)
{
	boost::mutex::scoped_lock lock(mtx);
	this->main_queue.push(msg_comm_obj);
	this->condition.notify_one();
}

boost::shared_ptr<MessageCommunicationWrapper> GlobalLeader::pop()
{
	boost::mutex::scoped_lock lock(mtx);
	while(this->main_queue.empty() and (!this->stop))
	{
		this->condition.wait(lock);
	}
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj;
	if(!this->main_queue.empty())
	{
		msg_comm_obj = this->main_queue.front();
		this->main_queue.pop();
	}
	return msg_comm_obj;
}

void GlobalLeader::abort()
{
	this->stop = true;
	this->condition.notify_one();
	OSDLOG(INFO, "GlobalLeader thread aborted");
	this->msg_handler->abort();		//GL Msg Handler Thread closed
	OSDLOG(INFO, "GL msg handler stopped");
	this->rec_identifier_thread->interrupt();	
	this->rec_identifier_thread->join();	//RI Thread closed
	OSDLOG(INFO, "RI-T stopped");
	this->mon_thread_mgr->stop_all_mon_threads();
	OSDLOG(INFO, "Monitoring stopped");
	this->states_thread_mgr->stop_all_state_mon_threads();
	OSDLOG(INFO, "Stopping all state mon threads");
	this->state_mgr_thread->interrupt();
	this->state_mgr_thread->join();
	OSDLOG(INFO, "State Change Initiator thread stopped");
	this->state_msg_ptr->abort();
	OSDLOG(INFO, "State msg handler stopped");
	this->gfs_available = false;
	OSDLOG(INFO, "gfs writer thread stopped");
        return; 
}	

void GlobalLeader::handle_main_queue_msg()
{
	OSDLOG(INFO, "Global Leader thread start");
	while(1)
	{
		boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj(this->pop());
		if(msg_comm_obj)
		{
			const uint32_t msg_type = msg_comm_obj->get_msg_result_wrap()->get_type();
			switch(msg_type)
			{
				case messageTypeEnum::typeEnums::NODE_ADDITION_CLI:
				{
					OSDLOG(INFO, "Node Addition message received");
					if(!this->cluster_recovery_in_progress)
					{
					boost::shared_ptr<comm_messages::NodeAdditionCli> node_add_msg(new comm_messages::NodeAdditionCli);
					node_add_msg->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					boost::thread node_add_thread(boost::bind(&GlobalLeader::handle_node_addition, this, node_add_msg, msg_comm_obj->get_comm_object()));
					OSDLOG(INFO, "Node Addition thread created");
					}
					else
					{
						OSDLOG(INFO, "Node Addition msg rejected because cluster recovery is in progress");
					}
					//this->handle_node_addition(node_add_msg, msg_comm_obj->get_comm_object());
				}
					break;
				case messageTypeEnum::typeEnums::NODE_DELETION:
				{
					OSDLOG(INFO, "Node Deletion message received");
					if(!this->cluster_recovery_in_progress)
					{
					boost::shared_ptr<comm_messages::NodeDeleteCli> node_del_msg(new comm_messages::NodeDeleteCli);
					node_del_msg->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					boost::thread node_del_thread(boost::bind(&GlobalLeader::handle_node_deletion, this, node_del_msg, msg_comm_obj->get_comm_object()));
					OSDLOG(INFO, "Node Deletion thread Created");
					}
					else
					{
						OSDLOG(INFO, "Node Deletion msg rejected because cluster recovery is in progress");
					}
					break;
				}
				case messageTypeEnum::typeEnums::NODE_STOP_CLI:		//To stop Global Leader
				{
					OSDLOG(INFO, "Node stop message received");
					if(!this->cluster_recovery_in_progress)
					{
						this->abort();
					}
					else
					{
						OSDLOG(INFO, "Node Stop msg rejected because cluster recovery is in progress");
					}
					break;
				}
				case messageTypeEnum::typeEnums::GET_OBJECT_VERSION:
				{
					OSDLOG(INFO, "Global Object Version request received");
					boost::shared_ptr<comm_messages::GetVersionID> version_obj(new comm_messages::GetVersionID);
					version_obj->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					this->handle_get_version_request(version_obj->get_service_id(), msg_comm_obj->get_comm_object());
				}
					break;
				case messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY:
				{
					OSDLOG(INFO, "Node Rejoin message received");
					if(!this->cluster_recovery_in_progress)
					{
					boost::shared_ptr<comm_messages::NodeRejoinAfterRecovery> rejoin_ptr(new comm_messages::NodeRejoinAfterRecovery);
					rejoin_ptr->deserialize(msg_comm_obj->get_msg_result_wrap()->get_payload(), msg_comm_obj->get_msg_result_wrap()->get_size());
					boost::thread node_rejoin_thread(boost::bind(&GlobalLeader::handle_node_rejoin_request, this, rejoin_ptr, msg_comm_obj->get_comm_object()));
					OSDLOG(INFO, "Node Rejoin thread start");
					}
					else
					{
					OSDLOG(INFO, "Node rejoin msg rejected because cluster recovery is in progress");
					}
					break;
				}
				case messageTypeEnum::typeEnums::GET_CLUSTER_STATUS:
				{
					OSDLOG(INFO, "Cluster status message received");
					std::map<std::string, network_messages::NodeStatusEnum> node_to_status_map = 
											this->monitoring->get_all_nodes_status();
					boost::shared_ptr<ErrorStatus> error_status;
					if(node_to_status_map.empty())
					{
						error_status.reset(new ErrorStatus(10, "Cluster is empty"));
					}
					else
					{
						error_status.reset(new ErrorStatus(0, "Retrieved cluster status"));
					}
					boost::shared_ptr<comm_messages::GetOsdClusterStatusAck> ack_msg(
								new comm_messages::GetOsdClusterStatusAck(
										node_to_status_map, error_status
										)
									);
					SuccessCallBack callback_obj;
					MessageExternalInterface::sync_send_get_cluster_status_ack(
						ack_msg.get(),
						msg_comm_obj->get_comm_object(),
						boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
						boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
						30
						);
					OSDLOG(INFO, "Cluster Status sent successfully");
					break;
				}
				case messageTypeEnum::typeEnums::INITIATE_CLUSTER_RECOVERY:
				{
					OSDLOG(INFO, "Initiating cluster Recovery");
					boost::thread node_rejoin_thread(boost::bind(&GlobalLeader::initiate_cluster_recovery, this));
					break;
				}
				default:
					OSDLOG(ERROR, "Msg type not supported in Global Leader handler");
					break;
			}
		}
		else
		{
			break;
		}
	}
}

void GlobalLeader::initiate_cluster_recovery()
{
	OSDLOG(INFO, "Initiating Cluster Recovery");
	this->cluster_recovery_in_progress = true;
	//while(!this->states_thread_mgr->refresh_states_for_cluster_recovery(this->node_status_list_after_reclaim))
	while(!this->states_thread_mgr->refresh_states_for_cluster_recovery())
	{
		boost::this_thread::sleep(boost::posix_time::seconds(30));
	}

	OSDLOG(INFO, "Starting Cluster Recovery Procedure");
	//TODO: Clear states of StateChangeInitiator..
	//Unmount all container journal filesystems from each node
	boost::shared_ptr<cluster_recovery_component::ClusterRecoveryInitiator> ini_obj(new cluster_recovery_component::ClusterRecoveryInitiator(this->monitoring, this->states_thread_mgr, this->node_info_file_path, this->node_add_response_timeout));
	ini_obj->initiate_cluster_recovery();
	OSDLOG(INFO, "Cluster Recovery Completed");
	this->cluster_recovery_in_progress = false;

	// OBB-696 fix
	boost::thread::id tid;
	if(ini_obj->thr->get_id() != tid)
	{
		ini_obj->thr->join();
	}
}

void GlobalLeader::handle_get_version_request(std::string service_id, boost::shared_ptr<Communication::SynchronousCommunication> syn_comm)
{
	OSDLOG(DEBUG, "Get_version_request rcvd from " << service_id);
	char *buf = new char[50];
	memset(&buf[0], '\0', 50);

	//Reading version id from status file
	int fd = ::open(this->status_file.c_str(), O_RDONLY, S_IRUSR);
	::read(fd, buf, 8);
	uint64_t *ptr1 = reinterpret_cast<uint64_t *>(&buf[0]);
	OSDLOG(INFO, "Version ID read from status file: " << *ptr1);
	::close(fd);

	//Writing version id after incrementing it by 1
	uint64_t version_id = *ptr1;
	version_id += 1;
	memcpy(&buf[0], &version_id, sizeof(version_id));
	OSDLOG(INFO, "Incrementing and writing version id: " << version_id);

	fd = ::open(this->status_file.c_str(), O_RDWR | O_TRUNC, S_IRUSR);
	::write(fd, buf, 8);
	::close(fd);

	boost::shared_ptr<comm_messages::GetVersionIDAck> version_req_ack(new comm_messages::GetVersionIDAck(service_id, version_id, true));
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_get_version_id_ack(
			version_req_ack.get(),
			syn_comm,
			boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
			boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
			60
			);
	OSDLOG(INFO, "Get version Request from "<<service_id<<" acknowledged");
}

void GlobalLeader::handle_node_rejoin_request(
	boost::shared_ptr<comm_messages::NodeRejoinAfterRecovery> rejoin_ptr,
	boost::shared_ptr<Communication::SynchronousCommunication> syn_comm
)
{
	GLUtils util_obj;
	std::string node_id = rejoin_ptr->get_node_id();
	std::string node_ip = rejoin_ptr->get_node_ip();
	int32_t node_port = rejoin_ptr->get_node_port();
	//Check if node is already added
	if(this->monitoring->if_node_already_exists(node_id) and 
		(this->monitoring->get_node_status(node_id) == network_messages::RECOVERED or
		this->monitoring->get_node_status(node_id) == network_messages::NODE_STOPPED)
	)
	{
		OSDLOG(INFO, "Handling Node Rejoin Request for Node "<< node_id);
		boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3(
				new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path));
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd =
				rhandler3->get_record_from_id(node_id);

		if(node_info_file_rcrd->get_node_ip() == node_ip and
			node_info_file_rcrd->get_localLeader_port() == node_port)
		{
			OSDLOG(INFO, "Marking node "<< node_id <<" as REJOINING");
			this->monitoring->change_status_in_memory(node_id, network_messages::REJOINING);
			boost::shared_ptr<Communication::SynchronousCommunication> sync_ll_obj(
					new Communication::SynchronousCommunication(node_ip, node_port)
				);
			if(sync_ll_obj->is_connected())
			{
				boost::shared_ptr<comm_messages::NodeAdditionGl> na_gl_ptr(
							new comm_messages::NodeAdditionGl(node_id)
						);
				OSDLOG(INFO, "Sending node rejoin request to LL: (" << node_ip << ", " << node_port << ")");
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
						OSDLOG(INFO, "Rcvd Node Rejoin success from LL");
						bool success = this->monitoring->change_status_in_monitoring_component(node_id, network_messages::REGISTERED);	
						if(success)
						{
							this->states_thread_mgr->prepare_and_add_entry(node_id, system_state::NODE_ADDITION);
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
						}
						else
						{
							//Failed to change status in node_info_file. Revert to RECOVERED in memory.
							this->monitoring->change_status_in_memory(node_id, network_messages::RECOVERED);
						}
						
					}
					else
					{
						OSDLOG(INFO, "Rcvd Node Rejoin failure from LL");
						this->monitoring->change_status_in_memory(node_id, network_messages::RECOVERED);
					}
				}
				else
				{
					OSDLOG(INFO, "Couldn't rcv Node Rejoin Ack from LL");
					this->monitoring->change_status_in_memory(node_id, network_messages::RECOVERED);
				}
			}	
			else
			{
				OSDLOG(ERROR, "Could not connect to node " << node_ip);
				this->monitoring->change_status_in_memory(node_id, network_messages::RECOVERED);
			}
		}
		else
		{
			OSDLOG(INFO, "Node IP/PORT donot match as in Node Info file, ip: " << node_ip << ", port: " << node_port);

		}
	}
	else
	{
		OSDLOG(INFO, "Node Status not appropriate for Rejoin");
	}

}

bool GlobalLeader::add_node_in_info_file(
	std::list<std::pair<boost::shared_ptr<ServiceObject>, std::list<boost::shared_ptr<ServiceObject> > > > srcv_obj_list,
	int node_status
)
{
	GLUtils gl_utils_obj;
	//std::list<boost::shared_ptr<ServiceObject> >::iterator it = srcv_obj_list.begin();
	std::list<std::pair<boost::shared_ptr<ServiceObject>, std::list<boost::shared_ptr<ServiceObject> > > >::iterator it = srcv_obj_list.begin();
	bool status = true;
	std::list<boost::shared_ptr<recordStructure::Record> > node_info_records;
	for(; it != srcv_obj_list.end(); ++it)
	{
		std::string node_id = (*it).first->get_service_id();
		std::string node_ip = (*it).first->get_ip();
		OSDLOG(INFO, "Node ip is : " << node_ip);
		int32_t node_port = (*it).first->get_port();
		int32_t container_port = -1, account_port = -1, updater_port = -1, object_port = -1, proxy_port = -1;
		std::list<boost::shared_ptr<ServiceObject> >::iterator it1 = (*it).second.begin();
		for (; it1 != (*it).second.end(); ++it1)
		{
			if(gl_utils_obj.get_service_name((*it1)->get_service_id()) == CONTAINER_SERVICE)
				container_port = (*it1)->get_port();
			else if(gl_utils_obj.get_service_name((*it1)->get_service_id()) == ACCOUNT_SERVICE)
				account_port = (*it1)->get_port();
			else if(gl_utils_obj.get_service_name((*it1)->get_service_id()) == ACCOUNT_UPDATER)
				updater_port = (*it1)->get_port();
			else if(gl_utils_obj.get_service_name((*it1)->get_service_id()) == OBJECT_SERVICE)
				object_port = (*it1)->get_port();
			else if(gl_utils_obj.get_service_name((*it1)->get_service_id()) == PROXY_SERVICE)
				proxy_port = (*it1)->get_port();

		}
		boost::shared_ptr<recordStructure::Record> record_ptr(new recordStructure::NodeInfoFileRecord(node_id, node_ip, node_port, account_port, container_port, object_port, updater_port, proxy_port, node_status));
		node_info_records.push_back(record_ptr);
		
	}

	status = this->node_info_file_writer->add_record(node_info_records);
	
	return status;
}

void GlobalLeader::handle_node_addition(
	boost::shared_ptr<comm_messages::NodeAdditionCli> msg,
	boost::shared_ptr<Communication::SynchronousCommunication> syn_comm	
)
{
	OSDLOG(INFO, "Handling Node Addition message");
	boost::shared_ptr<comm_messages::NodeAdditionCliAck> na_ack(new comm_messages::NodeAdditionCliAck);

	std::list<boost::shared_ptr<ServiceObject> > srcv_obj_list = msg->get_node_objects();
	std::list<boost::shared_ptr<ServiceObject> >::iterator it = srcv_obj_list.begin();
	bool status = true;
	std::list<std::pair<boost::shared_ptr<ServiceObject>, std::list<boost::shared_ptr<ServiceObject> > > > success_list;
	for(; it != srcv_obj_list.end(); ++it)
	{
		GLUtils util_obj;
		std::string node_id = util_obj.get_node_id((*it)->get_service_id(), (*it)->get_port());
		(*it)->set_service_id(node_id);

		//Check if node is already added
		if(!this->monitoring->if_node_already_exists((*it)->get_service_id()))
		{
			boost::shared_ptr<Communication::SynchronousCommunication> sync_ll_obj(
					new Communication::SynchronousCommunication((*it)->get_ip(), (*it)->get_port())
				);
			if(sync_ll_obj->is_connected())
			{

				boost::shared_ptr<comm_messages::NodeAdditionGl> na_gl_ptr(
						new comm_messages::NodeAdditionGl((*it)->get_service_id())
						);
				OSDLOG(INFO, "Sending node addition request to LL: (" << (*it)->get_ip() << ", " << (*it)->get_port() << ")");
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

					if(ack_frm_ll_msg->get_message_object()->get_error_status()->get_code() ==
										OSDException::OSDSuccessCode::NODE_ADDITION_SUCCESS)
					{
						OSDLOG(INFO, "Rcvd Node Addition success from LL");
						boost::shared_ptr<comm_messages::NodeAdditionFinalAck> final_ack_ptr;
						if(!this->mon_thread_mgr->add_node_for_monitoring((*it)))
						{
							OSDLOG(INFO, "Cluster recovery is in progress, cannot add node");
							boost::shared_ptr<ErrorStatus> err_obj(new ErrorStatus(20, "Cannot add node while cluster recovery is in progress"));
							na_ack->add_node_in_node_status_list((*it), err_obj);
							final_ack_ptr.reset(new comm_messages::NodeAdditionFinalAck(false));
						}
						else
						{
							success_list.push_back(std::make_pair(*it, ack_frm_ll_msg->get_message_object()->get_service_objects()));
							final_ack_ptr.reset(new comm_messages::NodeAdditionFinalAck(true));
						}

						SuccessCallBack callback_obj1;
						MessageExternalInterface::sync_send_node_add_final_ack(
								final_ack_ptr.get(),
								sync_ll_obj,
								boost::bind(&SuccessCallBack::success_handler, &callback_obj1, _1),
								boost::bind(&SuccessCallBack::failure_handler, &callback_obj1),
								60
						);
						
					}
					else
					{
						OSDLOG(INFO, "Rcvd Node Addition failure from LL");
						boost::shared_ptr<ErrorStatus> err_obj(new ErrorStatus(10, "LL couldnot start OSD services"));
						na_ack->add_node_in_node_status_list((*it), err_obj);
					}
				}
				else
				{
					OSDLOG(INFO, "Couldn't rcv Node Addition Ack from LL");
					boost::shared_ptr<ErrorStatus> err_obj(new ErrorStatus(10, "Couldnot rcv NA ack from LL"));
					na_ack->add_node_in_node_status_list((*it), err_obj);
				}
			}
			else
			{
				OSDLOG(ERROR, "Could not connect to node " << (*it)->get_ip());
				boost::shared_ptr<ErrorStatus> err_obj(new ErrorStatus(10, "Could not connect to node"));
				na_ack->add_node_in_node_status_list((*it), err_obj);
			}
		}
		else
		{
			OSDLOG(ERROR, "Node is already added in cluster");
			boost::shared_ptr<ErrorStatus> err_obj(new ErrorStatus(20, "Node Already Added"));
			na_ack->add_node_in_node_status_list((*it), err_obj);
		}
	}
	if(!success_list.empty())
	{
		OSDLOG(DEBUG, "Success List for Node Addition is not empty");
		status = this->add_node_in_info_file(success_list, network_messages::REGISTERED);	//Changed seq as per new discussion
		if(status)
		{
			OSDLOG(INFO, "Node added in Node Info File: " << status);
			std::list<std::pair<boost::shared_ptr<ServiceObject>, std::list<boost::shared_ptr<ServiceObject> > > >::iterator it1 = success_list.begin();
			for(; it1 != success_list.end(); ++it1)
			{
				//this->mon_thread_mgr->add_node_for_monitoring((*it1).first);	//First check in memory, only then add entry in journal and node info file
				this->states_thread_mgr->prepare_and_add_entry((*it1).first->get_service_id(), system_state::NODE_ADDITION);
				boost::shared_ptr<ErrorStatus> err_obj(new ErrorStatus(0, "Node Added successfully"));
				na_ack->add_node_in_node_status_list((*it1).first, err_obj);
			}
		}
		else
		{
			OSDLOG(INFO, "Unable to add entry in Node Info File");
			std::list<std::pair<boost::shared_ptr<ServiceObject>, std::list<boost::shared_ptr<ServiceObject> > > >::iterator it1 = success_list.begin();
			for(; it1 != success_list.end(); ++it1)
			{
				boost::shared_ptr<ErrorStatus> err_obj(new ErrorStatus(10, "Unable to add nodes in Node Info File"));
				na_ack->add_node_in_node_status_list((*it1).first, err_obj);
				this->mon_thread_mgr->remove_node_from_monitoring(((*it1).first)->get_service_id());
			
			}
		}
	}
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_node_addition_ack(
					na_ack.get(),
					syn_comm,
					boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
					boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
					0
				);
	OSDLOG(INFO, "Node Addition Completed");	
}

void GlobalLeader::handle_node_deletion(
	boost::shared_ptr<comm_messages::NodeDeleteCli> msg,
	boost::shared_ptr<Communication::SynchronousCommunication> syn_comm
)
{
	std::string node_id = msg->get_node_serv_obj()->get_service_id();
	bool status = false;
	OSDLOG(INFO, "Handling node deletion message for " << node_id);
	if(this->monitoring->if_node_already_exists(node_id))
	{
		status = this->monitoring->remove_node_from_monitoring(node_id);
		if(status)
			status = this->node_info_file_writer->delete_record(node_id);
	}
	else
	{
		OSDLOG(WARNING, "Node does not exist in GL Health Monitoring list");
	}
	this->send_node_deletion_ack(node_id, syn_comm, status);
}

void GlobalLeader::send_node_deletion_ack(
	std::string node_id,
	boost::shared_ptr<Communication::SynchronousCommunication> syn_comm,
	bool status
)
{
	boost::shared_ptr<ErrorStatus> err;
	if(status)
	{
		err.reset(new ErrorStatus(0, "Node has been deleted from OSD cluster"));
	}
	else
	{
		err.reset(new ErrorStatus(10, "Node could not be deleted"));
	}
	boost::shared_ptr<comm_messages::NodeDeleteCliAck> node_del_ack(new comm_messages::NodeDeleteCliAck(node_id, err));
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_node_deletion_ack(
			node_del_ack.get(),
			syn_comm,
			boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
			boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
			0
		);
	OSDLOG(INFO, "Sent Node Del Ack to CLI");
}

} // namespace global_leader
