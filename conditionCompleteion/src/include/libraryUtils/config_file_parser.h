#ifndef __CONFIG_FILE_PARSER__
#define __CONFIG_FILE_PARSER__

#include "cool/cool.h"
#include "cool/config/configuration.h"
#include "cool/config/configParamTypes.h"
#include "cool/config/configurationBuilder.h"

namespace config_file
{

CREATE_CONFIGURATION_CLASS(
	AccountConfigFileParser,
	(async_thread_count, uint32_t, 100, "Async Thread Count at Account Library Level")
)

CREATE_CONFIGURATION_CLASS(
	ConfigFileParser,
	(file_writer_threads_count, uint32_t, 3, "no of threads "),
	(transaction_journal_path, std::string, "", "path for transaction journal"),
	(container_journal_path, std::string, "", "path for container journal"),
	(timeout, uint32_t, 30, "timeout"),
	(checkpoint_interval, size_t, 5242880, "check point interval value"),
	(max_journal_size, size_t, 10485760, "max file size"),
	(updater_journal_path, std::string, "", "updater journal path"),
	(max_updater_list_size, uint32_t, 10, "Maximum length of updated containers list"),
	(memory_entry_count, uint32_t, 1000, "Number of entries in Memory"),
	(updater_timeout, int32_t, 3600, "Seconds after which new updater journal will be created"),
	(async_thread_count, uint32_t, 100, "Async Thread Count at Container Library Level"),
	(port, uint32_t, 61011, "port for sending active id list")
)

CREATE_CONFIGURATION_CLASS(
	LogConfig,
	(log_dir, std::string, "/var/log/HYDRAstor/AN/objectStorage", "folder for log directory"),
	(container_library_log_file, std::string, "containerLibrary.log", "folder for log directory"),
        (account_library_log_file, std::string, "accountLibrary.log", "folder for log directory"),
	(object_library_log_file, std::string, "objectLibrary.log", "folder for log directory"),
	(osm_log_file, std::string, "osdMonitoringService.log", "log file name for osm"),
	(proxy_monitoring_log_file, std::string, "proxy-server_hml.log", "proxy monitoring log file"),
	(account_updater_monitoring_log_file, std::string, "accountUpdater_hml.log", "account updater monitoring log file"),
	(log_config_file, std::string, "/opt/HYDRAstor/objectStorage/configFiles/OSDLogConfig.ini", "ini file for logging"),
	(object_storage_root_path, std::string, "/opt/HYDRAstor/objectStorage", "folder for root path"),
	(containerchildserver_library_log_file, std::string, "containerChild_hml.log", "folder for log directory")
)

CREATE_CONFIGURATION_CLASS(
	OsdServiceConfigFileParser,
	(container_strm_timeout, uint64_t, 120, "Local leader will receive the strm packet from container service"),
	(account_strm_timeout, uint64_t, 120, "Local leader will receive the strm packet from account service"),
	(proxy_strm_timeout, uint64_t, 120, "Local leader will receive the strm packet from proxy service"),
	(object_strm_timeout, uint64_t, 120, "Local leader will receive the strm packet from object service"),
	(accountUpdater_strm_timeout, uint64_t, 120, "Local leader will receive the strm packet from accountUpdater service"),
	(containerChild_strm_timeout, uint64_t, 120, "Local leader will receive the strm packet from accountUpdater service"),
	(heartbeat_timeout, uint64_t, 30, "Local leader will receive the heartbeat every this timeout" ),
	(retry_count, uint16_t, 3, "Recovery process will try to start the osd service until count reach to retry_count"),
	(service_start_time_gap,  uint16_t, 300, "Gap between each start of osd service"),
	(ll_strm_timeout, uint32_t, 60, "Global Leader should rcv STRM from LL before failure count multiplied by ll_strm_timeout"),
	(hrbt_failure_count, uint16_t, 2, ""),
	(fs_create_timeout, uint64_t, 120, "Timeout for fs create operation"),
	(fs_mount_timeout, uint64_t, 120, "Timeout for fs mount operation"),
	(node_add_ack_timeout_for_gl, uint32_t, 900, "Timeout for node add ack from LL"),
	(ll_hrbt_failure_count, uint16_t, 8, "Heartbeat count miss after which GL will mark LL as failed"),
	(ll_strm_failure_count, uint16_t, 3, "STRM count miss after which GL will mark LL as failed")
)

CREATE_CONFIGURATION_CLASS(
	OSMConfigFileParser,
	(osm_port, uint16_t, 61014, "OSM listing port"),
	(osm_info_file_path, std::string, "/export/.osd_meta_config/", "OSM Info File location"),
	(config_filesystem, std::string, ".osd_meta_config", "meta filesystem name"),
	(election_count, uint16_t, 3, "count for election start When gl unable to toch the gfs file"),
	(thread_wait_timeout, uint16_t, 30, "timeout for thread" ),
	(export_path, std::string, "/export", "export path" ),
	(journal_path, std::string, "/export/.global_leader_journal", "GL journal dir path"),
	(mount_command, std::string, "/opt/HYDRAstor/hfs/bin/release_prod_sym/mount.hydrafs ", "Command used for mount filesystem"),
	(mkfs_command, std::string, "/opt/HYDRAstor/hfs/bin/release_prod_sym/mkfs.hydrafs", "Command used for creating filesystem"),
	(fsexist_command, std::string, "/opt/HYDRAstor/hfs/bin/release_prod_sym/fsexists.hydrafs", "Command used for creating filesystem"),
	(UMOUNT_HYDRAFS, std::string, "/opt/HYDRAstor/hfs/bin/release_prod_sym/umount.hydrafs ", "Command for unmount filesystem"),
	(HFS_BUSY_TIMEOUT, uint32_t, 900, "Timeout for hfs"),
	(gns_accessibility_interval, uint32_t, 30, "Gns Accessibility Interval"),
	(hfs_avaialble_check_interval, uint32_t, 300, "Hfs available check interval after remount the gfs"),
	(node_addition_response_interval , uint16_t , 300, "Local leader will send the ack to gl within this interval after all osd service start"),
	(account_server_name, std::string, "account", "account server name" ),
	(container_server_name, std::string, "container", "container server name" ),
	(object_server_name, std::string, "object", "object server name" ),
	(proxy_server_name, std::string, "proxy", "proxy server name" ),
	(account_updater_server_name, std::string, "account-updater", "account-updater server name" ),
	(containerChild_name, std::string, "containerChild", "containerChild server name" ),
	(node_info_file_path, std::string, "/export/.osd_meta_config/", "directory path of node info file"),
	(checkpoint_interval, int64_t, 3000, "check point interval value"),
	(max_journal_size, int64_t, 12040, "max file size"),
	(strm_count, uint16_t, 1, "Ll retry the send to strm packet to gl until count reach this strm_count"),
	(remount_interval, uint16_t, 600, "Retry interval for remount the filesystem"),
	(unix_domain_port, std::string, "/var/opt/HYDRAstor/objectStorage/osm_socket", "Unix socket on which OSM service will listen for local requests"),
	(stop_service, bool, true, "bool to raise assert or not when HML cannot communicate with local leader"),
	(strm_ack_timeout_hml, uint16_t, 30, "time after which HML retires STRM message to LL"),
	(hbrt_ack_timeout_hml, uint16_t, 30, "time after which HML retires HBRT message to LL"),
	(tcp_port, int32_t, 61014, "TCP port on which OSM will listen for requests from services on other nodes"),
	(listener_thread_count, int, 10, "Maximum no of threads for handling requests coming at OSM service"),
	(gfs_write_interval, int, 30, "Time after which GL will touch file on GFS"),
	(gfs_write_failure_count, int, 3, "Count of GFS write failure after which GL will assume its failed"),
	(service_stop_interval, uint32_t, 300, "Local leader will kill osd service if service is not stop within this interval"),
	(container_recovery_script_location, std::string, "/opt/HYDRAstor/objectStorage/osd/containerService/container_recovery.py", "Script location of container server recovery process"),
	(account_recovery_script_location, std::string, "/opt/HYDRAstor/objectStorage/osd/accountService/account_server_recovery.py", "Script location of account server recovery process"),
	(updater_recovery_script_location, std::string, "/opt/HYDRAstor/objectStorage/osd/accountUpdaterService/account_updater_recovery.py", "Script location of updater server recovery process"),
	(object_recovery_script_location, std::string, "/opt/HYDRAstor/objectStorage/osd/objectService/object_recovery.py", "Script location of object server recovery process"),
	(recovery_script_wrapper, std::string, "/opt/HYDRAstor/objectStorage/osd/common/recovery_script_executor.sh", ""),
	(select_timeout, int, 300, "Timeout for select call for osd service monitoring"),
	(hrbt_select_timeout, int, 60, "Timeout for select call for Osm service monitoring"),
	(system_stop_file, std::string, "/export/.osd_meta_config/osm_stop", "system stop file"),
	(te_file_wait_timeout, uint16_t , 10, "timeout when election file not exists"),
	(t_el_timeout, uint16_t, 60, "time window to write records in file"),
	(t_notify_timeout, uint16_t, 60, "timeout to take gl ownership"),
	(new_GL_response_timeout, uint16_t, 200, "timeout in which new GL must respond back to co-ordinator"),
	(ll_strm_retry_timeout, uint16_t, 30, "time after which LL retries sending STRM to GL"),
	(total_components, uint16_t, 512, "total number of components per service"),
	(node_status_query_interval, int, 120, "time interval after which LL, which is in failed state, sends node status to GL"),
	(t_el_timeout_delta, uint16_t , 90, "t_el_timeout plus delta"),
	(node_status_time_interval, int , 20 , "time interval when LL doesn't got node status on startup"),
	(hrbt_ack_failure_count, uint16_t, 5, "heartbeat ack failure count when gns stat is writability")
)

CREATE_CONFIGURATION_CLASS(
        StatsConfigParser,
        (product_version, std::string, "V1","Product Version Tag"),
        (contLib_base_stats_dir, std::string, "./cont_lib", "Base directory where the stats files of Container Library will be dumped.(Must be empty)"),
        (transLib_base_stats_dir, std::string, "./trans_lib", "Base directory where the stats files of Transaction Library will be dumped.(Must be empty)"),
        (gl_base_stats_dir, std::string, "./global_leader", "Base directory where the stats files of Global Leader will be dumped.(Must be empty)"),
        (ll_base_stats_dir, std::string, "./local_leader", "Base directory where the stats files of Global Leader will be dumped.(Must be empty)"),
        (cont_serv_stats_dir, std::string, "./container_service", "Base directory where the stats files of Container Service will be dumped"),
        (proxy_serv_stats_dir, std::string, "./proxy_service", "Base directory where the stats files of Proxy Service will be dumped"),
        (obj_serv_stats_dir, std::string, "./object_service", "Base directory where the stats files of Proxy Service will be dumped"),
        (acc_serv_stats_dir, std::string, "./account_service", "Base directory where the stats files of Proxy Service will be dumped"),
	(acc_lib_stats_dir, std::string, "./acc_lib", "Base directory where the stats files of Proxy Service will be dumped"),
        (stat_collect_interval, uint64_t, 500, "Stats collection time interval"),
        (journal_memory_limit, uint64_t, 10485760, "Journal File Size limit"),
        (journal_compress_limit, uint64_t, 10485760, "Journal Compress limit")
        )
} // namespace config_file

#endif
