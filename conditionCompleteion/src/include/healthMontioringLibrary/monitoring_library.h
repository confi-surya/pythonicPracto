#ifndef __HEALTH_MONITORING_H__
#define __HEALTH_MONITORING_H__

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread.hpp>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include "cool/cool.h"
#include "cool/log.h"
#include "cool/aio.h"
#include "cool/option.h"
#include "mts/hydraProdMacro.h"
#include "cool/boostTime.h"
#include "cool/shellCommandsExecution.h"
#include "communication/message_interface_handlers.h"

#ifdef INCL_PROD_MSGS
#include "msgcat/HOSSyslog.h"
#endif
 
namespace healthMonitoring
{
	using namespace std;
	
	enum HEARTBEAT_RESPONSE_TYPES
	{
		HRBT_SUCCESS = 1,
		HRBT_TIMEOUT = 2,
		HRBT_FAILURE = 3
	};

	class healthMonitoringController
	{
		private:
		string self_service_id;
		string self_node_id;
                string ll_service_id;
                string self_ip;
                uint64_t self_port;
                uint64_t ll_port;
		std::string logger_string;
		bool flag;
		bool stop_service;
		bool channel_break; 
		bool is_connected;
		uint16_t strm_timeout;
		uint16_t hbrt_timeout;
		std::string system_stop_file;
		boost::thread *req_sender_thread;
		boost::condition_variable condition;
		boost::mutex mtx;
		void restart_service();
		bool create_connection(int timeout);
		string get_service_id();
		string domain_path;
		bool verify_start_monitoring_response(boost::shared_ptr<MessageResultWrap>);
		int verify_heartbeat_response(boost::shared_ptr<MessageResultWrap>);
		bool send_start_monitoring_request_local_leader(bool restart_needed);
		void send_heartbeat_local_leader(bool restart_needed);
		
		public:
		healthMonitoringController(string, uint64_t, uint64_t, string, bool restart_needed = false);
		~healthMonitoringController();
		void start_monitoring(bool);
		boost::shared_ptr<sync_com> comm_sock;
	};
}

#endif
