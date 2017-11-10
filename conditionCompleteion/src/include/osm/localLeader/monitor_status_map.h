#ifndef __monitor_status_map_h__
#define __monitor_status_map_h__

#include <string>
#include <time.h>
#include <ctime>
#include "libraryUtils/object_storage_exception.h"
#include "communication/communication.h"

class MonitorStatusMap
{
	int fd;
	enum OSDException::OSDServiceStatus::service_status status;
	enum OSDException::OSDMsgTypeInMap::msg_type_in_map msg_type;
	boost::shared_ptr<Communication::SynchronousCommunication> comm_obj;
	time_t last_update_time;
	time_t strm_time;
	uint8_t retry_count;
	uint64_t strm_timeout;
	uint64_t hrbt_timeout;
	uint16_t retry_time_gap;
	uint8_t heartbeat_count;
	int32_t port;
	std::string ip_addr;
	time_t timeout;
public:
	MonitorStatusMap(uint64_t, uint64_t, uint16_t, uint16_t);	
	virtual ~MonitorStatusMap();
	void set_msg_type(enum OSDException::OSDMsgTypeInMap::msg_type_in_map msg_tpye); 
	enum OSDException::OSDMsgTypeInMap::msg_type_in_map get_msg_type(void) const;
	int get_fd(void) const;
	void set_fd(int fd); 
	boost::shared_ptr<Communication::SynchronousCommunication> get_comm_obj() const;
	void set_comm_obj(boost::shared_ptr<Communication::SynchronousCommunication> obj);
	void update_last_update_time();
	void set_heartbeat_count(int);
	void set_timeout(time_t);
	uint64_t get_hrbt_timeout() const;
	time_t get_last_update_time() const;
	uint64_t get_strm_timeout() const;	
	time_t get_timeout() const;
	void set_retry_count(int);
	uint16_t get_retry_count(void) const; 
	void set_service_status(enum OSDException::OSDServiceStatus::service_status status);
	enum OSDException::OSDServiceStatus::service_status get_service_status(void) const;
	uint16_t get_retry_time_gap() const;
	int32_t get_port() const;
	void set_port(int32_t);
	void set_ip_addr(std::string);
	std::string get_ip_addr() const;
};

#endif
