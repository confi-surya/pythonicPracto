#ifndef __OSM_DAEMON_074__
#define __OSM_DAEMON_074__

#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <queue>

#include "osm/osmServer/msg_handler.h"
#include "osm/osmServer/listener.h"
#include "osm/globalLeader/global_leader.h"
#include "osm/localLeader/local_leader.h"

enum gl_running_status
{
	UNABLE_TO_UPDATE_INFO = 1,
	UNABLE_TO_MOUNT = 2,
	SUCCESS = 3
};

int instantiate_GL(            
	boost::shared_ptr<global_leader::GlobalLeader> &gl_obj,
	boost::shared_ptr<gl_parser::GLMsgHandler> &gl_msg_ptr,
	boost::shared_ptr<boost::thread> &gl_thread,
	boost::shared_ptr<common_parser::MessageParser> &parser_obj,
	bool if_cluster,
	bool env_check,
	int32_t osm_port,
	bool create_flag,
	bool if_by_election,
	boost::shared_ptr<common_parser::GLMsgProvider>  &gl_msg_provider_obj
);

bool create_and_mount_gl_journal();

class MainThreadHandler
{
	public:
		MainThreadHandler(
			boost::shared_ptr<global_leader::GlobalLeader> &gl_obj,
			boost::shared_ptr<gl_parser::GLMsgHandler>& gl_msg_ptr,
			boost::shared_ptr<boost::thread>& gl_thread,
			boost::shared_ptr<common_parser::MessageParser> &parser_obj,
			boost::shared_ptr<local_leader::LocalLeader>& ll_ptr,
			boost::shared_ptr<Listener>& listener_ptr,
			boost::shared_ptr<boost::thread> &listener_thread,
			cool::SharedPtr<config_file::OSMConfigFileParser> config_parser,			
			int32_t osm_port,
			bool &if_i_am_gl,
			boost::shared_ptr<common_parser::GLMsgProvider> gl_msg_provider_obj
		);
		void push(boost::shared_ptr<MessageCommunicationWrapper> msg);
		boost::shared_ptr<MessageCommunicationWrapper> pop();
		void handle_msg();
	private:
		std::queue<boost::shared_ptr<MessageCommunicationWrapper> > main_thread_queue;
		boost::condition_variable main_thread_cond;
		boost::mutex mtx;
		boost::shared_ptr<global_leader::GlobalLeader> gl_obj;
		boost::shared_ptr<gl_parser::GLMsgHandler> gl_msg_ptr;
		boost::shared_ptr<boost::thread> gl_thread;
		boost::shared_ptr<common_parser::MessageParser> parser_obj;
		boost::shared_ptr<local_leader::LocalLeader> ll_ptr;
		boost::shared_ptr<Listener> listener_ptr;
		boost::shared_ptr<boost::thread> listener_thread;
		cool::SharedPtr<config_file::OSMConfigFileParser> config_parser;
		int32_t osm_port;
		bool if_i_am_gl;
		boost::shared_ptr<common_parser::GLMsgProvider> gl_msg_provider_obj;
		int mount_retry_count;
		bool mount_gl_journal_fs(std::string old_gl_id, std::string new_gl_id);
		bool unmount_gl_journal_fs(std::string old_gl_id);
		void send_ownership_notification(
			std::string new_gl_id,
			boost::shared_ptr<Communication::SynchronousCommunication> comm,
			int32_t status
		);
		std::pair<bool, std::string> verify_election_pre_conditions();
		bool update_gl_info_file(std::string gl_info_file_version, int32_t gl_port);
};

#endif
