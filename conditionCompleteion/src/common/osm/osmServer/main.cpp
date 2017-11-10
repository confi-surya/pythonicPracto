#include <iostream>
#include <fcntl.h>
#include <unistd.h>   // ftruncate, write
#include <errno.h>
#include "cool/types.h"
#include "cool/threads.h"
#include <sys/resource.h>
#include <cool/debug.h>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <sys/types.h>
#include <unistd.h>

#include "osm/osmServer/utils.h"
#include "osm/osmServer/msg_handler.h"
#include "osm/osmServer/osm_daemon.h"
#include "osm/osmServer/pid_file.h"
#include "osm/osmServer/osm_info_file.h"

#include "osm/globalLeader/global_leader.h"

#include "osm/localLeader/local_leader.h"
#include "osm/localLeader/ll_msg_handler.h"

#include "libraryUtils/osd_logger_iniliazer.h"
#include "libraryUtils/object_storage_exception.h"
#include "libraryUtils/record_structure.h"

#include "cool/log.h"
#include "cool/aio.h"
#include "cool/option.h"
#include "mts/hydraProdMacro.h"
#include "cool/boostTime.h"
#include "cool/shellCommandsExecution.h"

Completion shutDown;
static char const * data_daemon_pid_file_name = NULL;
static int data_daemon_pid_file_fd = -1;

void signal_handler(int32 sig)
 {
	switch(sig)
	{
		case SIGHUP:
			OSDLOG(INFO, "Osm daemon recvd SIGHUP signal");
			break;
/*		case SIGTERM:
			OSDLOG(INFO, "Osm daemon recvd SIGTERM signal");
			shutDown.signal();
			break;
*/		case SIGALRM:
			OSDLOG(INFO, "Osm daemon recvd SIGALRM signal");
			break;
	}
}

int create_lock_file(const char * const  pid_file_name, LockFileBlocking::Enum should_block)
{
	PASSERT(pid_file_name != NULL, "Attempt to create file without telling its name");
	int const fd = ::open(pid_file_name, O_WRONLY|O_CREAT, 0644); // EINTR can't be returned from open
	if (fd < 0)
	{
		int const open_errno = errno;
		OSDLOG(ERROR, "Unable to open file: " << pid_file_name << " with error: "<< open_errno);  
		return -1;
	}
	if (::lockf(fd, (should_block == LockFileBlocking::BLOCKING) ? F_LOCK : F_TLOCK, 0) < 0)
	{
		int const lockf_errno = errno;
		OSDLOG(ERROR, "Unable to lockfile: " << pid_file_name << " with error: "<<lockf_errno);
		::close(fd);
		return -1;
	}
	return fd;
}

int create_pid_file(const char * const  pid_file_name)
{
	return create_lock_file(pid_file_name, LockFileBlocking::NOT_BLOCKING);
}

void set_pid_file_name(const char * pid_file_name)
{
	::data_daemon_pid_file_name = pid_file_name;
}

int write_pid_to_pid_file(int const pid_file_fd)
{
	if( ::ftruncate(pid_file_fd, 0) < 0)
	{
		int const ftruncate_errno = errno;
		OSDLOG(ERROR, "Failed to truncate the pid file for fd "<<pid_file_fd);
		return -1;
	}
	char pid_str[16];
	::snprintf(pid_str, sizeof(pid_str), "%d\n", ::getpid());
	pid_str[sizeof(pid_str) - 1] = '\0';
	int const pid_strlen = ::strlen(pid_str);
	if (::write(pid_file_fd, pid_str, pid_strlen) != pid_strlen)
	{
		int const write_errno = errno;
		OSDLOG(ERROR, "Failed to write PID to pid file fd: "<<pid_file_fd);
		return -1;
	}
	OSDLOG(INFO,  "Pid file fd : " << pid_file_fd << " filled with value: '" << pid_str << "'");
	return 0;
}

void unlock_file(int file_fd)
{               
        PASSERT(file_fd != -1, "invalid file descriptor");
        if (::close(file_fd) != 0)
        {       
                int const ecopy = errno;
                OSDLOG(ERROR, "failed to close pidFile " << file_fd << " errno " << ecopy);
        }
}

void destroy_data_daemon_pid_file()
{
	PASSERT(::data_daemon_pid_file_name != NULL, "Attempt to remove file without telling its name");
	if (::data_daemon_pid_file_fd != -1)
	{
		if (close(::data_daemon_pid_file_fd) < 0)
		{
			int const close_errno = errno;	
			OSDLOG(ERROR, "Failed to close the pid file fd " <<close_errno);
		}
	}
	::data_daemon_pid_file_fd  = -1;
	if (remove(::data_daemon_pid_file_name) < 0)
	{
		int const remove_errno = errno;
		OSDLOG(ERROR, "Failed to destroy the pid file: " << ::data_daemon_pid_file_name << " with error "<< remove_errno);
		return;	
	}
	OSDLOG(DEBUG, "Pid file: " << ::data_daemon_pid_file_name << " destroyed");
	return;
}

int daemonize()
{
	if( daemon(0, 0) < 0)
	{
		OSDLOG(INFO, "Osd Daemonized failed");
		return -1;
	}
	::set_pid_file_name("/var/run/osmd.pid");
	::data_daemon_pid_file_fd = create_pid_file(::data_daemon_pid_file_name);
	if( ::data_daemon_pid_file_fd < 0)
	{
		return -1;	
	}
	atexit(destroy_data_daemon_pid_file);
	if (write_pid_to_pid_file(::data_daemon_pid_file_fd) < 0)
	{
		return -1; 
	}
	umask(0);
	//signal handling ...
	signal(SIGHUP, signal_handler); // catch hangup signal:
	signal(SIGTERM, signal_handler); // catch kill signal
	signal(SIGALRM, signal_handler); // SIGALRM signal handling temporary solution...
	return 0;
}

/*int get_msg_for_main_queue()
{
	while(1)
	{
		
	}
}*/

int main()
{
	INIT_CATALOG("/opt/ESCORE/etc/nls/msg/C/%N.cat");
	LOAD_CATALOG("system_state", "HOS", "HOS", "HOS");
	LOAD_CATALOG("gl_monitoring", "HOS", "HOS", "HOS");
	LOAD_CATALOG("local_leader", "HOS", "HOS", "HOS");
	LOAD_CATALOG("MonitoringManager", "HOS", "HOS", "HOS");
	LOAD_CATALOG("ServiceStartupManager", "HOS", "HOS", "HOS");
	LOAD_CATALOG("hfs_checker", "HOS", "HOS", "HOS");

	boost::shared_ptr<OSD_Logger::OSDLoggerImpl>logger;
	GLUtils gl_utils_obj;
	logger.reset(new OSD_Logger::OSDLoggerImpl("osm"));
	try
	{
		logger->initialize_logger();
		OSDLOG(INFO, "logger initiallization succussfuly");
	}
	catch(...)
	{
		shutDown.signal();
	}
	if( daemonize() < 0)
	{
		OSDLOG(INFO, "Failed to daemonize the osmd");
		shutDown.signal();
	}

	bool if_cluster = false;
	bool env_check = false;
	bool if_i_am_gl = false;

	common_parser::Executor ex_obj;
	boost::shared_ptr<common_parser::MessageParser> parser_obj(new common_parser::MessageParser(ex_obj));

	OsmUtils ut;
	cool::SharedPtr<config_file::OSMConfigFileParser> config_parser;
	config_parser = gl_utils_obj.get_config();

	boost::shared_ptr<Listener> listener_ptr(new Listener(parser_obj, config_parser));
	listener_ptr->create_sockets();

	std::string config_fs_name = config_parser->config_filesystemValue();
	std::string path = gl_utils_obj.get_mount_path(config_parser->export_pathValue(), config_fs_name);
	//Enable the core dump file
	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core_limits);

	//Verify if all file systems mounted
	bool is_filesystem_mounted = gl_utils_obj.create_and_mount_file_system(config_parser->export_pathValue(),
			config_parser->mount_commandValue(), config_parser->mkfs_commandValue(),
			config_parser->fsexist_commandValue(), config_parser->HFS_BUSY_TIMEOUTValue() );
	if(false == is_filesystem_mounted)
	{
		OSDLOG(DEBUG, "osd_meta_config Filesystem couldn't mount. Exiting...");
		return -1;
	}

	//Environment Check- HFS
	env_check =  gl_utils_obj.check_environment(path);
	//Verify if cluster exists
	if_cluster = gl_utils_obj.verify_cluster_existence(config_parser->node_info_file_pathValue(), config_fs_name);
	int32_t tcp_port = config_parser->osm_portValue(); 	

	//Verify if i am GL
	if_i_am_gl = gl_utils_obj.verify_if_gl(if_cluster, tcp_port);

	boost::shared_ptr<global_leader::GlobalLeader> gl_obj;
	boost::shared_ptr<gl_parser::GLMsgHandler> gl_msg_ptr;
	boost::shared_ptr<boost::thread> gl_thread;
	boost::shared_ptr<common_parser::GLMsgProvider> gl_msg_provider_obj;

	OSDLOG(DEBUG, "Address before passing " << gl_obj.get()  << "  " << gl_msg_ptr.get() << "  " << gl_thread.get());
	if(if_i_am_gl)
	{
		int ret1 = instantiate_GL(gl_obj, gl_msg_ptr, gl_thread, parser_obj, if_cluster, env_check, tcp_port, true, false, gl_msg_provider_obj);
		if(ret1 != SUCCESS)
		{
			OSDLOG(ERROR, "Could not run GL on this node, error code " << ret1);
		}
	}
	bool if_part_of_cluster = false;
	//enum OSDNodeStat::NodeStatStatus::node_stat node_stat = OSDNodeStat::NodeStatStatus::NODE_NEW; 
	enum network_messages::NodeStatusEnum node_stat = network_messages::NEW;

	std::string node_id = gl_utils_obj.get_my_node_id() + "_" +
                                        boost::lexical_cast<std::string>(config_parser->osm_portValue());
/*
	osm_info_file_manager::NodeInfoFileReadHandler nodeInfoFileReadObj(path);	
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> recordPtr = nodeInfoFileReadObj.get_record_from_id(node_id);
	if(recordPtr)
		node_stat = static_cast<network_messages::NodeStatusEnum>(recordPtr->get_node_status());
*/
	//Verify if i am part of cluster if cluster exists
	if(if_cluster) // and node_stat != network_messages::NEW
	{
		//if_part_of_cluster = gl_utils_obj.verify_if_part_of_cluster(node_id);
		node_stat = gl_utils_obj.verify_if_part_of_cluster(node_id);;
		if(node_stat != network_messages::NEW)
			if_part_of_cluster = true;
	}
	if(if_part_of_cluster)
	{
		is_filesystem_mounted = gl_utils_obj.mount_file_system(config_parser->export_pathValue(),
				config_parser->mount_commandValue(), 
				config_parser->HFS_BUSY_TIMEOUTValue() );
		if(!is_filesystem_mounted)
		{
			OSDLOG(DEBUG, "Filesystem couldn't mount. Exiting...");
			return -1;
		}

	}

	node_stat = gl_utils_obj.verify_if_part_of_cluster(node_id);
	//TODO: If node is marked failed/recovered until this point, do not start services as it is.
	//In GA, handling is to be done at GL to return error code to services,
	//requesting components in failed/recovered state and handling of this at services to kill themselves.

	boost::shared_ptr<common_parser::LLMsgProvider> ll_msg_provider_obj(new common_parser::LLMsgProvider);
	boost::shared_ptr<common_parser::LLNodeAddMsgProvider> na_msg_provider(new common_parser::LLNodeAddMsgProvider);
	boost::shared_ptr<local_leader::LocalLeader> ll_ptr(
			new local_leader::LocalLeader(
				node_stat,
				ll_msg_provider_obj,
				na_msg_provider,
				parser_obj,
				config_parser,
				tcp_port
				)
			);

	boost::shared_ptr<ll_parser::LlMsgHandler> ll_msg_ptr(ll_ptr->get_ll_msg_handler());
	if (if_part_of_cluster)
	{
		// LL should run in full mode
		ll_msg_provider_obj->register_msgs(parser_obj, ll_msg_ptr);
		//parser_obj.register_ll_functionality(ll_msg_obj, true);
	}
	else
	{
		// LL should run in wait mode
		na_msg_provider->register_msgs(parser_obj, ll_msg_ptr);
		//parser_obj.register_ll_functionality(ll_msg_obj, false);
	}

		
	boost::shared_ptr<boost::thread> ll_thread(new boost::thread(boost::bind(&local_leader::LocalLeader::start, ll_ptr)));
	boost::shared_ptr<boost::thread> listener_thread(new boost::thread(boost::bind(&Listener::accept, listener_ptr)));

	boost::shared_ptr<MainThreadHandler> main_handler_obj(
				new MainThreadHandler(gl_obj,
							gl_msg_ptr,
							gl_thread,
							parser_obj,
							ll_ptr,
							listener_ptr,
							listener_thread,
							config_parser,
							tcp_port,
							if_i_am_gl,
                                                        gl_msg_provider_obj
							)
						);
	boost::shared_ptr<common_parser::MainThreadMsgProvider> main_msg_provider_obj(new common_parser::MainThreadMsgProvider);
	main_msg_provider_obj->register_msgs(parser_obj, main_handler_obj);
	OSDLOG(DEBUG, "All relevant threads have started.");
	//Now main thread is waiting on queue, which will handle msgs like NODE_STOP, GL stop at runtime etc
	//boost::thread main_thread(boost::bind(&MainThreadHandler::handle_msg, main_handler_obj));
	main_handler_obj->handle_msg();
	SHUTDOWN_CATALOG();
	shutDown.wait();
}
