#ifndef __GL_MSG_HANDLER_890__
#define __GL_MSG_HANDLER_890__

#include <boost/shared_ptr.hpp>
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition_variable.hpp>

#include "osm/globalLeader/state_change_initiator.h"
#include "osm/globalLeader/monitoring.h"

namespace gl_parser
{

class GLMsgHandler
{
	public:
		GLMsgHandler(
			boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> global_leader_handler,
			boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> state_msg_handler,
			boost::function<void(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring>,
					boost::shared_ptr<Communication::SynchronousCommunication>) > monitor_service,
			boost::function<void(boost::shared_ptr<comm_messages::NodeStatus>,
					boost::shared_ptr<Communication::SynchronousCommunication>) > retrieve_node_status

		);
		void push(boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj);
		boost::shared_ptr<MessageCommunicationWrapper> pop();
		void handle_msg();
		void abort();
	private:
		//Component Message Handler
		//Monitoring Thread Manager
		bool stop;
		boost::mutex mtx;
		boost::condition_variable condition;
		std::queue<boost::shared_ptr<MessageCommunicationWrapper> > gl_msg_queue;
		boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> global_leader_handler;
		boost::function<void(boost::shared_ptr<MessageCommunicationWrapper>)> state_msg_handler;
		boost::function<void(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring>,
				boost::shared_ptr<Communication::SynchronousCommunication>) > monitor_service;
		boost::function<void(boost::shared_ptr<comm_messages::NodeStatus>,
				boost::shared_ptr<Communication::SynchronousCommunication>) > retrieve_node_status;

};

} //namespace gl_parser

#endif
