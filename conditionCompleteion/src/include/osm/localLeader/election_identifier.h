#ifndef __election_identifier_h__
#define __election_identifier_h__

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <vector>
#include <boost/thread/mutex.hpp>
#include "libraryUtils/object_storage_exception.h"
#include "libraryUtils/config_file_parser.h"

#include "osm/localLeader/election_manager.h"
#include "osm/localLeader/monitoring_manager.h"

#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/osmServer/utils.h"

using namespace hfs_checker;

class ElectionIdentifier
{
	private:
	boost::shared_ptr<ElectionManager> elec_mgr;
	boost::shared_ptr<HfsChecker> hfs_checker;
	boost::shared_ptr<MonitoringManager> monitor_mgr;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	enum OSDException::OSDConnStatus::connectivity_stat conn_stat;
	uint16_t count;
	uint16_t heartbeat_failure_count;
	uint16_t strm_failure_count;
	bool stop;
	bool successer;
	bool failure;
	int send_heartbeat_to_gl(boost::shared_ptr<Communication::SynchronousCommunication> , uint32_t seq_no);
	int notify_to_election_manager_for_start_election(enum OSDException::OSDConnStatus::connectivity_stat);
	boost::tuple<boost::shared_ptr<Communication::SynchronousCommunication> , int> send_strm_to_gl();	
	enum OSDException::OSDHfsStatus::gfs_stat verify_gfs_stat();
	int stat_handler(enum OSDException::OSDHfsStatus::gfs_stat stat);
	void success_handler(boost::shared_ptr<MessageResultWrap> result);
	void failure_handler();
	bool stop_all_service();
	boost::mutex &mtx;
	enum network_messages::NodeStatusEnum &stat;
	public:
  	ElectionIdentifier(boost::shared_ptr<ElectionManager>, boost::shared_ptr<HfsChecker>, boost::shared_ptr<MonitoringManager>,
		cool::SharedPtr<config_file::OSMConfigFileParser>, enum network_messages::NodeStatusEnum &stat, boost::mutex &mtx);	
	virtual ~ElectionIdentifier();
	void start();
	void stop_election_identifier();
	enum network_messages::NodeStatusEnum local_stat;
};
#endif
