#include <iostream>
#include <stdlib.h>
#include <set>
#include <unistd.h>
#include <boost/foreach.hpp>

#include "communication/callback.h"
#include "osm/osmServer/msg_handler.h"
#include "communication/message_type_enum.pb.h"
#include "osm/globalLeader/state_change_initiator.h"
#include "osm/globalLeader/monitoring.h"
#include "osm/osmServer/osm_info_file.h"

using namespace objectStorage;
namespace gl_monitoring
{

HealthMonitoringRecord::HealthMonitoringRecord(
	int msg_type,
	time_t last_updated_time,
	int status,
	int timeout,
	int hrbt_failure_count,
	int hfs_stat,
	int node_state
):
	msg_type(msg_type),
	last_updated_time(last_updated_time),
	status(status),
	timeout(timeout),
	failure_count(0),
	hrbt_failure_count(hrbt_failure_count),
	hfs_stat(hfs_stat),
	node_state(node_state)
{
}

void HealthMonitoringRecord::set_node_state(int node_state)
{

}

void HealthMonitoringRecord::set_hfs_stat(int hfs_stat)
{
	this->hfs_stat = hfs_stat;
}

int HealthMonitoringRecord::get_hfs_stat()
{
	return this->hfs_stat;
}

void HealthMonitoringRecord::set_hrbt_failure_count(int count)
{
	this->hrbt_failure_count = count;
}

void HealthMonitoringRecord::increment_failure_count()
{
	this->failure_count++;
}

int HealthMonitoringRecord::get_failure_count()
{
	return this->failure_count;
}

void HealthMonitoringRecord::set_expected_msg_type(int type)
{
	this->msg_type = type;
}

void HealthMonitoringRecord::set_timeout(int timeout_value)
{
	this->timeout = timeout_value;
}

int HealthMonitoringRecord::get_timeout()
{
	return (this->timeout*this->hrbt_failure_count);
}

int HealthMonitoringRecord::get_expected_msg_type()
{
	return this->msg_type;
}

time_t HealthMonitoringRecord::get_last_updated_time()
{
	return this->last_updated_time;
}

void HealthMonitoringRecord::set_last_updated_time(time_t last_updated_time)
{
	this->last_updated_time = last_updated_time;
}

int HealthMonitoringRecord::get_status()
{
	return this->status;
}

void HealthMonitoringRecord::set_status(int status)
{
	this->status = status;
}

MonitoringThreadManager::MonitoringThreadManager(boost::shared_ptr<Monitoring> monitoring
):
        monitoring(monitoring)
{
	/* One bucket and corresponding monitoring thread needs to be created in constructor too */
	this->create_bucket();
}

bool MonitoringThreadManager::add_node_for_monitoring(boost::shared_ptr<ServiceObject> msg)
{
	return this->monitoring->add_entry_in_map(msg);
}

void MonitoringThreadManager::remove_node_from_monitoring(std::string node_id)
{
	this->monitoring->remove_node_from_monitoring(node_id);
}

/* When STRM is rcvd from any LL */
void MonitoringThreadManager::monitor_service(
	boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_obj,
	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm
)
{
	bool status = this->monitoring->update_map(strm_obj);
	boost::shared_ptr<comm_messages::LocalLeaderStartMonitoringAck> ll_strm_ack(new comm_messages::LocalLeaderStartMonitoringAck(strm_obj->get_service_id()));
	if ( status )
	{
		OSDLOG(DEBUG, "Map Updated");
		if(this->bucket_fd_ratio.empty()) // TODO also check if bucket max limit reached
		{
			OSDLOG(DEBUG, "Creating Bucket");
			this->create_bucket();
		}
		this->update_bucket(sync_comm);
		ll_strm_ack->set_status(true);
		OSDLOG(INFO, "STRM accepted for node: " << strm_obj->get_service_id() << ", sending status: " << ll_strm_ack->get_status());
	}	
	else
	{
		OSDLOG(INFO, "STRM not  accepted for node: " << strm_obj->get_service_id());
		ll_strm_ack->set_status(false);	
	}
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_local_leader_start_monitoring_ack(
				ll_strm_ack.get(),
				sync_comm,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				30
			);

}

void MonitoringThreadManager::create_bucket()
{
	OSDLOG(INFO, "Creating bucket");
	boost::shared_ptr<Bucket> bucket_ptr(new Bucket);
	bucket_ptr->set_fd_limit(100);
	this->bucket_fd_ratio.push_back(std::tr1::tuple<boost::shared_ptr<Bucket>, int64_t>(bucket_ptr, 0));
	boost::shared_ptr<MonitoringThread> mon_thread_ptr(new MonitoringThread(this->monitoring, bucket_ptr));
	boost::shared_ptr<boost::thread> mon_thread(new boost::thread(&MonitoringThread::run, mon_thread_ptr));
	this->mon_threads.push_back(mon_thread);
}

void MonitoringThreadManager::stop_all_mon_threads()
{
	std::vector<boost::shared_ptr<boost::thread> >::iterator thread_it = this->mon_threads.begin();
	for ( ; thread_it != this->mon_threads.end(); ++thread_it )
	{
		(*thread_it)->interrupt();
		(*thread_it)->join();
	}
}

void MonitoringThreadManager::update_bucket(
	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm
)
{
	std::list<std::tr1::tuple<boost::shared_ptr<Bucket>, int64_t> >::iterator it = this->bucket_fd_ratio.begin();
	for (; it != this->bucket_fd_ratio.end(); ++it)
	{
		if ((std::tr1::get<0>(*it))->get_fd_limit() > std::tr1::get<1>(*it))
		{
			OSDLOG(DEBUG, "Breaking.... ");
			break;
		}
	}
	if(it != this->bucket_fd_ratio.end())
	{
		OSDLOG(DEBUG, "Adding fd: "<<sync_comm->get_fd()<<" in bucket");
		(std::tr1::get<0>(*it))->add_fd(sync_comm->get_fd(), sync_comm);
	}
}

MonitoringThread::MonitoringThread(
	boost::shared_ptr<Monitoring> monitoring,
	boost::shared_ptr<Bucket> bucket_ptr
):
	monitoring(monitoring),
	bucket(bucket_ptr)
{
    	GLUtils util_obj;
	this->config_parser = util_obj.get_config();
	OSDLOG(INFO, "New Monitoring thread created");
}

bool MonitoringThread::add_bucket(boost::shared_ptr<Bucket> bucket_ptr)
{
	this->bucket = bucket_ptr;
	return true;
}

boost::shared_ptr<comm_messages::HeartBeat> MonitoringThread::receive_heartbeat(int64_t fd)
{
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj = this->bucket->get_comm_obj(fd);
	boost::shared_ptr<MessageResult<comm_messages::HeartBeat> > hb_msg_wrap = MessageExternalInterface::sync_read_heartbeat(comm_obj, 20);
	boost::shared_ptr<comm_messages::HeartBeat> hb;
	if(hb_msg_wrap->get_status() == true)
	{
		hb.reset(hb_msg_wrap->get_message_object());
	}
	else    //if any error(socket error or type error), remove fd from bucket
	{
		OSDLOG(DEBUG, "removing fd from bucket" << fd);
		this->bucket->remove_fd(fd);
	}
	return hb;
}

//UNCOVERED_CODE_BEGIN:
void MonitoringThread::close_all_sockets()
{
	std::set<int64_t> ll_fd_set = this->bucket->get_fd_list();
	BOOST_FOREACH(int64_t fd, ll_fd_set)
	{
		close(fd);
	}
}
//UNCOVERED_CODE_END

void MonitoringThread::run()
{
	OSDLOG(INFO, "Starting hrbt receiver thread");
	GLUtils utils_obj;
	try
	{
		while(1)
		{
			boost::this_thread::sleep( boost::posix_time::seconds(5));	//TODO: Just for testing purpose
			boost::this_thread::interruption_point();
			int error;
			std::set<int64_t> ll_fd_set = this->bucket->get_fd_list();
			for (std::set<int64_t>::iterator it = ll_fd_set.begin(); it != ll_fd_set.end(); ++it) {
				OSDLOG(DEBUG, "Elements in bucket" << *it);
			}
			if (ll_fd_set.empty())
			{
				continue;
			}
			std::set<int64_t> active_fds = utils_obj.selectOnSockets(ll_fd_set, error, this->config_parser->hrbt_select_timeoutValue());
			if ( error != 0 )
			{
				//OSDLOG(ERROR, "Error while selecting on sockets: " << error);
				//check for fatal error else continue
			}
			for (std::set<int64_t>::iterator it = active_fds.begin(); it != active_fds.end(); ++it)
			{
				boost::shared_ptr<comm_messages::HeartBeat> msg(this->receive_heartbeat(*it));
				if(msg)
				{
					OSDLOG(INFO, "Rcvd heartbeat from " << msg->get_service_id());
					//if(!this->monitoring->update_time_in_map(msg->get_service_id())) //Node status is marked failed
					if(!this->monitoring->update_time_in_map(msg->get_service_id(), msg->get_hfs_stat())) //Node status is marked failed
					{
						OSDLOG(INFO, "Node status is marked failed, removing fd from bucket" << *it);
						this->bucket->remove_fd(*it);
						continue;
					}
					this->send_heartbeat_ack(*it, msg->get_sequence(), msg->get_service_id());
				}
			}
		}
	}
	catch(const boost::thread_interrupted&)
	{
		OSDLOG(INFO, "Closing Monitoring Thread");
	}
}

void MonitoringThread::send_heartbeat_ack(int64_t fd, uint32_t seq_no, std::string node_id)
{
	int node_status = this->monitoring->get_node_status(node_id);
	boost::shared_ptr<comm_messages::HeartBeatAck> hb_ack(new comm_messages::HeartBeatAck(seq_no, node_status));
	boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr = this->bucket->get_comm_obj(fd);
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_heartbeat_ack(
		hb_ack.get(),
		comm_ptr,
		boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
		boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
		30
		);
	OSDLOG(DEBUG, "Sent hb ack to LL");
}

//Not needed anymore
/*
void MonitoringThread::update_time_in_map(std::string node_id)
{
//	this->monitoring->update_time_in_map(node_id);
}*/

Monitoring::~Monitoring()
{
	;
}

/*cool::SharedPtr<config_file::OsdServiceConfigFileParser> get_osm_service_config()
{
        cool::config::ConfigurationBuilder<config_file::OsdServiceConfigFileParser> config_file_builder;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config_dir)
		config_file_builder.readFromFile(std::string(osd_config_dir).append("/OsdServiceConfigFile.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/OsdServiceConfigFile.conf");
	return config_file_builder.buildConfiguration();
}*/

MonitoringImpl::MonitoringImpl(
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer
):
	node_info_file_writer(node_info_file_writer)
{
	GLUtils util_obj;
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> parser_ptr = util_obj.get_osm_timeout_config();
	cool::SharedPtr<config_file::OSMConfigFileParser> osm_parser_ptr = util_obj.get_config();
	this->hrbt_failure_count = parser_ptr->ll_hrbt_failure_countValue();
	this->strm_failure_count = parser_ptr->ll_strm_failure_countValue();
	this->hrbt_timeout = parser_ptr->heartbeat_timeoutValue();
	this->strm_timeout = parser_ptr->ll_strm_timeoutValue();
	this->system_stop_file = osm_parser_ptr->system_stop_fileValue();
	this->system_stop = false;
	this->cluster_recovery_identification_count = 0;
	this->cluster_recovery_in_progress = false;
}


void MonitoringImpl::initialize_prepare_and_add_entry(boost::function<bool(std::string, int)> prepare_and_add_entry)
{
	this->prepare_and_add_entry = prepare_and_add_entry;
}

bool MonitoringImpl::if_node_already_exists(std::string node_id)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.find(node_id);
	if(it != this->active_node_map.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

/* Only called when STRM is received from LL */
bool MonitoringImpl::update_map(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> msg)
{
	OSDLOG(INFO, "Updating Active Node Map for service id: " << msg->get_service_id());
	boost::recursive_mutex::scoped_lock lock(this->mtx);

	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.find(msg->get_service_id());
	bool status = false;
	if ( it != this->active_node_map.end())
	{
		if(it->second->get_status() == network_messages::NEW or it->second->get_status() == network_messages::RETIRING or (it->second->get_status() == network_messages::RETIRED and msg->get_type() == messageTypeEnum::typeEnums::LL_START_MONITORING))
		{
			status = false;
		}
		else if(msg->get_type() == messageTypeEnum::typeEnums::NODE_RETIRE)
		{
			if(it->second->get_status() == network_messages::FAILED)
				status = false;
			else if(it->second->get_status() == network_messages::RECOVERED)
			{
				status = true;
				it->second->set_status(network_messages::RETIRED);
			}
			else
			{
				it->second->set_status(network_messages::RETIRING);
				status = false;
			}
		}
		else
		{
			time_t now;
			time(&now);
			it->second->set_last_updated_time(now);
			it->second->set_expected_msg_type(HRBT);
//			it->second->set_comm_obj(sync_comm);
			//it->second->set_status(RUNNING);
			it->second->set_timeout(this->hrbt_timeout);
			it->second->set_hrbt_failure_count(this->hrbt_failure_count);
			status = true;
		}
	}
	return status;
}

bool MonitoringImpl::register_status(std::string node_id, int operation, int status)
{
	bool mod_status = false;
	if(!prepare_and_add_entry.empty())
	{
		mod_status = prepare_and_add_entry(node_id, operation);
		if(mod_status)
		{
			bool success = this->node_info_file_writer->update_record(node_id, status);
			if ( success )
			{
				OSDLOG(INFO, "Node Info File updated with status " << status << " for node " << node_id);
				return true;
			}
			else
			{
				OSDLOG(ERROR, "Node Info File could not be opened for updating status for node " << node_id);
				return false;
			}
		}
		else
		{
			return false;
		}
		
	}
	else
	{
		OSDLOG(ERROR, "Function pointer empty");
	}
	return false;

}

int MonitoringImpl::get_node_status(std::string node_id)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.find(node_id);
	if(it != this->active_node_map.end())
	{
		return it->second->get_status();
	}
	else
	{
		return network_messages::INVALID_NODE;
	}
}

void MonitoringImpl::retrieve_node_status(
	boost::shared_ptr<comm_messages::NodeStatus> node_status_req,
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj
)
{	
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = 
						this->active_node_map.find(node_status_req->get_node()->get_service_id());
	boost::shared_ptr<comm_messages::NodeStatusAck> node_status_ack;
	if(it != this->active_node_map.end())
	{
		node_status_ack.reset(new comm_messages::NodeStatusAck(network_messages::NodeStatusEnum(it->second->get_status())));
	}
	else
	{
		node_status_ack.reset(new comm_messages::NodeStatusAck(network_messages::NodeStatusEnum(network_messages::INVALID_NODE)));

	}
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_node_status_ack(
				node_status_ack.get(),
				comm_obj,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				30
				);

}

void MonitoringImpl::get_nodes_status(
	std::vector<std::string> &nodes_registered,
	std::vector<std::string> &nodes_failed,
	std::vector<std::string> &nodes_retired,
	std::vector<std::string> &nodes_recovered
)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.begin();
	for(; it != this->active_node_map.end(); ++it)
	{
		if(it->second->get_status() == network_messages::REGISTERED)
		{
			nodes_registered.push_back(it->first);
		}
		else if(it->second->get_status() == network_messages::FAILED or
			it->second->get_status() == network_messages::RETIRING_FAILED or
			it->second->get_status() == network_messages::NODE_STOPPING) //Handling NODE_STOPPING same as FAILED
		{
			nodes_failed.push_back(it->first);
		}
		else if(it->second->get_status() == network_messages::RETIRING)
		{
			nodes_retired.push_back(it->first);
		}
		else if(it->second->get_status() == network_messages::RECOVERED or
			it->second->get_status() == network_messages::NODE_STOPPED or
			it->second->get_status() == network_messages::RETIRED or
			it->second->get_status() == network_messages::RETIRED_RECOVERED
			)
		{
			nodes_recovered.push_back(it->first);
		}
	}
}

void MonitoringImpl::get_node_status_while_cluster_recovery(
	std::vector<std::string> &nodes_unknown,
	std::vector<std::string> &nodes_totally_failed,
	std::vector<std::string> &nodes_in_stopping_state
)
{
	nodes_unknown = this->unknown_status_nodes;
	nodes_totally_failed = this->totally_failed_node_list;
	nodes_in_stopping_state = this->stopping_nodes;

}

std::map<std::string, network_messages::NodeStatusEnum> MonitoringImpl::get_all_nodes_status()
{
	std::map<std::string, network_messages::NodeStatusEnum> ret;
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.begin();
	for(; it != this->active_node_map.end(); ++it)
	{
		ret.insert(std::make_pair(it->first, static_cast<network_messages::NodeStatusEnum>(it->second->get_status())));
	}
	return ret;
}

void MonitoringImpl::monitor_for_recovery()		//Recovery Identifier Thread
{
	OSDLOG(INFO, "Starting Recovery Identifier Thread")
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it;
	try
	{
		for ( it = this->active_node_map.begin(); it != this->active_node_map.end(); ++it )
		{
			time_t now;
			time(&now);
			it->second->set_last_updated_time(now);
		}

		while(1)
		{
			int healthy_node_count = 0;
			this->mtx.lock();
			for ( it = this->active_node_map.begin(); it != this->active_node_map.end(); ++it )
			{
				std::string node_id = it->first;
				time_t now;
				time(&now);
				OSDLOG(DEBUG, "HM: Last updated time diff: " << (now - it->second->get_last_updated_time()));
				if( ((now - it->second->get_last_updated_time()) > it->second->get_timeout())
					and ((it->second->get_status() != network_messages::FAILED)
					and (it->second->get_status() != network_messages::RECOVERED)
					and (it->second->get_status() != network_messages::REJOINING)
					and (it->second->get_status() != network_messages::RETIRED) 
					and (it->second->get_status() != network_messages::NODE_STOPPED)
					and (!cluster_recovery_in_progress))
				)
				{
					if (!this->system_stop && boost::filesystem::exists(this->system_stop_file))
					{
						this->system_stop = true;
						OSDLOG(INFO, "System Stop in Progress. Skipping node " << node_id << " for marking as FAILED");	
					}
					else if (!this->system_stop && !(boost::filesystem::exists(this->system_stop_file)))
					{
						OSDLOG(INFO, "Setting status of node " << node_id << " as FAILED");
						bool reg;	//to ensure necessary changes are done in State Map and node info file
						int state;
						if(it->second->get_status() == network_messages::RETIRING)
						{
							this->mtx.unlock();
							reg = this->register_status(node_id, system_state::NODE_RECOVERY, network_messages::RETIRING_FAILED);
							state = network_messages::RETIRING_FAILED;
						}
						else
						{
							this->mtx.unlock();
							GLUtils util_obj;
							std::string node_name = util_obj.get_node_id(node_id);
							CLOG(SYSMSG::OSD_SERVICE_STOP, node_name.c_str());
							reg = this->register_status(node_id, system_state::NODE_RECOVERY, network_messages::FAILED);
							state = network_messages::FAILED;
						}
						this->mtx.lock();
						if(reg)
							it->second->set_status(state);
					}
					else
					{
						OSDLOG(INFO, "System Stop in Progress. Skipping node " << node_id << " for marking as FAILED");	
						if (!(boost::filesystem::exists(this->system_stop_file)))
						{
							this->system_stop = false;
						}
					}
					
				}
				else if(it->second->get_status() == network_messages::REGISTERED or
								it->second->get_status() == network_messages::TRANSITION_STATE
								or it->second->get_status() == network_messages::REJOINING)
				{
					healthy_node_count+=1;
				}
			}
			if(healthy_node_count == 0 and !(this->active_node_map.empty()))
			{
				this->mtx.unlock();
				OSDLOG(INFO, "Healthy node count found 0 by Mon comp, initiating cluster recovery");
				this->cluster_recovery_identification_count+=1;
				this->initiate_cluster_recovery();
			}
			this->mtx.unlock();
			boost::this_thread::interruption_point();
			boost::this_thread::sleep(boost::posix_time::seconds(5));	//TODO: Just for testing purpose
		}
	}
	catch(const boost::thread_interrupted&)
	{
		OSDLOG(INFO, "Closing Recovery Identifier Thread");
	}
}

void MonitoringImpl::reset_cluster_recovery_details()
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	OSDLOG(INFO, "Reseting cluster recovery related details in Monitoring Component");
	this->cluster_recovery_identification_count = 0;
	this->cluster_recovery_in_progress = false;
	std::vector<std::string>::iterator it = this->unknown_status_nodes.begin();
	for(; it != this->unknown_status_nodes.end(); ++it)
	{
		std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator itr = this->active_node_map.find(*it);
		if (itr != this->active_node_map.end())
		{
			if (itr->second->get_status() == network_messages::TRANSITION_STATE)
			{
				OSDLOG(INFO, "Reverting node state of " << *it << " into failed state from transition state.");
				itr->second->set_status(network_messages::FAILED);
			}
		}
	}
	this->unknown_status_nodes.clear();
	this->stopping_nodes.clear();
	this->totally_failed_node_list.clear();
}

void MonitoringImpl::initiate_cluster_recovery()
{
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it;
	size_t failed_node_count;
	this->mtx.lock();
	for ( it = this->active_node_map.begin(); it != this->active_node_map.end(); ++it )
	{
		if (it->second->get_status() == network_messages::REJOINING)
		{
			OSDLOG(INFO, "Aborting Cluster Recovery as node"<< it->first <<" is Rejoining in cluster");
			this->totally_failed_node_list.clear();
			this->unknown_status_nodes.clear();
			return;
		}
		else if(it->second->get_status() == network_messages::FAILED or it->second->get_status() == network_messages::RECOVERED or it->second->get_status() == network_messages::NODE_STOPPING)
		{
			failed_node_count+= 1;
			OSDLOG(DEBUG, "Node " << it->first << " has " << it->second->get_hfs_stat());
			if(it->second->get_hfs_stat() != OSDException::OSDHfsStatus::NOT_READABILITY)
			{
				OSDLOG(INFO, it->first << " has readability");
				this->unknown_status_nodes.push_back(it->first);
			}
			else
			{
				OSDLOG(INFO, it->first << " doesn't has readability");
				this->totally_failed_node_list.push_back(it->first);
			}
		}
		/*else if(it->second->get_status() == network_messages::NODE_STOPPING) //Considering NODE_STOPPING in cluster recovery
		{
			this->stopping_nodes.push_back(it->first);
		}*/
	}
	this->mtx.unlock();
	if(this->unknown_status_nodes.empty())
	{
		OSDLOG(INFO, "No nodes can be put into transition state");
		this->totally_failed_node_list.clear();
		this->unknown_status_nodes.clear();
		return;
	}
	else if(this->unknown_status_nodes.size() == failed_node_count or this->cluster_recovery_identification_count > 1)
	{
		//move all node in unknown_status_nodes array to TRANSITION_STATE state
		OSDLOG(DEBUG, "Unknown status nodes count is " << this->unknown_status_nodes.size());
		std::vector<std::string>::iterator it = this->unknown_status_nodes.begin();
		this->mtx.lock();
		for(; it != this->unknown_status_nodes.end(); ++it)
		{
			std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator itr = this->active_node_map.find(*it);
			if (itr != this->active_node_map.end())
			{
				OSDLOG(INFO, "Changing node state of " << *it << " into transition state from "<<itr->second->get_status()<<" state");
				itr->second->set_status(network_messages::TRANSITION_STATE);
			}
		}
		this->mtx.unlock();
		OsmUtils obj;
		boost::tuple<std::string, uint32_t> info = obj.get_ip_and_port_of_gl();
		OSDLOG(INFO, "Sending cluster recovery msg to GL with ip and port: "<< boost::get<0>(info) << " " << boost::get<1>(info));
		boost::shared_ptr<Communication::SynchronousCommunication> comm;
		comm.reset(new Communication::SynchronousCommunication(boost::get<0>(info), boost::get<1>(info)));
		if(comm->is_connected())
		{
			boost::shared_ptr<comm_messages::InitiateClusterRecovery> rec_ptr(new comm_messages::InitiateClusterRecovery());
			SuccessCallBack callback_obj;
			MessageExternalInterface::sync_send_initiate_cluster_recovery_message(
				rec_ptr.get(),
				comm,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj),
				0
			);
			//Failure should not occur ideally in this case, because GL is sending msg to itself
		}
		else
		{
			this->reset_cluster_recovery_details();
		}
		//send msg to GL to start recovery
		this->cluster_recovery_identification_count = 0;
		this->cluster_recovery_in_progress = true;
	}
	else
	{
		OSDLOG(INFO, "Sleeping for 120 seconds");
		this->totally_failed_node_list.clear();
		this->unknown_status_nodes.clear();
		boost::this_thread::sleep(boost::posix_time::seconds(120));
		return;
	}
}

bool MonitoringImpl::change_status_in_memory(std::string node_id, int status)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.find(node_id);
	if(it != this->active_node_map.end())
	{
		OSDLOG(INFO, "Changing status of " << node_id << " to " << status << " in memory");
		it->second->set_status(status);
		return true;
	}
	return false;
}

//Called by StateChangeInitiator, need to check if lock is needed here
bool MonitoringImpl::change_status_in_monitoring_component(std::string node_id, int status)
{

	bool success = false;
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.find(node_id);
	if(it != this->active_node_map.end())
	{
		if(it->second->get_status() == network_messages::RETIRING_FAILED and status == network_messages::RECOVERED)
		{
			//If previous state is anyhow related to retiring, never set it as recovered
			OSDLOG(INFO, "Changing state from RET_FAILED to RET_RECOVERED");
			status = network_messages::RETIRED_RECOVERED;
		}
		else if(it->second->get_status() == network_messages::RETIRING and status == network_messages::NODE_STOPPED)
		//TODO: Check if NODE_STOPPING also needs to be handled
		{
			status = network_messages::RETIRED;
		}
		
		success = this->node_info_file_writer->update_record(node_id, status);
		if(success)
		{
			it->second->set_status(status);
			if(status == network_messages::REGISTERED)
			{
				time_t now;
				time(&now);
				it->second->set_last_updated_time(now);
			}
			OSDLOG(INFO, "Setting status of node " << it->first << " as " << status);
		}
		else
		{
			//OSDLOG(FATAL, "Node Info File could not be opened for updating status for node " << node_id);
			OSDLOG(ERROR, "Node Info File could not be opened for updating status for node " << node_id);
			//raise exception?
		}
	}
	else
	{
		OSDLOG(ERROR, "Node not found in Monitoring Map");
	}
	return success;
}

bool MonitoringImpl::remove_node_from_monitoring(std::string node_id)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.find(node_id);
	if(it != this->active_node_map.end() and it->second->get_status() != network_messages::RETIRED)
	{
		this->active_node_map.erase(it);
		return true;
	}
	else
	{
		OSDLOG(WARNING, "Could not delete node from monitoring list due to invalid node status: " << it->second->get_status());
	}
	return false;
}

bool MonitoringImpl::update_time_in_map(std::string node_id, int hfs_stat)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator it = this->active_node_map.find(node_id);
	//if(it != this->active_node_map.end() and it->second->get_status() != network_messages::FAILED)
	if(it != this->active_node_map.end())
	{
		time_t now;
		time(&now);
		it->second->set_last_updated_time(now);
		it->second->set_hfs_stat(hfs_stat);
		return true;
	}
	return false;
}

bool MonitoringImpl::add_entries_while_recovery(std::string node_id, int msg_type, int status)
{
	time_t now;
	time(&now);
	OSDLOG(INFO, "Adding entry in monitoring comp while recovery for node: " << node_id << " with status " << status);
	boost::shared_ptr<HealthMonitoringRecord> health_record(new HealthMonitoringRecord(msg_type, now, status, this->strm_timeout, this->strm_failure_count, OSDException::OSDHfsStatus::NOT_READABILITY, 1));
	std::pair<std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator, bool> ret_status;
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	ret_status = this->active_node_map.insert(std::pair<std::string, boost::shared_ptr<HealthMonitoringRecord> >(node_id, health_record));
	return ret_status.second;
}

bool MonitoringImpl::add_entry_in_map(boost::shared_ptr<ServiceObject> msg)
{
	time_t now;
	time(&now);
	boost::shared_ptr<HealthMonitoringRecord> health_record(new HealthMonitoringRecord(STRM, now, network_messages::REGISTERED, this->strm_timeout, this->hrbt_failure_count, OSDException::OSDHfsStatus::NOT_READABILITY, 1));
	std::pair<std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> >::iterator, bool> ret_status;

	boost::recursive_mutex::scoped_lock lock(this->mtx);
	if(cluster_recovery_in_progress)
		return false;
	ret_status = this->active_node_map.insert(std::make_pair(msg->get_service_id(), health_record));
	return ret_status.second;
}

MonitoringImpl::~MonitoringImpl()
{
	;
}

} // namespace gl_monitoring
