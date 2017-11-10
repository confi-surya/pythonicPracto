#include "osm/osmServer/service_config_reader.h"
#include "libraryUtils/object_storage_log_setup.h"

ServiceConfigReader::ServiceConfigReader()
{
	this->config = this->get_config();
	this->func_map["container"] = boost::bind(&ServiceConfigReader::get_container_strm_timeout, this);
	this->func_map["account-updater"] = boost::bind(&ServiceConfigReader::get_updater_strm_timeout, this);
	this->func_map["object"] = boost::bind(&ServiceConfigReader::get_object_strm_timeout, this);
	this->func_map["proxy"] = boost::bind(&ServiceConfigReader::get_proxy_strm_timeout, this);
	this->func_map["account"] = boost::bind(&ServiceConfigReader::get_account_strm_timeout, this);
	this->func_map["containerChild"] = boost::bind(&ServiceConfigReader::get_containerchild_strm_timeout, this);
}

cool::SharedPtr<config_file::OsdServiceConfigFileParser> ServiceConfigReader::get_config() const
{
	cool::config::ConfigurationBuilder<config_file::OsdServiceConfigFileParser> config_file_builder;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config_dir)
		config_file_builder.readFromFile(std::string(osd_config_dir).append("/OsdServiceConfigFile.conf"));
	else
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/OsdServiceConfigFile.conf");
	return config_file_builder.buildConfiguration();
}

ServiceConfigReader::~ServiceConfigReader()
{
	this->config.reset();
}
   
uint64_t ServiceConfigReader::get_account_strm_timeout() const
{
	return this->config->account_strm_timeoutValue();
}

uint64_t ServiceConfigReader::get_containerchild_strm_timeout() const
{
	return this->config->containerChild_strm_timeoutValue();
}

uint64_t ServiceConfigReader::get_proxy_strm_timeout() const
{
	return this->config->proxy_strm_timeoutValue();
}

uint64_t ServiceConfigReader::get_object_strm_timeout() const
{
	return this->config->object_strm_timeoutValue();
}

uint64_t ServiceConfigReader::get_updater_strm_timeout() const
{
	return this->config->accountUpdater_strm_timeoutValue();
}

uint64_t ServiceConfigReader::get_container_strm_timeout() const
{
	return this->config->container_strm_timeoutValue();
}

uint64_t ServiceConfigReader::get_strm_timeout(std::string service_name) 
{
	std::map<std::string, boost::function<uint64_t()> >::iterator it;
        it = this->func_map.find(service_name);
	if( it != this->func_map.end())
	{
		return this->func_map[service_name]();
	}
	OSDLOG(DEBUG, "Invalid service name "<< service_name);
	return -1;
}
	
uint64_t ServiceConfigReader::get_heartbeat_timeout() const
{
	return this->config->heartbeat_timeoutValue();
}

uint16_t ServiceConfigReader::get_service_start_time_gap() const
{
	return this->config->service_start_time_gapValue();
}

uint16_t ServiceConfigReader::get_retry_count() const
{
	return this->config->retry_countValue();
}
