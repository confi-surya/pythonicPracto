#include "osm/localLeader/monitor_status_map.h"


MonitorStatusMap::MonitorStatusMap(uint64_t STRM_TIMEOUT, uint64_t HRBT_TIMEOUT, uint16_t gap, uint16_t count)
{
	this->fd = -1;
	this->port = -1;
	this->ip_addr = "";
	this->msg_type = OSDException::OSDMsgTypeInMap::STRM;
	this->heartbeat_count = 1;
	this->retry_count  = count;
	this->strm_timeout = STRM_TIMEOUT;
        this->status = OSDException::OSDServiceStatus::REG;
	this->update_last_update_time();
	this->hrbt_timeout = HRBT_TIMEOUT;
	this->set_timeout(this->strm_timeout);
	this->retry_time_gap = gap;
}

MonitorStatusMap::~MonitorStatusMap()
{
	if( this->comm_obj)
	{
		this->comm_obj.reset();
	}	
}

boost::shared_ptr<Communication::SynchronousCommunication> MonitorStatusMap::get_comm_obj() const
{
	return this->comm_obj;
}

void  MonitorStatusMap::set_comm_obj(boost::shared_ptr<Communication::SynchronousCommunication> obj )
{
	this->comm_obj = obj;
}

void MonitorStatusMap::set_fd(int fd)
{
	this->fd = fd;
}

int MonitorStatusMap::get_fd(void) const
{
	return this->fd;
}

void MonitorStatusMap::set_ip_addr(std::string addr)
{
	this->ip_addr = addr;
}

std::string MonitorStatusMap::get_ip_addr() const
{
	return this->ip_addr;
}

void MonitorStatusMap::set_port(int32_t port)
{
	this->port = port;
}

int32_t MonitorStatusMap::get_port() const
{
	return this->port;
}

void MonitorStatusMap::set_msg_type(enum OSDException::OSDMsgTypeInMap::msg_type_in_map msg)
{
	this->msg_type = msg;
}

enum OSDException::OSDMsgTypeInMap::msg_type_in_map MonitorStatusMap::get_msg_type(void) const
{
	return this->msg_type;
}
void MonitorStatusMap::set_retry_count(int count)
{
	this->retry_count = count;
}	 

uint16_t MonitorStatusMap::get_retry_count(void) const
{
	return this->retry_count;
}

uint64_t MonitorStatusMap::get_strm_timeout() const
{
	return this->strm_timeout;
}

uint64_t MonitorStatusMap::get_hrbt_timeout() const
{
	return this->hrbt_timeout;
}

void MonitorStatusMap::update_last_update_time()
{
	time_t now;
	time(&now);
	this->last_update_time = now;
}

time_t  MonitorStatusMap::get_last_update_time() const
{
	return this->last_update_time;
}

enum OSDException::OSDServiceStatus::service_status MonitorStatusMap::get_service_status(void) const
{
	return this->status;
}

void MonitorStatusMap::set_service_status(enum OSDException::OSDServiceStatus::service_status status)
{
	this->status = status;
}

void MonitorStatusMap::set_heartbeat_count(int count)
{
	this->heartbeat_count = count;
}	

void MonitorStatusMap::set_timeout(time_t timeout)
{
	this->timeout = timeout;  
}

time_t MonitorStatusMap::get_timeout(void) const
{
	return this->heartbeat_count * this->timeout;
}

uint16_t MonitorStatusMap::get_retry_time_gap() const
{
	return this->retry_time_gap;
}
