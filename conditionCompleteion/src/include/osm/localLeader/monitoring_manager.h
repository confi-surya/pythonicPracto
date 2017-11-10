#ifndef __monitoring_h__
#define __monitoring_h__

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/thread/condition_variable.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <list>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <map>
#include <set>
#include <errno.h>

#ifdef INCL_PROD_MSGS
#include "msgcat/HOSSyslog.h"
#endif

#include "libraryUtils/object_storage_exception.h"

#include "osm/localLeader/monitor_status_map.h"
#include "osm/localLeader/recovery_handler.h"
//#include "osm/communicator.h"
#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/utils.h"

#include "communication/stubs.h"
#include "communication/message_interface_handlers.h"

#define HEARTBEAT_COUNT 3
#define number_of_service 5
#define MAX_RETRY_COUNT 3

using namespace hfs_checker;

class MonitoringManager
{
private:
	//boost::scoped_ptr<Communicator> comm; 
	std::map < std::string, MonitorStatusMap > monitor_status_map;
	boost::shared_ptr<HfsChecker> hfs_checker;
	fd_set m_fd_set;
	bool all_service_started;
	int m_max;
	std::set<int64_t> m_fds;
	boost::shared_ptr<GLUtils>utils;
	boost::shared_ptr<boost::thread> recovery_identifier_thread;
	boost::shared_ptr<boost::thread> recovery_process_thread;
	boost::condition_variable condition;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	boost::mutex mtx, mtx1;
	bool stop;
	bool notify_to_gl_result;
	bool recovery_disable;
	int mTermFlag;
	int mThreadStatus;
	void insert(int fd);
	void erase(int fd);
	int get_max() const { return m_max; }
	const fd_set &get_fd_set() const { return m_fd_set; }
	const std::set<int64_t> &getFds() const { return m_fds; }		
	void service_monitoring(void);
	void start_service_recovery(std::string);
	void send_ack_to_service(std::string, boost::shared_ptr<Communication::SynchronousCommunication> comm , bool);
	bool recv_heartbeat(boost::shared_ptr<Communication::SynchronousCommunication> comm, uint64_t & comm_code);
	bool is_hfs_avaialble();
	std::string get_service_name(std::string);
	void update_table_in_case_stop_service();
	void wait(uint16_t timeout);
	bool verify_recovery_disable();
	void enable_recovery();
	void failure_handler();
	void success_handler(boost::shared_ptr<MessageResultWrap> result);
	void kill_all_services();

	//map<string, boost::tuple<boost::thread::native_handle_type, bool> > recovery_thread_map;

public:
	void change_recovery_status(bool);
	MonitoringManager(boost::shared_ptr<HfsChecker> hfs_checker, cool::SharedPtr<config_file::OSMConfigFileParser>config_parser);
	virtual ~MonitoringManager();
	void start_monitoring(boost::shared_ptr<Communication::SynchronousCommunication> comm, int, std::string, int32_t port, std::string ip);
	void end_monitoring(boost::shared_ptr<Communication::SynchronousCommunication> comm, std::string);
	void end_all();	
	bool notify_to_gl();
	void clean_up(std::string service_id);
	bool register_service(std::string service_id, uint64_t, uint64_t, uint16_t, uint16_t);
	bool un_register_service(int, std::string service_id);
	bool is_register_service(std::string service_id);
	void stop_recovery_identifier();
	void start();
	//boost::tuple<std::list< ServiceObject >, bool> get_service_port_and_ip();
	boost::tuple<std::list<boost::shared_ptr<ServiceObject > >, bool> get_service_port_and_ip();
	//bool node_stop_operation(int fd);
	bool system_stop_operation();
	void stop_mon_thread();
	void enable_mon_thread();
	bool stop_all_service();
	void start_recovery_identifier();
	bool start_all_service();
	void join();
	void send_http_requeset_for_service_stop(int32_t, std::string, std::string service_name);
	bool unmount_file_system();
	bool mount_file_system();
	bool if_notify_to_gl_success() const;
};
#endif
