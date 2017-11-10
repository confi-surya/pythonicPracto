#include "osm/osmServer/hfs_checker.h"
using namespace objectStorage;
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
	return this->stat;
}

void HfsChecker::set_gfs_stat(enum OSDException::OSDHfsStatus::gfs_stat s)
{
	this->stat = s;
}

void HfsChecker::terminate()
{
	boost::mutex::scoped_lock lock(mtx);
	this->thread_stop = true;
	this->condition.notify_one();
}

void HfsChecker::run_io_service()
{
	this->io_service_obj.run();
}

void HfsChecker::scheduled_timer()
{
	this->timer->expires_from_now(boost::posix_time::seconds(this->config_parser->gns_accessibility_intervalValue()));
}

void HfsChecker::start_timer()
{
	this->scheduled_timer();
	this->run_io_service();
}

void HfsChecker::reset_timer(const boost::system::error_code& error)
{
	cool::unused(error);
	this->timer.reset();
	this->io_service_obj.stop();
}

void HfsChecker::read_update_time_from_gfs_file()
{
	GLUtils utils;
	const std::string target_path( utils.get_mount_path(this->config_parser->export_pathValue(), ".osd_meta_config"));
	if( !utils.hfs_availability_check(target_path))
	{
		if(OSDException::OSDHfsStatus::NOT_READABILITY != this->get_gfs_stat())
		{
			CLOG(SYSMSG::OSD_UNMOUNT, utils.get_my_node_id().c_str());
		}
		this->set_gfs_stat( OSDException::OSDHfsStatus::NOT_READABILITY);
		return;
	}
	std::string file_name( "osd_global_leader_status_" );
	boost::tuple<std::string, bool> file =	utils.find_file(target_path, file_name);
	if( !boost::get<1>(file) )
	{
		if(OSDException::OSDHfsStatus::NOT_READABILITY != this->get_gfs_stat())
		{
			CLOG(SYSMSG::OSD_UNMOUNT, utils.get_my_node_id().c_str());
		}
		this->set_gfs_stat( OSDException::OSDHfsStatus::NOT_READABILITY);
		return;
	}
	file_name = target_path + "/" + boost::get<0>(file);
	struct stat st;
	if(::stat(file_name.c_str(), &st))
	{
		if( errno == ENOENT)
		{
			OSDLOG(INFO, "A component of path does not exist, or path is an empty string"<<file_name);
			if(OSDException::OSDHfsStatus::NOT_READABILITY != this->get_gfs_stat())
			{
				CLOG(SYSMSG::OSD_UNMOUNT, utils.get_my_node_id().c_str());
			}
			this->set_gfs_stat( OSDException::OSDHfsStatus::NOT_READABILITY );
			return;
		}
	}
	time_t current_update_time = st.st_mtime;     	
	if( (current_update_time - this->last_update_time) > 0 )
	{
		OSDLOG(INFO, "Updating the GFS stat");
		this->last_update_time = current_update_time;
		this->set_gfs_stat( OSDException::OSDHfsStatus::WRITABILITY );
		return;
	}	
	OSDLOG(INFO, "GL is unable to touch the file "<<file_name);
	this->set_gfs_stat( OSDException::OSDHfsStatus::NOT_WRITABILITY );	
}

void HfsChecker::gfs_file_update_checker()
{
	uint64_t timeout = this->config_parser->gns_accessibility_intervalValue();
	boost::this_thread::sleep(boost::posix_time::seconds(timeout));
	this->read_update_time_from_gfs_file();
}

void  HfsChecker::stop()
{
	boost::mutex::scoped_lock lock(this->mtx);
	while(!this->thread_stop)
        {
                this->condition.wait(lock);
        }
}

void HfsChecker::start()
{
	OSDLOG(INFO, "HFS Checker thread start");
	try
	{
	for(;;)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		boost::this_thread::interruption_point();
		this->gfs_file_update_checker();
	}
	}
	catch (boost::thread_interrupted&)
	{
		OSDLOG(INFO, "Closing the hfs checker thread");
	}
}
}  //namespace hfs_checker
