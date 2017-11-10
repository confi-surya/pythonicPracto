#include "osm/osmServer/hfs_checker.h"

namespace hfs_checker
{

HfsChecker::HfsChecker(cool::SharedPtr<config_file::OSMConfigFileParser>config_parser)
{
	this->thread_stop = false;
	this->timer.reset(new boost::asio::deadline_timer(this->io_service_obj));
	this->config_parser = config_parser;
	this->stat = OSDException::OSDHfsStatus::WRITABILITY; 
	this->last_update_time = 0;
}

HfsChecker::~HfsChecker()
{
	this->timer.reset();
	this->io_service_obj.stop();
	this->config_parser.reset();
}

enum OSDException::OSDHfsStatus::gfs_stat HfsChecker::get_gfs_stat() const
{
	return stat;
}

void HfsChecker::set_gfs_stat(enum OSDException::OSDHfsStatus::gfs_stat s)
{
	this->stat = s;	
}

void HfsChecker::terminate()
{
}

void HfsChecker::run_io_service()
{
}

void HfsChecker::scheduled_timer()
{
}

void HfsChecker::start_timer()
{
}
void HfsChecker::reset_timer(const boost::system::error_code& error)
{
}

void HfsChecker::read_update_time_from_gfs_file()
{
	 this->set_gfs_stat( OSDException::OSDHfsStatus::NOT_WRITABILITY);
}

void HfsChecker::gfs_file_update_checker()
{
	this->read_update_time_from_gfs_file();
}

void  HfsChecker::stop()
{
	
	this->set_gfs_stat( OSDException::OSDHfsStatus::NOT_WRITABILITY);
}

void HfsChecker::start()
{
	OSDLOG(DEBUG, "HFS Checker thread start ");
	this->set_gfs_stat(OSDException::OSDHfsStatus::NOT_READABILITY);
}

}  //namespace hfs_checker
