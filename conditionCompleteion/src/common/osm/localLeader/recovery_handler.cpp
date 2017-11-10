#include "osm/localLeader/recovery_handler.h"


void alarm_signal_handler(int sig_no)
{
	cool::unused(sig_no);
	throw OSDException::Alram();
}

CommandExecuter::CommandExecuter()
{
}

CommandExecuter::~CommandExecuter()
{
}

std::string CommandExecuter::get_command(std::string service_name, std::string action) const
{
	std::string cmd = "";
	if(action == std::string("start") || action == std::string("START"))
	{
		cmd = osd_script_path + "osd-blocking-start";
		cmd = cmd + " " + service_name;
		return cmd;
	}
	//std::string cmd = path + "osd-init-wrap"; 
	cmd = osd_script_path + "osd-init"; 
	cmd = cmd + " " + service_name + " " + action;
	return cmd;
}
#ifdef DEBUG_MODE
void CommandExecuter::exec_command(const char* cmd,  FILE** outStdoutFD, int *out_status, uint32_t timeout)
{
	*outStdoutFD = (FILE*)0xffff;
	*out_status = 0;
}
#else
void CommandExecuter::exec_command(const char* cmd,  FILE** outStdoutFD, int *out_status, uint32_t timeout)
{
	::signal(SIGALRM, alarm_signal_handler);
	FILE * fp = popen(cmd, "r");
	//alarm(timeout);
	try
	{
		if( fp == NULL)
		{
			*outStdoutFD = NULL;
		}
		char buf[50];
		while ( !feof(fp) && fgets(buf,sizeof(buf),fp) );
		*outStdoutFD = fp;
		*out_status = pclose(fp);
	}
	catch(OSDException::Alram& e)
	{
		*out_status = OSDException::OSDErrorCode::TIMEOUT_EXIT_CODE;
		*outStdoutFD = NULL;
	}
}
#endif

RecoveryManager::RecoveryManager()
{
}

RecoveryManager::~RecoveryManager()
{
}

#ifdef DEBUG_MODE
bool RecoveryManager::task_executer(std::string cmd)
{
	return true;
}
#else
/*task_executer function returns true if command successfully executed else return false*/
bool RecoveryManager::task_executer(std::string cmd)
{
	CommandExecuter cmd_exec;
	int status;
	FILE *fp;
	cmd_exec.exec_command(cmd.c_str(), &fp, &status);
	OSDLOG(INFO, "Status of executed command ="<<cmd <<" is: "<<(status));
	if( (status) || (fp == NULL) )
		return false;
	return true;
}
#endif

bool RecoveryManager::start_service(std::string service_name, std::string port)
{
	CommandExecuter cmd_exec;
	std::string cmd  = cmd_exec.get_command(service_name, START); 
	cmd = cmd + " -l" + " " + port; 
	OSDLOG(INFO, "Command for start service = "<< cmd);
	return this->task_executer(cmd);
}
void RecoveryManager::kill_service(std::string service_name)
{
	OSDLOG(INFO, "Killing service : " << service_name);
	if(service_name != std::string("account-updater"))
	{
		service_name = service_name + "-server";
	}
	std::string cmd = "killall -9 osd-";
	cmd = cmd + service_name;
	this->task_executer(cmd);
}

bool RecoveryManager::stop_all_service()
{
	std::string cmd = osd_script_path + "osd-init status all"; 
	OSDLOG(INFO, "Command for status check =: "<<cmd);
	if( true == this->task_executer(cmd))
	{
		/*true means status all command was successful, i.e., services are running*/
		cmd = osd_script_path + "osd-init stop all";
		OSDLOG(INFO, "Command for stop service =: " << cmd);
		if(true == this->task_executer(cmd))	
		{
			OSDLOG(INFO, "All osd services stoped");
			return true;
		}
	}
	OSDLOG(INFO, "Init script could not stop all osd services");
	return false; 
}

bool RecoveryManager::start_all_service(std::string port)
{
	//std::string cmd = path + "osd-init-wrap start all";
	/*
	std::string cmd = path + "osd-init start all";
	cmd = cmd + " -l" + " " + port; 
	return this->task_executer(cmd);
	*/
/*start*/
	std::vector<std::string> task_list;
	task_list.push_back("account");
	task_list.push_back("container");
	task_list.push_back("object");
	task_list.push_back("proxy");
	task_list.push_back("account-updater");
	std::vector<std::string>::iterator it;
	for( it = task_list.begin(); it != task_list.end(); it++)
	{
		if(false == start_service(*it, port) )
		{
			return false;
		}
	}
	return true;
}

bool RecoveryManager::stop_service(std::string service_name)
{
	CommandExecuter cmd_exec;
	std::string cmd  = cmd_exec.get_command(service_name, "stop");
	return this->task_executer(cmd);
}

