/*****************************************************************************/
/**
  * $Id$
  *
  * object_storage_log_setup.cpp
  *
  * 
  *
  */
/************************************************************************************/

#include <pwd.h>
#include <sys/stat.h>
#include <boost/filesystem/operations.hpp>
#include <unistd.h> 
#include <cool/misc.h>
#include "libraryUtils/object_storage_log_setup.h"

using namespace object_storage_log_setup;

LogSetupTool::LogSetupTool(std::string log_name):
	enable_logs(true)
{
	cool::SharedPtr<config_file::LogConfig> config = this->get_config();
	this->log_dir = config->log_dirValue();
	this->log_config_file = config->log_config_fileValue();
	if(log_name.compare("container-library") == 0 )
		this->log_file = config->log_dirValue() + "/" + config->container_library_log_fileValue();
	else if(log_name.compare("account-library") == 0 )
	        this->log_file = config->log_dirValue() + "/" + config->account_library_log_fileValue();
	else if(log_name.compare("object-library") == 0 )
		this->log_file = config->log_dirValue() + "/" + config->object_library_log_fileValue();
	else if(log_name.compare("osm") == 0 )
		this->log_file = config->log_dirValue() + "/" + config->osm_log_fileValue();
	else if(log_name.compare("proxy-monitoring") == 0 )
		this->log_file = config->log_dirValue() + "/" + config->proxy_monitoring_log_fileValue();
	else if(log_name.compare("account-updater-monitoring") == 0 ) 
		this->log_file = config->log_dirValue() + "/" + config->account_updater_monitoring_log_fileValue();
	else if(log_name.compare("containerchildserver-library") == 0)
               this->log_file = config->log_dirValue() + "/" + config->containerchildserver_library_log_fileValue();
}

cool::SharedPtr<config_file::LogConfig> LogSetupTool::get_config()
{
	cool::config::ConfigurationBuilder<config_file::LogConfig> config_file_builder;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
        if (osd_config_dir)
                config_file_builder.readFromFile(std::string(osd_config_dir).append("/log.conf"));
        else
                config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/log.conf");

        return config_file_builder.buildConfiguration();
}

void LogSetupTool::initializeLogSystem()
{
	this->verifyAndTryToCorrectRMLSettings();
	if (this->enable_logs)
	{
		if (::setenv("MAIN_LOG_FILE", this->log_file.c_str(), 1) != 0)
		{
			SYSLOG ("Unable to execute setenv()");
		}
		this->log_system.reset(new cool::InitLog(this->log_config_file));
	}
}

void LogSetupTool::changeOwnerForRMLLogFileAndDir() const
{
	struct passwd const * const passwdRecord = ::getpwnam("hydragui");
	// Changing log directory ownership
	if (::chown(this->log_dir.c_str(), passwdRecord->pw_uid, passwdRecord->pw_gid) == -1)
	{
		SYSLOG ("Could not change owner of the Log Dir: " << this->log_dir);
	}
	// Changing log file ownership
	if (::chown(this->log_file.c_str(), passwdRecord->pw_uid, passwdRecord->pw_gid) == -1)
	{
		SYSLOG ("Could not change owner of the Log File: " << this->log_file);
	}
}

void LogSetupTool::verifyForWritePermission()
{
	if ((::access(this->log_dir.c_str(), W_OK) != 0)
						|| (::access(this->log_file.c_str(), F_OK) == 0 && ::access(this->log_file.c_str(), W_OK) != 0))
	{
		SYSLOG("Disabling remote management logs for objectStorage due to system error# " << errno);
		this->enable_logs = false;
	}
}

void LogSetupTool::verifyLogDirExistenceAndCreate()
{
	if (boost::filesystem::exists(this->log_dir))
	{
		this->verifyForWritePermission();
	}
	else
	{
		if (::mkdir(this->log_dir.c_str(),S_IRWXU | S_IRWXG) == -1)
		{
			SYSLOG ("Could not create Log Dir: " << this->log_dir << " system error #" << errno);
			this->enable_logs = false;
		}
	}
}

void LogSetupTool::verifyAndTryToCorrectRMLSettings()
{
	this->verifyLogDirExistenceAndCreate();

	if (this->enable_logs)
	{
		if (!boost::filesystem::exists(this->log_config_file))
		{
			SYSLOG ("INI file for remote management logs doesn't exists: " << this->log_config_file);
		}

		if (::getuid() == 0 && !this->test_mode)
		{
			//his->changeOwnerForRMLLogFileAndDir();
		}
	}
}

LogSetupTool::~LogSetupTool() throw()
{

}
