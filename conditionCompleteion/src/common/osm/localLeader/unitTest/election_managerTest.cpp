#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "osm/localLeader/monitoring_manager.h"
#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/service_config_reader.h"
#include <unistd.h>
#include <boost/filesystem.hpp>
#include "osm/localLeader/election_manager.h"
#include "libraryUtils/config_file_parser.h"
#include "osm/osmServer/osm_info_file.h"
#include "stdlib.h"
#include <iostream>
#include "stdio.h"
#include <fstream>
#include <dirent.h>
#include <vector>
#include <algorithm>

namespace ElectionManagerTest
{ 
	std::string osd_glb;
	std::string command;
	std::string expt;
	std::string mk_dir;

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

	void osd_global_file_update() 
	{
		GLUtils gl_utils_obj;
                cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
                config_parser = gl_utils_obj.get_config();
		std::string gfs_path = config_parser->osm_info_file_pathValue();
		OSDLOG(DEBUG, "osd global file update");
		boost::this_thread::sleep(boost::posix_time::seconds(6));
		boost::filesystem::remove(boost::filesystem::path(gfs_path + "osd_global_leader_information_-1"));
		osd_glb = gfs_path + "osd_global_leader_information_8";
                command = "touch " + osd_glb;
                std::system(command.c_str());	
		OSDLOG(DEBUG, "osd global leader info. 8");
		
	
	}
	TEST(ElectionManagerTest, ElectionManagerTest) 
	{
		Communication::SynchronousCommunication *comm = NULL;
		boost::shared_ptr<HfsChecker>hfs_checker;
		GLUtils gl_utils_obj;
		cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
		config_parser = gl_utils_obj.get_config();
		ServiceConfigReader service_config_reader;
		hfs_checker.reset(new HfsChecker(config_parser));		

                expt = config_parser->export_pathValue();
		osd_glb = expt + "/.osd_meta_config" + "/osd_global_leader_information_1.0";
                command = "touch " + osd_glb;
                std::system(command.c_str());	
		
	        mk_dir = expt + "/.osd_meta_config";

		std::string id = "proxy";
		boost::shared_ptr<ElectionManager> elec_mgr5;
		elec_mgr5.reset(new ElectionManager(id, config_parser, hfs_checker));
		int vl = elec_mgr5->start_election("HN0101");
		ASSERT_EQ(vl, -1);

		cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();

		std::string path = parser_ptr->osm_info_file_pathValue();
		boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
		boost::shared_ptr<gl_monitoring::MonitoringImpl> monitoring (new gl_monitoring::MonitoringImpl(whandler3));
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));

		bool val;
		std::list<boost::shared_ptr<recordStructure::Record> > record_list;
		record_list.push_back(nifrecord1);
		record_list.push_back(nifrecord2);
		val = whandler3->add_record(record_list);
		OSDLOG(DEBUG, "record added"<< val)
		ASSERT_EQ(val, true);
		boost::filesystem::remove(boost::filesystem::path(expt + "/.osd_meta_config" + "/osd_global_leader_information_1.0"));
		
		OsmUtils osm_utils;
		std::string lock_file;
		std::string gfs_path = config_parser->osm_info_file_pathValue();

		boost::filesystem::remove(boost::filesystem::path(gfs_path + "/LOCK_1.0-0"));
		
		osd_glb = expt + "/.osd_meta_config" + "/osd_global_leader_information_2.0";
                command = "touch " + osd_glb;
                std::system(command.c_str());	
		
		id = "account";
		boost::shared_ptr<ElectionManager> elec_mgr;
		elec_mgr.reset(new ElectionManager(id, config_parser, hfs_checker));
		
		int value;
		value = elec_mgr->start_election("HN0101");
		ASSERT_EQ(value, 98);

		std::string gl_version = "2.0";
		std::string election_version = gl_version + "-" + osm_utils.ItoStr(0);
		lock_file = gfs_path + "/LOCK_" + election_version;
		
		command = "touch " + lock_file;
		std::system(command.c_str());

		osd_glb = gfs_path + "osd_global_leader_information_3";
		command = "touch " + osd_glb;
		std::system(command.c_str());
		
		id = "HN0101_60123";
		boost::shared_ptr<ElectionManager> elec_mgr2;
		elec_mgr2.reset(new ElectionManager(id, config_parser, hfs_checker));
		value = elec_mgr2->start_election("HN0101");
		ASSERT_EQ(value, 98);

		boost::filesystem::remove(boost::filesystem::path(osd_glb.c_str()));

		osd_glb = gfs_path + "osd_global_leader_information_-1";
		command = "touch " + osd_glb;
		std::system(command.c_str());

		std::string lock_file_2 = gfs_path + "/LOCK_2.0-1";
		command = "touch " +  lock_file_2;
		std::system(command.c_str());
		boost::shared_ptr<ElectionManager> elec_mgr3;
		elec_mgr3.reset(new ElectionManager(id, config_parser, hfs_checker));
		value = elec_mgr3->start_election("HN0101");
		ASSERT_EQ(value, 98);

		boost::filesystem::remove(boost::filesystem::path(lock_file));
		boost::filesystem::remove(boost::filesystem::path(osd_glb));
		boost::filesystem::remove(boost::filesystem::path(lock_file_2));

		osd_glb = gfs_path + "osd_global_leader_information_4";
		command = "touch " + osd_glb;
		std::system(command.c_str());
		std::string gl_new_version = "gl_new_version";
		hfs_checker->stop();
		elec_mgr3.reset(new ElectionManager(id, config_parser, hfs_checker));
		value = elec_mgr3->start_election("HN0101");
		ASSERT_EQ(value, 98);

		boost::filesystem::remove(boost::filesystem::path(osd_glb.c_str()));

		osd_glb = gfs_path + "osd_global_leader_information_-1";
		command = "touch " + osd_glb;
		std::system(command.c_str());
		
		boost::shared_ptr<ElectionManager> elec_mgr4;
		hfs_checker->stop();
		boost::thread t1(osd_global_file_update);
		elec_mgr4.reset(new ElectionManager(id, config_parser, hfs_checker));
		value = elec_mgr3->start_election("HN0101");
		ASSERT_EQ(value, 98);
	
		boost::filesystem::remove(boost::filesystem::path(path + "/ELECT_FILE"));
		boost::filesystem::remove(boost::filesystem::path(osd_glb));

		id = "proxy";
		boost::shared_ptr<ElectionManager> elec_mgr6;
		hfs_checker->stop();
		elec_mgr6.reset(new ElectionManager(id, config_parser, hfs_checker));
		vl = elec_mgr6->start_election("HN0101");
		ASSERT_EQ(vl, -1);
			
		boost::filesystem::remove(boost::filesystem::path(path + "/node_info_file.info"));
		boost::filesystem::remove(boost::filesystem::path(expt + "/.osd_meta_config" + "/osd_global_leader_information_2.0"));
		
		command = "rmdir " + mk_dir;
                std::system(command.c_str());
			
		DIR *pdir;
		std::vector<std::string> v;
		std::string s;
		struct dirent *entry;		
		pdir = opendir(path.c_str());

		if(pdir) {
			while((entry = readdir(pdir)) != NULL) {
					if(entry->d_type != DT_DIR) {
						s = entry->d_name;
						if(s.find("LOCK_") != std::string::npos || s.find("ELECT_") != std::string::npos ||
							s.find("osd_global_leader_information") != std::string::npos) {
							v.push_back(s);
						}	
					}
			}
			closedir(pdir);
		}

		for(int i = 0; i < (int) v.size(); i++) {
			std::string  q = path + v[i];
			 boost::filesystem::remove(boost::filesystem::path(q));	
		}
			
	}
}
