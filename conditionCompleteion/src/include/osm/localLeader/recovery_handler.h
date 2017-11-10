#ifndef __recovery_Handler_h__
#define __recovery_Handler_h__

#include <boost/shared_ptr.hpp>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <signal.h>
#include <iostream>

#include "libraryUtils/config_file_parser.h"
#include "libraryUtils/object_storage_exception.h"

#define START "start"
const std::string osd_script_path = "/opt/HYDRAstor/objectStorage/scripts/";


class CommandExecuter
{

public:
	CommandExecuter();
	virtual ~CommandExecuter();
	std::string get_command(std::string, std::string) const;
	void exec_command(const char*, FILE**, int *out_status, uint32_t timeout = 1200);
};

class RecoveryManager
{
	bool task_executer(std::string cmd);
public:	
	RecoveryManager();
	virtual ~RecoveryManager();
	bool start_service(std::string, std::string port);	
	bool stop_service(std::string);
	bool stop_all_service();
	bool start_all_service(std::string port);
	void kill_service(std::string service_name);
};
#endif
