#ifndef __msg_handler_h__
#define __msg_handler_h__

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>
#include <queue>
#include <unistd.h>

#include "osm/localLeader/monitoring_manager.h"
#include "osm/localLeader/election_manager.h"
#include "osm/osmServer/ring_file.h"

#include "communication/msg_external_interface.h"
#include "communication/stubs.h"
#include "communication/message_binding.pb.h"

namespace ll_parser
{

class LlMsgHandler
{
private:
	boost::shared_ptr<MonitoringManager> monitor_mgr;	
	boost::shared_ptr<ElectionManager> elec_mgr;
	bool node_addition_msg;
	bool ring_file_error;
	boost::shared_ptr<MessageCommunicationWrapper> pop();
	std::queue<boost::shared_ptr<MessageCommunicationWrapper> > ll_msg_queue;
        boost::condition_variable condition;
	boost::condition_variable node_condition;
        boost::mutex mtx, node_mtx;
	boost::asio::io_service io_service;
	boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	bool stop;
	bool is_osd_services_stop;
	enum network_messages::NodeStatusEnum node_status;
	void success_handler(boost::shared_ptr<MessageResultWrap> result);
	void node_addition_msg_rcv(boost::shared_ptr<MessageCommunicationWrapper>);
	void failure_handler();
	void start_io_service();
	network_messages::NodeStatusEnum get_local_leader_status() const;
public:
	LlMsgHandler(boost::shared_ptr<MonitoringManager> mtr_mgr, boost::shared_ptr<ElectionManager> mgr,
			cool::SharedPtr<config_file::OSMConfigFileParser>config_parser );	
	virtual ~LlMsgHandler();
	void verify_node_addition_msg_recv();	
	void node_addition_completed();
	boost::shared_ptr<MessageCommunicationWrapper> get_gl_channel() const;
	void update_local_leader_status(enum network_messages::NodeStatusEnum status);
	void push(boost::shared_ptr<MessageCommunicationWrapper> msg);
	void start();
	bool get_ring_file_error() const;
	bool non_blocking_node_add_verify();  // new method added for node rejoin recovery so that it can not wait infinitely
};
} //namespace ll_parser
#endif
