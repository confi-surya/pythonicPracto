#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "osm/globalLeader/monitoring.h"
#include "osm/osmServer/osm_info_file.h"
#include "communication/message_binding.pb.h"

namespace MonitoringTest
{
cool::SharedPtr<config_file::OSMConfigFileParser> const get_config_parser()
{
        cool::config::ConfigurationBuilder<config_file::OSMConfigFileParser> config_file_builder;
        char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
        if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/osm.conf"));
        else
                config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/osm.conf");
        return config_file_builder.buildConfiguration();
}

cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");

std::string path = parser_ptr->osm_info_file_pathValue();

TEST(MonitoringTest, Monitoring_HealthMonitoringRecord_Test)
{
	OSDLOG(INFO, "Monitoring Tests------");
	time_t timer;
	time(&timer);
	boost::shared_ptr<gl_monitoring::HealthMonitoringRecord> hmrecord(new gl_monitoring::HealthMonitoringRecord(gl_monitoring::STRM, timer, gl_monitoring::NEW, 30, 0, OSDException::OSDHfsStatus::NOT_READABILITY, 1));
	ASSERT_EQ(gl_monitoring::STRM, hmrecord->get_expected_msg_type());
	hmrecord->set_expected_msg_type(gl_monitoring::HRBT);
	ASSERT_EQ(gl_monitoring::HRBT, hmrecord->get_expected_msg_type());

	ASSERT_EQ(timer, hmrecord->get_last_updated_time());
	time_t timer1;
	time(&timer1);
	hmrecord->set_last_updated_time(timer1);
	ASSERT_EQ(timer1, hmrecord->get_last_updated_time());

	ASSERT_EQ(gl_monitoring::NEW, hmrecord->get_status());
	hmrecord->set_status(network_messages::RECOVERED);
	ASSERT_EQ(network_messages::RECOVERED, hmrecord->get_status());

	ASSERT_EQ(0, hmrecord->get_timeout());

	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler1( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler1->init());
	ASSERT_EQ(0, hmrecord->get_failure_count());
	hmrecord->increment_failure_count();
	ASSERT_EQ(1, hmrecord->get_failure_count());
	
}

TEST(MonitoringTest, Monitoring_add_entry_in_map_Test)
{
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	std::string node_id = "HN0101_60123";
	msg->set_service_id(node_id);
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));

	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));

	ASSERT_FALSE(monitoring->if_node_already_exists(node_id));
	ASSERT_TRUE(monitoring->add_entry_in_map(msg));
	ASSERT_TRUE(monitoring->if_node_already_exists(node_id));
	ASSERT_FALSE(monitoring->if_node_already_exists("HN0111_60123"));
}

TEST(MonitoringTest, Monitoring_add_entry_in_map_while_recovery_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));

	ASSERT_TRUE(monitoring->add_entries_while_recovery("HN0102_60123", gl_monitoring::HRBT, network_messages::FAILED));
}

TEST(MonitoringTest, Monitoring_remove_node_from_monitoring_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));

	ASSERT_TRUE(monitoring->add_entries_while_recovery("HN0101_60123", gl_monitoring::HRBT, network_messages::RETIRED));
	ASSERT_TRUE(monitoring->add_entries_while_recovery("HN0102_60123", gl_monitoring::HRBT, network_messages::FAILED));

	ASSERT_FALSE(monitoring->remove_node_from_monitoring("HN0101_60123"));
	ASSERT_TRUE(monitoring->remove_node_from_monitoring("HN0102_60123"));
}

TEST(MonitoringTest, Monitoring_get_node_status_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));

	monitoring->add_entries_while_recovery("HN0102_60123", gl_monitoring::HRBT, network_messages::FAILED);
	ASSERT_EQ(network_messages::FAILED, monitoring->get_node_status("HN0102_60123"));
	ASSERT_EQ(network_messages::INVALID_NODE, monitoring->get_node_status("HN0112_60123"));
}

TEST(MonitoringTest, Monitoring_retrieve_node_status_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));

	monitoring->add_entries_while_recovery("HN0102_60123", gl_monitoring::HRBT, network_messages::FAILED);
	ASSERT_EQ(network_messages::FAILED, monitoring->get_node_status("HN0102_60123"));
	ASSERT_EQ(network_messages::INVALID_NODE, monitoring->get_node_status("HN0112_60123"));
	
	boost::shared_ptr<ServiceObject> node (new ServiceObject());
	node->set_service_id("HN0102_60123_container-server");
	node->set_ip("192.168.101.1");
	node->set_port(60123);
	boost::shared_ptr<comm_messages::NodeStatus> node_status(new comm_messages::NodeStatus(node));

	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm1 (new Communication::SynchronousCommunication("192.168.101.1", 60123));
	monitoring->retrieve_node_status(node_status, sync_comm1);
}

TEST(MonitoringTest, Monitoring_update_time_in_map_Test)
{
	boost::shared_ptr<ServiceObject> msg(new ServiceObject()); //make sure node id is HN0101_60123

	msg->set_service_id("HN0101_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	monitoring->add_entry_in_map(msg); //Remember the Node Id and Time
	monitoring->add_entries_while_recovery("HN0102_60123", gl_monitoring::HRBT, network_messages::FAILED);

	ASSERT_TRUE(monitoring->update_time_in_map("HN0101_60123", OSDException::OSDHfsStatus::NOT_READABILITY)); //Make sure this is not in failed state
	ASSERT_FALSE(monitoring->update_time_in_map("HN0102_60123", OSDException::OSDHfsStatus::NOT_READABILITY)); //Make sure this is in failed state
}

TEST(MonitoringTest, Monitoring_monitor_for_recovery_Test)
{
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	msg->set_service_id("HN0107_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::MonitoringImpl> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	monitoring->add_entry_in_map(msg);

//-------------
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0107_60123", "192.168.101.1", 60123, 61006, 61007, 61008, 61009, 61005, network_messages::REGISTERED));

        std::list<boost::shared_ptr<recordStructure::Record> > record_list;
        record_list.push_back(nifrecord1);
        node_info_file_writer->add_record(record_list);

//----------------

	sleep(8);
	boost::thread bthread(boost::bind(&gl_monitoring::MonitoringImpl::monitor_for_recovery, monitoring));
	sleep(8);
	bthread.interrupt();
	//TODO: populate prepare_and_add_entry
	//ASSERT_FALSE(monitoring->update_time_in_map("HN0107_60123"));
	//node_info_file_writer->delete_record("HN0107_60123");
}

TEST(MonitoringTest, Monitoring_update_map_Test)
{
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	msg->set_service_id("HN0101_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	monitoring->add_entry_in_map(msg);
	monitoring->add_entries_while_recovery("HN0102_60123", gl_monitoring::HRBT, network_messages::FAILED);

	boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_msg1 (new comm_messages::LocalLeaderStartMonitoring("HN0101_60123"));
	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm1 (new Communication::SynchronousCommunication("192.168.101.1", 60123));
	ASSERT_TRUE(monitoring->update_map(strm_msg1)); 
	boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_msg2 (new comm_messages::LocalLeaderStartMonitoring("HN0102_60123"));
	ASSERT_FALSE(monitoring->update_map(strm_msg2));
}

/*TEST(MonitoringTest, Monitoring_add_bucket_Test) //Declared but not Defined
{
	boost::shared_ptr<Bucket> bucket_ptr (new Bucket());
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer;
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::Monitoring(node_info_file_writer));
	monitoring->add_bucket(bucket_ptr);
}*/

TEST(MonitoringTest, Monitoring_get_nodes_status_Test)
{
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	msg->set_service_id("HN0101_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	monitoring->add_entry_in_map(msg);
	monitoring->add_entries_while_recovery("HN0102_60123", gl_monitoring::HRBT, network_messages::FAILED);

	std::vector<std::string> nodes_registered;
	std::vector<std::string> nodes_failed;
	std::vector<std::string> nodes_retired;
	std::vector<std::string> nodes_recovered;
	monitoring->get_nodes_status(nodes_registered, nodes_failed, nodes_retired, nodes_recovered);
	ASSERT_EQ(1, int (nodes_registered.size()));
	ASSERT_EQ(1, int (nodes_failed.size()));
	
	ASSERT_EQ(0, int (nodes_retired.size()));

}

TEST(MonitoringTest, Monitoring_change_status_in_monitoring_component_Test)
{
        boost::filesystem::remove(boost::filesystem::path("/remote/hydra042/gmishra/journal_dir/node_info_file.info"));
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	msg->set_service_id("HN0101_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	monitoring->add_entry_in_map(msg);

//-------------
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.101.1", 60123, 61006, 61007, 61008, 61009, 61005, network_messages::REGISTERED));

        std::list<boost::shared_ptr<recordStructure::Record> > record_list;
        record_list.push_back(nifrecord1);
        node_info_file_writer->add_record(record_list);

//----------------

	ASSERT_TRUE(monitoring->change_status_in_monitoring_component("HN0101_60123", network_messages::FAILED));
	ASSERT_FALSE(monitoring->change_status_in_monitoring_component("HN0109_60123", network_messages::FAILED));
}

TEST(MonitoringTest, get_node_status_Test)
{
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	msg->set_service_id("HN0101_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	boost::shared_ptr<comm_messages::NodeStatus> node_status_req (new comm_messages::NodeStatus(msg));
	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm1 (new Communication::SynchronousCommunication("192.168.101.1", 60123));
	
}

TEST(MonitoringThreadTest, MonitoringThread_add_bucket_Test)
{
	boost::shared_ptr<Bucket> bucket_ptr (new Bucket());
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	
	boost::shared_ptr<gl_monitoring::MonitoringThread> monitoringthread (new gl_monitoring::MonitoringThread(monitoring, bucket_ptr));
	ASSERT_TRUE(monitoringthread->add_bucket(bucket_ptr));
}

TEST(MonitoringThreadTest, MonitoringThread_add_node_for_monitoring_Test)
{
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	msg->set_service_id("HN0101_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);
	boost::shared_ptr<Bucket> bucket_ptr (new Bucket());
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	
	//boost::shared_ptr<gl_monitoring::MonitoringThread> monitoringthread (new gl_monitoring::MonitoringThread(monitoring, bucket_ptr));
	boost::shared_ptr<gl_monitoring::MonitoringThreadManager> monitoringthreadmanager (new gl_monitoring::MonitoringThreadManager(monitoring));

	ASSERT_TRUE(monitoringthreadmanager->add_node_for_monitoring(msg));
	
	

}

TEST(MonitoringThreadTest, MonitoringThread_receive_heartbeat_Test)
{
	boost::shared_ptr<Bucket> bucket_ptr (new Bucket());
	//boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_msg1 (new comm_messages::LocalLeaderStartMonitoring("HN0101_60123", 60123, "192.168.101.1"));
	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm1(new Communication::SynchronousCommunication("192.168.101.1", 60123));
	bucket_ptr->add_fd(1, sync_comm1);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	boost::shared_ptr<gl_monitoring::MonitoringThread> monitoringthread (new gl_monitoring::MonitoringThread(monitoring, bucket_ptr));
	boost::shared_ptr<comm_messages::HeartBeat> hbrt = monitoringthread->receive_heartbeat(1);
	ASSERT_EQ("HEARTBEAT", hbrt->get_msg());
	//monitoringthread->close_all_sockets();
}

/*TEST(MonitoringThreadTest, MonitoringThread_send_heartbeat_ack_Test)
{
	boost::shared_ptr<Bucket> bucket_ptr (new Bucket());
	boost::shared_ptr<Communication::SynchronousCommunication> sync_comm1(new Communication::SynchronousCommunication("192.168.101.1", 60123));
	bucket_ptr->add_fd(1, sync_comm1);
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	boost::shared_ptr<gl_monitoring::MonitoringThread> monitoringthread (new gl_monitoring::MonitoringThread(monitoring, bucket_ptr));
	monitoringthread->send_heartbeat_ack(1, 1);
}*/

TEST(MonitoringThreadManagerTest, MonitoringThreadManager_monitor_service_Test)
{
	 //TODO: Aborted Terminated by an unhandled exception: mutex: Invalid argument 
	/*
	boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> strm_msg1 (new comm_messages::LocalLeaderStartMonitoring("HN0101_60123"));
	
	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication("192.168.101.1", 60123));
	
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> node_info_file_writer (new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring (new gl_monitoring::MonitoringImpl(node_info_file_writer));
	
	boost::shared_ptr<gl_monitoring::MonitoringThreadManager> monitoringthreadmanager (new gl_monitoring::MonitoringThreadManager(monitoring));
	OSDLOG(DEBUG, "Updating Monitoring Map for STRM");
	ASSERT_FALSE(monitoring->update_map(strm_msg1)); 
	
	boost::shared_ptr<ServiceObject> msg (new ServiceObject());
	msg->set_service_id("HN0101_60123");
	msg->set_ip("192.168.101.1");
	msg->set_port(60123);

	ASSERT_TRUE(monitoring->add_entry_in_map(msg));
	ASSERT_TRUE(monitoring->update_map(strm_msg1)); 
	
	//ASSERT_TRUE(monitoring->add_entries_while_recovery("HN0101_60123", gl_monitoring::HRBT, network_messages::FAILED));
	monitoringthreadmanager->monitor_service(strm_msg1, comm);
	monitoringthreadmanager->stop_all_mon_threads();
	*/
	
	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) )
	{
		//std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));

}

} //namespace InfoFileHelperWriterTest
