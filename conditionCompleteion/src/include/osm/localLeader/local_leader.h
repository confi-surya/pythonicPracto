#ifndef __local_leader_h__
#define __local_leader_h__

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <list>

#include "libraryUtils/object_storage_exception.h"

#ifdef INCL_PROD_MSGS
#include "msgcat/HOSSyslog.h"
#endif

#include "osm/localLeader/monitoring_manager.h"
#include "osm/localLeader/service_startup_manager.h"
#include "osm/localLeader/ll_msg_handler.h"
#include "osm/localLeader/election_manager.h"
#include "osm/localLeader/election_identifier.h"
#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/osmServer/utils.h"

#include "communication/stubs.h"
#include "communication/message.h"
#include "communication/communication.h"
#include "communication/message_binding.pb.h"

using namespace hfs_checker;

namespace local_leader
{

class LocalLeader
{
private:
	boost::shared_ptr<HfsChecker> hfs_checker;
	boost::shared_ptr<ll_parser::LlMsgHandler> msg_handler;
	boost::shared_ptr<MonitoringManager> monitor_mgr;
	boost::shared_ptr<ElectionManager> election_mgr;
	boost::shared_ptr<common_parser::LLMsgProvider> ll_msg_provider_obj;
	boost::shared_ptr<common_parser::LLNodeAddMsgProvider> na_msg_provider;
	boost::shared_ptr<common_parser::MessageParser> parser_obj;
	boost::shared_ptr<ElectionIdentifier> election_idf;
	//boost::scoped_ptr<Communicator> comm;
	boost::shared_ptr<boost::thread> service_startup_thread;
	boost::shared_ptr<boost::thread> election_idf_thread;
	boost::shared_ptr<boost::thread> monitoring_thread;
	boost::shared_ptr<boost::thread> hfs_checker_thread;
	boost::shared_ptr<boost::thread> msg_handler_thread;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	boost::condition_variable node_addition_msg_recv; 
	boost::asio::io_service io;
	bool if_cluster_part;	
	//enum OSDNodeStat::NodeStatStatus::node_stat node_stat;
	enum network_messages::NodeStatusEnum node_stat;
	bool stop;
	bool if_part_of_cluster();
	int32_t osm_port;
	bool notify_gl(boost::shared_ptr<MessageCommunicationWrapper> msg_obj, bool );
	void terminate_all_active_thread();
	void stop_operation(const boost::system::error_code &e);
	void start_timer();
	void success_handler(boost::shared_ptr<MessageResultWrap> result);
	void failure_handler();
	bool failure;                // check failure for NodeRejoinRecovery

	boost::mutex mtx;
public:
	LocalLeader();	
	LocalLeader(	//enum OSDNodeStat::NodeStatStatus::node_stat,
		enum network_messages::NodeStatusEnum,
		boost::shared_ptr<common_parser::LLMsgProvider>,
		boost::shared_ptr<common_parser::LLNodeAddMsgProvider>,
		boost::shared_ptr<common_parser::MessageParser>, 
		cool::SharedPtr<config_file::OSMConfigFileParser>config_parser,
		int32_t tcp_port
	);
	virtual ~LocalLeader();
	void start();	
	boost::shared_ptr<ll_parser::LlMsgHandler> get_ll_msg_handler() const;
};

}  //namespace local_leader

#endif
