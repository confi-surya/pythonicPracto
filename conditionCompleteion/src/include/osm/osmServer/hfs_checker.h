#ifndef __hfs_checker_h__
#define __hfs_checker_h__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/condition_variable.hpp>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "libraryUtils/object_storage_exception.h"

#include "osm/osmServer/utils.h"

#ifdef INCL_PROD_MSGS
#include "msgcat/HOSSyslog.h"
#endif

namespace hfs_checker
{

class HfsChecker
{
private:
	enum OSDException::OSDHfsStatus::gfs_stat stat;	
	bool thread_stop;
	boost::condition_variable condition;
	boost::shared_ptr<boost::asio::deadline_timer> timer;
	boost::asio::io_service io_service_obj;
	boost::mutex mtx;
	boost::shared_ptr<GLUtils>utils;
	cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
	time_t last_update_time;
	void set_gfs_stat(enum OSDException::OSDHfsStatus::gfs_stat);
	void terminate();
	void scheduled_timer();
	void start_timer();
	void reset_timer(const boost::system::error_code& error);
	void read_update_time_from_gfs_file();
	void gfs_file_update_checker();
	void run_io_service();
public:
	HfsChecker(cool::SharedPtr<config_file::OSMConfigFileParser>);
	virtual ~HfsChecker();
	enum OSDException::OSDHfsStatus::gfs_stat get_gfs_stat() const;
	void start();
	void stop();
};

} //namespace hfs_checker
#endif

