#ifndef __STATE_CHANGE_MGR_789__
#define __STATE_CHANGE_MGR_789__

#include <string>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include <tr1/tuple>

#ifdef INCL_PROD_MSGS
#include "msgcat/HOSSyslog.h"
#endif

#include "communication/msg_external_interface.h"
#include "osm/osmServer/utils.h"
#include "osm/globalLeader/journal_library.h"
#include "libraryUtils/record_structure.h"
#include "osm/globalLeader/component_balancing.h"

#define CONTAINER_SUFFIX "_container-server"
#define ACCOUNT_SUFFIX "_account-server"
#define UPDATER_SUFFIX "_updater-server"
#define OBJECT_SUFFIX "_object-server"

//using namespace std::tr1;

namespace component_manager
{
	class ComponentMapManager;
}

namespace system_state
{

enum StateChangeStatus
{
	NA_NEW = 1,
	NA_COMPLETED = 2,
	BALANCING_NEW = 3,
	BALANCING_PLANNED = 4,
	BALANCING_IN_PROGRESS = 5,
	BALANCING_COMPLETED = 6,
	BALANCING_ABORTED = 7,
	BALANCING_FAILED = 8,
	BALANCING_RESCHEDULE = 9,
	RECOVERY_NEW = 10,
	RECOVERY_PLANNED = 11,
	RECOVERY_IN_PROGRESS = 12,
	RECOVERY_COMPLETED = 13,
	RECOVERY_CLOSED = 14,
	RECOVERY_FAILED = 15,	//When recovery process, na, nd request fails
	RECOVERY_ABORTED = 16,	//When one of the destinations die
	RECOVERY_INTERRUPTED = 17,
	ND_NEW = 18,
	ND_COMPLETED = 19,
	ND_PLANNED = 20,
	ND_FAILED = 21,
	ND_ABORTED = 22,
	ND_IN_PROGRESS = 23

};

/*enum MsgTypes
{
	HEART_BEAT = 1,
	COMP_TRNSFR_INFO = 2,
	COMP_TRNFR_FINAL_STAT = 3,
	GET_GLOBAL_MAP = 4,
	GET_SERVICE_COMPONENT = 5,
	RCV_PRO_STRT_MONITORING = 6,
	NODE_FAILURE_NOTIFICATION = 7
};*/

enum StateChangeOperation
{
	NODE_ADDITION = 1,
	NODE_DELETION = 2,
	NODE_RECOVERY = 3,
	COMPONENT_BALANCING = 4
};

class StateChangeInitiatorMap
{
	public:
		StateChangeInitiatorMap(		//For Recovery Process
			boost::shared_ptr<recordStructure::RecoveryRecord> record,
			int64_t fd,
			time_t last_updated_time,
			int timeout,
			bool balancing_planned,
			int hrbt_failure_count
		);
		StateChangeInitiatorMap(		//For Component Balancing
			boost::shared_ptr<recordStructure::RecoveryRecord> record,
			int64_t fd,
			bool balancing_planned
		);
		void set_timeout(int timeout_value);
		int get_timeout();
		int get_expected_msg_type();
		void set_expected_msg_type(int);
		time_t get_last_updated_time();
		void set_last_updated_time(time_t last_updated_time);
		boost::shared_ptr<recordStructure::RecoveryRecord> get_recovery_record();
		int64_t get_fd();
		void set_fd(int64_t fd);
		bool if_planned();
		void set_planned();
		void reset_planned();
	private:
		boost::shared_ptr<recordStructure::RecoveryRecord> record;
		int expected_msg_type;
		int64_t fd;
		time_t last_updated_time;
		int timeout;
		bool balancing_planned;		//Used to identify if BALANCING_PLANNED is encountered first time, if not, then we dont need to create new thread for component transfer
		int hrbt_failure_count;
};

/*class ResourceManager
{
	public:
		boost::shared_ptr<StateChangeInitiatorMap> get_sci_map_obj(std::string);
		void update_status(std::string);
		int32_t get_fd(std::string);
		set_status();
//		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> >::iterator find_in_sci_map(std::string);
	private:
		boost::mutex mtx;
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> > state_map;
};*/

class StateChangeThreadManager
{
	public:
		//StateChangeThreadManager(boost::shared_ptr<system_state::StateChangeMapManager> state_map_mgr);
		StateChangeThreadManager(
			cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr,
			boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
			boost::function<bool(std::string, int)> change_status_in_monitoring_component
		);
		StateChangeThreadManager() {}
		bool update_state_table(boost::shared_ptr<MessageCommunicationWrapper> msg);
		bool prepare_and_add_entry(std::string node_id, int operation);
		
		bool refresh_states_for_cluster_recovery();	//Used only in Cluster Recovery
		void prepare_and_add_entries(std::vector<std::pair<std::string,int> > node_list);	//Used only in Cluster Recovery

		void monitor_table();
		std::vector<std::string> get_state_change_ids();
		void send_node_failover_msg_ack(
			bool success,
			boost::shared_ptr<Communication::SynchronousCommunication> comm_obj
		);
		void start_recovery(
			std::vector<std::string> &nodes_registered,
			std::vector<std::string> &nodes_failed,
			std::vector<std::string> &nodes_retired,
			std::vector<std::string> &nodes_recovered,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
		);
		bool check_if_recovery_completed_for_this_node(std::string service_id, int operation);
		void add_service_entry_for_balancing(std::string service_id, int operation);

		void send_node_retire_msg_ack(
			bool success,
			boost::shared_ptr<Communication::SynchronousCommunication> comm_obj,
			std::string node_id
		);
		void send_node_stop_msg_ack(
			std::string node_id,
			boost::shared_ptr<Communication::SynchronousCommunication> comm_obj,
			int node_status,
			bool status
		);
		void stop_all_state_mon_threads(); // explicitly added to stop all state mon threads in abort() 
	private:
		//void start_node_recovery_process();
		bool update_map(boost::shared_ptr<MessageCommunicationWrapper> msg);
		void add_entry_for_recovery(
			std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records,
			int operation
		);
		bool check_and_add_entry_in_map(std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records, int operation);
		bool add_entry_in_map(std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records);
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> > state_map;
		int hrbt_timeout;
		int strm_timeout;
		int hrbt_failure_count;
		std::string node_info_file_path;
		bool start_node_recovery_process(
			std::string failed_node_id,
			std::string rec_process_destination
		);

		std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > get_map_from_tuple(
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_plan
		);

		void initiate_component_balancing(
			std::vector<std::string> &nodes_registered,
			std::vector<std::string> &nodes_failed,
			std::vector<std::string> &nodes_retired
		);
		
		void initiate_component_balancing_for_service(
			std::string service,
			std::vector<std::string> &nodes_registered,
			std::vector<std::string> &nodes_failed,
			std::vector<std::string> &nodes_retired
		);

		void modify_sci_map_for_new_plan(
			std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map,
			std::vector<std::string> na_list,
			std::vector<std::string> nd_list,
			std::vector<std::string> rec_list
		);
		void add_entry_in_map_while_recovery(
			std::vector<std::string> &nodes_failed,
			std::vector<std::string> &nodes_retired
		);
		bool unmount_journal_filesystems(std::string service_id);
		bool check_if_node_can_be_retired(std::vector<boost::shared_ptr<recordStructure::RecoveryRecord> > node_rec_records);

	private:
		boost::condition_variable condition;
		boost::mutex mtx;
		boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr;
		boost::shared_ptr<component_balancer::ComponentBalancer> component_balancer_ptr;
		bool work_done;
		std::map<std::string, int> service_entry_to_be_deleted;
		std::string journal_path;
		std::string cont_rec_script_location;
		std::string acc_rec_script_location;
		std::string updater_rec_script_location;
		std::string object_rec_script_location;
		std::string script_executor;
		std::string umount_cmd;
		int fs_mount_timeout;
		boost::function<void(std::string, int)> change_status_in_monitoring_component;
		boost::shared_ptr<global_leader::JournalLibrary> journal_lib_ptr;
		std::vector<std::tr1::tuple<boost::shared_ptr<Bucket>, int64_t> > bucket_fd_ratio;
		std::vector<boost::shared_ptr<boost::thread> > state_map_monitor;
		void create_bucket();
		void update_bucket(boost::shared_ptr<Communication::SynchronousCommunication> comm_obj);
		void notify();
		void wait_for_journal_write();
		void start_component_transfer(std::string source_service_id);
		void block_service_request(std::string service_id);
		void clean_recovery_process(
			std::string rec_process_src,
			std::string rec_process_destination
		);
		void get_info_about_unhealthy_nodes(
		        std::vector<std::string> &unhealthy_nodes,
		        std::vector<std::string> &nodes_as_rec_destination
		);
		bool send_ack_for_rec_strm(
			std::string service_id,
			boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr,
			bool status
		);
};

class StateChangeMonitorThread
{
	public:
		StateChangeMonitorThread(
			boost::shared_ptr<Bucket> bucket_ptr,
			std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> > &state_map,
			boost::shared_ptr<global_leader::JournalLibrary> journal_lib_ptr,
			boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
			boost::shared_ptr<component_balancer::ComponentBalancer> component_balancer_ptr,
			boost::mutex &mtx
		);
		void run();
	private:
		boost::shared_ptr<Bucket> bucket_ptr;
		boost::condition_variable condition;
		std::map<std::string, boost::shared_ptr<StateChangeInitiatorMap> > &state_map;
		boost::shared_ptr<global_leader::JournalLibrary> journal_lib_ptr;
		boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr;
		boost::shared_ptr<component_balancer::ComponentBalancer> component_balancer_ptr;
		bool work_done;
		boost::mutex &mtx;
		boost::shared_ptr<MessageResultWrap> receive_data(int64_t fd);
		void update_time_in_map(std::string service_id);
		void update_status(
				std::string service_id,
			        std::vector<boost::shared_ptr<recordStructure::Record> > &journal_records,
			        int status
				);
		void remove_entry_from_map(std::string service_id);
		void notify();
		void wait_for_journal_write();
		bool update_planned_components(
			std::string service_id,
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > intermediate_state,
			std::vector<boost::shared_ptr<recordStructure::Record> > &node_rec_records
		);
		void handle_end_monitoring(
		        std::string service_id,
			std::vector<std::pair<int32_t, bool> > final_status,
			std::vector<boost::shared_ptr<recordStructure::Record> > &node_rec_records
		);
		void move_planned_to_transient(std::string service_id);		//Only for testing purpose

};

class StateChangeMessageHandler
{
	public:
		StateChangeMessageHandler(
			boost::shared_ptr<StateChangeThreadManager> state_thread_mgr,
			boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
			boost::shared_ptr<gl_monitoring::Monitoring> monitoring
			);
		void push(boost::shared_ptr<MessageCommunicationWrapper> msg);
		boost::shared_ptr<MessageCommunicationWrapper> pop();
		void handle_msg();
		void abort();
	private:
		boost::shared_ptr<StateChangeThreadManager> state_thread_mgr;
		boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr;
		boost::shared_ptr<gl_monitoring::Monitoring> monitoring;
		std::queue<boost::shared_ptr<MessageCommunicationWrapper> > state_queue;
		boost::condition_variable condition;
		boost::mutex mtx;
		bool stop;
};


} // namespace system_state

#endif
