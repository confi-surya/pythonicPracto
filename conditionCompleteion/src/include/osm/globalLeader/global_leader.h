#ifndef __GLOBAL_LEADER_678__
#define __GLOBAL_LEADER_678__

#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include "osm/osmServer/osm_info_file.h"
#include "osm/globalLeader/gl_msg_handler.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/globalLeader/state_change_initiator.h"
#include "osm/globalLeader/monitoring.h"

namespace global_leader
{

class GlobalLeader
{
	public:
		GlobalLeader();
		boost::shared_ptr<gl_parser::GLMsgHandler> get_gl_msg_handler();
		void start();
		void push(boost::shared_ptr<MessageCommunicationWrapper> msg);
		boost::shared_ptr<MessageCommunicationWrapper> pop();
		void abort();
		void handle_main_queue_msg();
		void handle_node_addition(
			boost::shared_ptr<comm_messages::NodeAdditionCli> msg,
			boost::shared_ptr<Communication::SynchronousCommunication>  syn_comm
		);
		void handle_node_deletion(
			boost::shared_ptr<comm_messages::NodeDeleteCli> msg,
			boost::shared_ptr<Communication::SynchronousCommunication>  syn_comm
		);
		void handle_node_rejoin_request(
			boost::shared_ptr<comm_messages::NodeRejoinAfterRecovery> rejoin_ptr,
			boost::shared_ptr<Communication::SynchronousCommunication> syn_comm
		);

		void send_node_deletion_ack(
			std::string node_id,
			boost::shared_ptr<Communication::SynchronousCommunication>  syn_comm,
			bool status
		);
		void start_gl_recovery();
	private:
		std::queue<boost::shared_ptr<MessageCommunicationWrapper> > main_queue;
		boost::condition_variable condition;
		boost::mutex mtx;
		bool stop;
		bool cluster_recovery_in_progress;
		int gfs_write_interval;
		std::string status_file;
		int gfs_write_failure_count;
		int32_t gl_port;
		std::string node_info_file_path;
		bool gfs_available;
		uint32_t node_add_response_timeout;
		void gfs_file_writer();
		bool add_node_in_info_file(
			std::list<std::pair<boost::shared_ptr<ServiceObject>, std::list<boost::shared_ptr<ServiceObject> > > > srcv_obj_list,
			int node_status);
		void recover_active_node_list();
		//std::string create_and_rename_GL_specific_file(std::string file_pattern);
		//std::pair<bool, std::string> check_file_existense_and_rename(std::string file_pattern);
		//void update_gl_info_file();
		void handle_get_version_request(std::string service_id, boost::shared_ptr<Communication::SynchronousCommunication>  syn_comm);
		void initiate_cluster_recovery();
		//boost::shared_ptr<system_state::StateChangeMapManager> state_map_mgr;
		boost::shared_ptr<system_state::StateChangeMessageHandler> state_msg_ptr;
		boost::shared_ptr<gl_monitoring::Monitoring> monitoring;
		boost::shared_ptr<gl_parser::GLMsgHandler> msg_handler;
		boost::shared_ptr<gl_monitoring::MonitoringThreadManager> mon_thread_mgr;
		boost::shared_ptr<gl_monitoring::MonitoringThread> mon_thread;
		boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer;
		boost::shared_ptr<system_state::StateChangeThreadManager> states_thread_mgr;
		boost::shared_ptr<boost::thread> state_msg_handler_thread;
		boost::shared_ptr<boost::thread> state_mgr_thread;
		//boost::shared_ptr<boost::thread> monitoring_thread;
		boost::shared_ptr<boost::thread> msg_handler_thread;
		boost::shared_ptr<boost::thread> rec_identifier_thread;
		boost::shared_ptr<boost::thread> gfs_writer_thread;

};

} // namespace global_leader

#endif

