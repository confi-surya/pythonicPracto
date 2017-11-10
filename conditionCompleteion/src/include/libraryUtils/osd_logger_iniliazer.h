#ifndef __OSDLogger_H__
#define __OSDLogger_H__
#include <boost/shared_ptr.hpp>
#include "object_storage_log_setup.h"
#include <string>

namespace OSD_Logger
{

class OSDLogger
{
public:
	virtual void initialize_logger() = 0;
	virtual void logger_reset() = 0;
	virtual ~OSDLogger() {}
};

class OSDLoggerImpl: public OSDLogger
{
	std::string log_name;
public:
	OSDLoggerImpl(std::string log_name)
	{
		this->log_name = log_name;
	}
	void initialize_logger();
	void logger_reset();
};
}
#endif 
