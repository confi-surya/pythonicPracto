#include <iostream>
#include <stdexcept>

#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"
#include "healthMontioringLibrary/monitoring_library.h"
#include "libraryUtils/config_file_parser.h"
#include "communication/message_type_enum.pb.h"

#define IP_LOCAL_LEADER "127.0.0.1"

cool::SharedPtr<config_file::OSMConfigFileParser> get_config_parser()
{
	cool::config::ConfigurationBuilder<config_file::OSMConfigFileParser> config_file_builder;
	char const * const osd_config_dir = ::getenv("OBJECT_STORAGE_CONFIG");
	if (osd_config_dir)
	{
		config_file_builder.readFromFile(std::string(osd_config_dir).append("/osm.conf"));
	}
	else
	{
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/osm.conf");
	}
	return config_file_builder.buildConfiguration();
}
using namespace objectStorage;
namespace healthMonitoring
{
	healthMonitoringController::healthMonitoringController(string ip, uint64_t port, uint64_t ll_port, string running_service_id, bool restart_needed)
	{
		boost::mutex::scoped_lock lock(this->mtx);
		OSDLOG(DEBUG, "Construction for health monitoring started!");
		this->domain_path = get_config_parser()->unix_domain_portValue();
		this->stop_service = get_config_parser()->stop_serviceValue();
		this->strm_timeout = get_config_parser()->strm_ack_timeout_hmlValue();
		this->hbrt_timeout = get_config_parser()->hbrt_ack_timeout_hmlValue();
		this->system_stop_file = get_config_parser()->system_stop_fileValue();
		this->self_service_id = running_service_id;
		this->self_node_id = running_service_id.substr(0, running_service_id.find_first_of("_"));
		this->ll_port = ll_port;
		this->self_ip = ip;
		this->self_port = port;
		this->flag = true;
		this->channel_break = false;
		this->is_connected = false;
		this->logger_string = "[" + this->self_service_id + "]: ";
		this->req_sender_thread = new boost::thread(&healthMonitoringController::start_monitoring, this, restart_needed);
		while (!this->is_connected)
		{	
			this->condition.wait(lock);
		}
		INIT_CATALOG("/opt/ESCORE/etc/nls/msg/C/%N.cat");
		LOAD_CATALOG("healthMonitoring", "HOS", "HOS", "HOS");
		OSDLOG(INFO, this->logger_string << "Construction for health monitoring completed!");
	}

	healthMonitoringController::~healthMonitoringController(){
		OSDLOG(INFO, this->logger_string << "Destruction for health monitoring started!");
		this->comm_sock.reset();
		this->flag = false;
		this->req_sender_thread->join();
		delete this->req_sender_thread;
		SHUTDOWN_CATALOG();
		OSDLOG(DEBUG, this->logger_string << "Destruction for health monitoring completed!");
	}

	/*
	Function Name : restart_service
	Purpose : Restart local leader on connection failure
        */
	void healthMonitoringController::restart_service(){
		try{
			int counter = 0;
			while (counter < 2)
			{
				if (boost::filesystem::exists(this->system_stop_file))
				{
					OSDLOG(INFO, this->logger_string << "System Stop in progress, Ignoring Restart of LL");
					counter = 2;
					break;
				}
				std::string service_name = "OSDMonitoring";
				CLOG(SYSMSG::OSD_SERVICE_RESTART, this->self_node_id.c_str(), service_name.c_str());
				OSDLOG(DEBUG, this->logger_string << "Restarting Local Leader Services for round ["<<counter<<"].");
				FILE * fp = popen("sudo /opt/HYDRAstor/objectStorage/scripts/osmd.sh blockingRestart", "r");
				if(fp == NULL){
					OSDLOG(ERROR, this->logger_string << "Process error ocurred while restarting local leader!");
					if(this->stop_service){
						this->comm_sock.reset();
						//PASSERT(false, "Process error ocurred while restarting local leader!");
						OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
						exit(1);
					}
				}
				uint64_t error_code = pclose(fp);
				error_code = error_code/256;
				if(error_code != 0)
				{
					OSDLOG(ERROR, this->logger_string << "LL service restart failed for round ["<<counter<<"].");
					counter++;
				}
				else
				{
					OSDLOG(INFO, "Successfully re-started Local Leader service in round ["<<counter<<"].");
					break;
				}
			}
			if (counter >= 2 && this->stop_service)
			{
				OSDLOG(ERROR, this->logger_string << "LL service restart failed!");
				this->comm_sock.reset();
				//PASSERT(false, "LL service restart failed!");
				OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
				exit(1);
			}
		}
		catch(...){
			OSDLOG(ERROR, this->logger_string << "Exception raised while restarting local leader! ");
			this->comm_sock.reset();
			//PASSERT(false, "Exception raised while restarting local leader!");
			OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
			exit(1);
		}
	}


	/*
	Function Name : create_connection
	Purpose : Function to create conection to OSM
	*/
	bool healthMonitoringController::create_connection(int timeout){
		uint64_t counter = 0;
		while(true){
			if(counter >= 3){
				OSDLOG(ERROR, this->logger_string << "3 connection retries failed for " << this->self_service_id);
				this->comm_sock.reset();
				//PASSERT(false, "3 connection retries failed!");
				OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
				exit(1);
			}
			this->comm_sock.reset(new sync_com(this->self_ip, this->ll_port));
			if(!this->comm_sock->is_connected())
			{
				OSDLOG(DEBUG, this->logger_string << "Connection not established with OSM for round "<< counter);
				counter++;
				boost::this_thread::sleep(boost::posix_time::seconds(timeout));
			}
			else
			{
				OSDLOG(INFO, this->logger_string << "Connection established with OSM in round " << counter);
				return true;
			}
		}
		return false;
	}

	/*
	Function Name : start_monitoring
	Purpose : Main function for sending STRM and HRBT message to local leader
	*/
	void healthMonitoringController::start_monitoring(bool restart_needed){
		try{
			{
				boost::mutex::scoped_lock lock(this->mtx);
				bool connection_success = false;
				boost::this_thread::sleep( boost::posix_time::seconds(10));
				connection_success = this->create_connection(0);
				if (connection_success)
				{
					this->is_connected = true;
					this->condition.notify_one();
				}
			}
			boost::this_thread::sleep(boost::posix_time::seconds(this->strm_timeout));
			this->send_start_monitoring_request_local_leader(restart_needed);
			this->send_heartbeat_local_leader(restart_needed);
		}
		catch(...){
			OSDLOG(ERROR, this->logger_string << "Exception raised in health monitoring!");
			this->comm_sock.reset();
			//PASSERT(false, "Exception raised in health monitoring!");
			OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
			exit(1);
		}
	}
	
	/*
	Function name : send_start_monitoring_request_local_leader
	Purpose : send strm request to Local Leader
	*/
	bool healthMonitoringController::send_start_monitoring_request_local_leader(bool restart_needed)
	{
		boost::shared_ptr<MessageResultWrap> result;
		bool return_status = false;
		uint64_t counter = 0;
		while(this->flag){
			if (counter == 3 && restart_needed == true){
				//restart osm service
				OSDLOG(DEBUG, this->logger_string << "3 strms packets failed, restarting local leader service");
				this->restart_service();
				this->create_connection(0);
				counter = 0;
			}
			if (counter >= 3){
				OSDLOG(ERROR, this->logger_string << "3 strms packets failed for " << this->self_service_id);
				if(this->stop_service){
					//PASSERT(false, "3 strms packet failed, killing the service!");
					OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
					this->comm_sock.reset();
					exit(1);
				}
			}
			OSDLOG(INFO, this->logger_string << "STRM request started for " << this->self_service_id);
			result = MessageExternalInterface::send_start_monitoring_message_to_local_leader_synchronously(
					this->self_ip,
					this->self_port,
					this->self_service_id,
					this->comm_sock,
					this->strm_timeout
				);
			if (!this->verify_start_monitoring_response(result)){
				OSDLOG(INFO, this->logger_string << "STRM request failed for " << this->self_service_id);
				counter+=1;
				boost::this_thread::sleep(boost::posix_time::seconds(this->strm_timeout));
			}
			else{
				OSDLOG(DEBUG, this->logger_string << "STRM request completed for " << this->self_service_id);
				counter = 0;
				return_status = true;
				break;
			}
		}
		return return_status;
	}
 
 	/*
	Function name : send_heartbeat_local_leader
	Purpose : send HEART_BEAT to Local Leader
	*/
 	void healthMonitoringController::send_heartbeat_local_leader(bool restart_needed)
	{
		boost::shared_ptr<MessageResultWrap> result;
		uint64_t counter = 0;
		while(this->flag)
		{
			if (counter > 0 && this->channel_break == true && restart_needed == false)
			{
				OSDLOG(INFO, this->logger_string << "HRBT packet failed due to channel break. Reconnecting...");
				boost::this_thread::sleep(boost::posix_time::seconds(30));
				this->create_connection(30);
				OSDLOG(DEBUG, this->logger_string << "Sending STRM on new connection");
				bool strm_success = this->send_start_monitoring_request_local_leader(false);
				if (strm_success)
				{
					counter = 0;
					this->channel_break = false;
				}
			}

			if (counter == 2 && restart_needed == true)
			{
				//restart osm service
				OSDLOG(INFO, this->logger_string << "2 hrbt packets failed, restarting local leader service");
				this->restart_service();
				this->create_connection(0);
				bool strm_success = send_start_monitoring_request_local_leader(false);
				if (strm_success)
				{
					counter = 0;
				}
				else
				{
					OSDLOG(ERROR, this->logger_string << "3 strms packets failed for " << this->self_service_id);
					if(this->stop_service){
						this->comm_sock.reset();
						//PASSERT(false, "3 strms packet failed, killing the service!");
						OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
						exit(1);
					}
				}
			}
			
			if (counter >= 4){
				OSDLOG(ERROR, this->logger_string << "4 hrbts packets failed for " << this->self_service_id);
				if(this->stop_service){
					//PASSERT(false, "4 hrbts packet failed, killing the service!");
					this->comm_sock.reset();
					OSDLOG(ERROR, "------------------------------- ABORTING -------------------------------");
					exit(1);
				}
			}

			OSDLOG(INFO, this->logger_string << "HRBT request started for " << this->self_service_id);
			result = MessageExternalInterface::send_heartbeat_message_synchronously(
				this->self_service_id,
				this->comm_sock,
				this->hbrt_timeout
				);
			int ret_status = this->verify_heartbeat_response(result);
			if (ret_status != HRBT_SUCCESS)
			{
				if (ret_status == HRBT_TIMEOUT)
				{
					OSDLOG(INFO, this->logger_string << "Timeout occurred while waiting for HBRT ACK");
					counter+=1;
				}
				else
				{
					OSDLOG(INFO, this->logger_string << "HRBT request failed for " << this->self_service_id);
					counter+=1;
					this->channel_break = true;
				}
			}
			else{
				OSDLOG(DEBUG, this->logger_string << "HRBT request completed for " << this->self_service_id);
				counter = 0;
			}
			boost::this_thread::sleep(boost::posix_time::seconds(this->hbrt_timeout));
		}
	}

	/*
	Function name : verify_start_monitoring_response
	Purpose : Verify the result of communication with local leader
	*/
	bool healthMonitoringController::verify_start_monitoring_response(boost::shared_ptr<MessageResultWrap> result)
	{
		bool status = false;
		switch (result->get_type()){
			case messageTypeEnum::typeEnums::OSD_START_MONITORING_ACK:
				{
					comm_messages::OsdStartMonitoringAck * msg = new comm_messages::OsdStartMonitoringAck();
					msg->deserialize(result->get_payload(), result->get_size());
					boost::shared_ptr<ErrorStatus> error_status = msg->get_error_status();
					if (error_status->get_code() != OSDException::OSDSuccessCode::OSD_START_MONITORING_SUCCESS){
						OSDLOG(ERROR, this->logger_string << "Error code : " << error_status->get_code() << " message : " << error_status->get_msg());
						break;
					}
					else{
						OSDLOG(DEBUG, this->logger_string << "Recieved :  " << error_status->get_code());
						status = true;
						break;
					}
				}
			default:
				{
					OSDLOG(ERROR, this->logger_string << "Wrong packet recieved in ack for " << this->self_service_id);
					break;
				}
		}
		return status;
	}

	/*
	Function name : verify_heartbeat_response
	Purpose : Verify the result of communication with local leader
	*/
	int healthMonitoringController::verify_heartbeat_response(boost::shared_ptr<MessageResultWrap> result)
	{
		int status;
		if(result->get_error_code() == Communication::SUCCESS)
		{
			switch (result->get_type()){
				case messageTypeEnum::typeEnums::HEART_BEAT_ACK:
				{
					OSDLOG(DEBUG, this->logger_string << "Recevied Heartbeat Acknowledgement for " << self_service_id);
					status = HRBT_SUCCESS;
					break;
				}

				default:
				{
					OSDLOG(ERROR, this->logger_string << "Wrong packet recieved in ack for heartbeat of" << this->self_service_id);
					status = HRBT_FAILURE;
					break;
				}
			}
		}
		else if(result->get_error_code() == Communication::TIMEOUT_OCCURED)
		{
			status = HRBT_TIMEOUT;
		}
		else
		{
			status = HRBT_FAILURE;
		}
		return status;
	}
}
