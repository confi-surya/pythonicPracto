#ifndef __service_config_reader_h__
#define __service_config_reader_h__

#include "libraryUtils/config_file_parser.h"
#include <boost/function.hpp>
#include <map>
#include <cool/misc.h>

class ServiceConfigReader
{
private:
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> config;
	cool::SharedPtr<config_file::OsdServiceConfigFileParser> get_config() const;
	std::map<std::string, boost::function<uint64_t()> > func_map;
	uint64_t get_container_strm_timeout() const;
	uint64_t get_account_strm_timeout() const;
	uint64_t get_object_strm_timeout() const;
	uint64_t get_updater_strm_timeout() const;
	uint64_t get_proxy_strm_timeout() const;
	uint64_t get_containerchild_strm_timeout() const;
public:
	ServiceConfigReader();
	virtual ~ServiceConfigReader();
	uint64_t get_strm_timeout(std::string);
	uint64_t get_heartbeat_timeout() const;
	uint16_t get_service_start_time_gap() const;
	uint16_t get_retry_count() const;
};

#endif
