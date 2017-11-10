#ifndef __GL_MONITORING_456__
#define __GL_MONITORING_456__

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <tr1/tuple>
#include <vector>
#include <map>

#include "communication/message_result_wrapper.h"
#include "communication/stubs.h"
#include "communication/message.h"
#include "communication/communication.h"
#include "osm/osmServer/utils.h"

//using namespace std::tr1;

namespace system_state
{
	class StateChangeThreadManager;
}

namespace osm_info_file_manager
{
	class NodeInfoFileWriteHandler;
}

namespace gl_monitoring
{

enum OSDNodeStatus
{
	NEW = 10,
	REGISTERED = 20,
	FAILED = 30,
	RECOVERED = 40,
	RETIRING = 50,
	RETIRED = 60,
	NODE_NOT_FOUND = 100
};

enum OSDMsgTypeInMap
{
	STRM = 1,
	HRBT = 2
};

class HealthMonitoringRecord
{
	public:
		HealthMonitoringRecord(
			int msg_type,
			time_t last_updated_time,
			int status,
			int timeout,
			int hrbt_failure_count,
			int hfs_stat,
			int node_state
		);
		void set_timeout(int timeout_value);
		int get_timeout();
		int get_expected_msg_type();
		void set_expected_msg_type(int);
		time_t get_last_updated_time();
		void set_last_updated_time(time_t last_updated_time);
		int get_status();
		void set_status(int status);
		int get_failure_count();
		void increment_failure_count();
		void reset_failure_count();
		int get_hrbt_failure_count() const;
		void set_hrbt_failure_count(int);
		void set_node_state(int node_state);
		void set_hfs_stat(int);
		int get_hfs_stat();
		//void set_comm_obj(boost::shared_ptr<Communication::SynchronousCommunication> sync_comm);
		//boost::shared_ptr<Communication::SynchronousCommunication> get_comm_obj();
	private:
		int msg_type;
		time_t last_updated_time;
		int status;
		int timeout;
		int failure_count;
		int hrbt_failure_count;
		int hfs_stat;
		int node_state;

};

class Monitoring
{
        public:
                virtual void monitor_for_recovery() = 0;
                virtual bool update_map(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> msg) = 0;
                virtual bool update_time_in_map(std::string node_id, int hfs_stat) = 0;
                virtual bool add_entry_in_map(boost::shared_ptr<ServiceObject> msg) = 0;
                virtual bool add_entries_while_recovery(std::string node_id, int msg_type, int status) = 0;
                virtual void get_nodes_status(
                        std::vector<std::string> &nodes_registered,
                        std::vector<std::string> &nodes_failed,
                        std::vector<std::string> &nodes_retired,
			std::vector<std::string> &nodes_recovered
                ) = 0;
		virtual void retrieve_node_status(
			boost::shared_ptr<comm_messages::NodeStatus> node_status_req,
			boost::shared_ptr<Communication::SynchronousCommunication> comm_obj
		) = 0;
                virtual bool change_status_in_monitoring_component(std::string node_id, int status) = 0;
		virtual bool change_status_in_memory(std::string node_id, int status) = 0;
                virtual void initialize_prepare_and_add_entry(boost::function<bool(std::string, int)> prepare_and_add_entry) = 0;
		virtual bool if_node_already_exists(std::string node_id) = 0;
		virtual int get_node_status(std::string node_id) = 0;
		virtual void get_node_status_while_cluster_recovery(
			std::vector<std::string> &nodes_unknown,
			std::vector<std::string> &nodes_totally_failed,
			std::vector<std::string> &nodes_in_stopping_state
		) = 0;
		virtual void reset_cluster_recovery_details() = 0;
		virtual bool remove_node_from_monitoring(std::string node_id) = 0;
		virtual std::map<std::string, network_messages::NodeStatusEnum> get_all_nodes_status() = 0;
		virtual ~Monitoring();
};


class MonitoringImpl : public Monitoring
{
	public:
		MonitoringImpl(
	//		boost::function<void(std::string, int)> prepare_and_add_entry,
//			boost::shared_ptr<system_state::StateChangeThreadManager> state_thread_mgr,
			boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer
		);
		MonitoringImpl() {}
		virtual void monitor_for_recovery();
		virtual bool update_map(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> msg);
		virtual bool update_time_in_map(std::string node_id, int hfs_stat);
		virtual bool add_entry_in_map(boost::shared_ptr<ServiceObject> msg);
		virtual bool add_entries_while_recovery(std::string node_id, int msg_type, int status);
		virtual void get_nodes_status(
			std::vector<std::string> &nodes_registered,
			std::vector<std::string> &nodes_failed,
			std::vector<std::string> &nodes_retired,
			std::vector<std::string> &nodes_recovered
		);
		virtual bool change_status_in_memory(std::string node_id, int status);
		virtual bool change_status_in_monitoring_component(std::string node_id, int status);
		virtual void initialize_prepare_and_add_entry(boost::function<bool(std::string, int)> prepare_and_add_entry);
		virtual void retrieve_node_status(
			boost::shared_ptr<comm_messages::NodeStatus> node_status_req,
			boost::shared_ptr<Communication::SynchronousCommunication> comm_obj
		);
		virtual bool if_node_already_exists(std::string node_id);
		virtual int get_node_status(std::string node_id);
		virtual void get_node_status_while_cluster_recovery(
			std::vector<std::string> &nodes_unknown,
			std::vector<std::string> &nodes_totally_failed,
			std::vector<std::string> &nodes_in_stopping_state
		);
		virtual bool remove_node_from_monitoring(std::string node_id);
		virtual void initiate_cluster_recovery();
		virtual void reset_cluster_recovery_details();
		virtual std::map<std::string, network_messages::NodeStatusEnum> get_all_nodes_status();
		virtual ~MonitoringImpl();
	private:
		bool register_status(std::string node_id, int operation, int status);
		void update_msg_type_in_map(std::string node_id, int msg_type);
		void update_status_in_map(std::string node_id, std::string status);

	private:
		int hrbt_timeout;
		int strm_timeout;
		int hrbt_failure_count;
		int strm_failure_count;
		std::string system_stop_file;
		bool system_stop;
		boost::recursive_mutex mtx;
		std::map<std::string, boost::shared_ptr<HealthMonitoringRecord> > active_node_map;
//		boost::shared_ptr<system_state::StateChangeThreadManager> state_thread_mgr;
		boost::function<bool(std::string, int)> prepare_and_add_entry;
		boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer;
		int cluster_recovery_identification_count;
		bool cluster_recovery_in_progress;
//		boost::shared_ptr<Bucket> bucket;
		std::vector<std::string> unknown_status_nodes;
		std::vector<std::string> stopping_nodes;
		std::vector<std::string> totally_failed_node_list;
};

class MonitoringThread
{
	public:
		MonitoringThread(boost::shared_ptr<Monitoring> monitoring, boost::shared_ptr<Bucket> bucket);
		void run();
		bool add_bucket(boost::shared_ptr<Bucket> bucket_ptr);
//		void update_time_in_map(std::string node_id);
		boost::shared_ptr<comm_messages::HeartBeat> receive_heartbeat(int64_t fd);
		void close_all_sockets();
	private:
		boost::shared_ptr<Monitoring> monitoring;
		cool::SharedPtr<config_file::OSMConfigFileParser> config_parser;
		boost::shared_ptr<Bucket> bucket;
		void send_heartbeat_ack(int64_t fd, uint32_t seq_no, std::string node_stat);
	
};


class MonitoringThreadManager
{
	public:
		MonitoringThreadManager(boost::shared_ptr<Monitoring> monitoring);
		void monitor_service(
			boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_obj,
			boost::shared_ptr<Communication::SynchronousCommunication> sync_comm
		);
		bool add_node_for_monitoring(boost::shared_ptr<ServiceObject> msg);
		void remove_node_from_monitoring(std::string node_id);
		void stop_all_mon_threads();
	private:
		void update_bucket(boost::shared_ptr<Communication::SynchronousCommunication> sync_comm);
		void create_bucket();
//		void success_handler(boost::shared_ptr<MessageResultWrap> result);
//		void failure_handler();
		//void create_monitoring_thread();
		//void add_fd();
		//void remove_fd();
		//void start();
		//void stop();
	private:
		boost::shared_ptr<Monitoring> monitoring;
		boost::shared_ptr<Bucket> bckt_ptr;
		//std::map<boost::shared_ptr<Bucket>, int32_t> bucket_to_fd_map;	//TODO: this map will be converted to tuple
		std::list<std::tr1::tuple<boost::shared_ptr<Bucket>, int64_t> > bucket_fd_ratio;
		std::vector<boost::shared_ptr<boost::thread> > mon_threads;
		
};

} // namespace gl_monitoring

#endif
