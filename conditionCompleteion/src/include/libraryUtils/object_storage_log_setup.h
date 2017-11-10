/*
 * $Id$
 *
 * objectStorageLogSetup.h, Initialize the logging mechanism in objectStorage.
 *
 * 
 *
 */

#ifndef _object_storage_log_setup_h
#define _object_storage_log_setup_h

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <boost/noncopyable.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scoped_ptr.hpp>
#include <cool/log.h>
#include <cool/stlExtras.h>
#include <cool/foreach.h>
#include "libraryUtils/object_storage_exception.h"
#include "config_file_parser.h"
#include <string>

std::string const objectStorageLogTag = "[Object Storage Logger] :"; 
std::string const osd_account_lib_log_tag = "[Account Library] : ";
std::string const osd_container_lib_log_tag = "[Container Library] : ";
std::string const osd_transaction_lib_log_tag = "[Transaction Library] : ";


namespace object_storage_log_setup
{

// MACROS
#define SYSLOG(msg) \
	{ \
	oss os; \
	os << msg; \
	syslog(LOG_ERR, os.str().c_str()); \
	}

#define OSDLOG(logLevel, msg)\
	{\
		LOG(logLevel, msg);\
	}\

class LogSetupTool : public boost::noncopyable
{
public:
	LogSetupTool(std::string);

	/**
	 * @brief:
	 * Method will initialize logs for objectStorage.
	 */ 
	void initializeLogSystem();

	~LogSetupTool() throw();

private:

	/**
	 * @brief:
	 * Check for log directory, if not present then create it.
	 */
	void verifyLogDirExistenceAndCreate();

	/**
	 * @brief:
	 * Function will change owner of the logFile and logDir to hydragui:hydragui
	 * Function will be executed only when user is root
	 */
	void changeOwnerForRMLLogFileAndDir() const;

	/**
	 * @brief:
	 * Function will check whether current user has write permission on the log file and log directory
	 */
	void verifyForWritePermission();

	void verifyAndTryToCorrectRMLSettings();

	cool::SharedPtr<config_file::LogConfig> get_config();
	

private:
	boost::scoped_ptr<cool::InitLog> log_system;
	bool test_mode;
	std::string object_storage_root_path;
	std::string log_dir;
	std::string log_file;
	std::string log_config_file;
	bool enable_logs;
};

} // namespace object_storage_log_setup

#endif //_OBJECTSTORAGE_OBJECTSTORAGELOGSETUP_H
