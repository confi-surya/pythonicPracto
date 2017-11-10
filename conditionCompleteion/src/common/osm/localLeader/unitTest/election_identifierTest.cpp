#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>
#include "osm/osmServer/hfs_checker.h"
#include "osm/osmServer/service_config_reader.h"
#include "libraryUtils/object_storage_exception.h"
#include "libraryUtils/config_file_parser.h"
#include "osm/localLeader/election_manager.h"
#include "osm/localLeader/monitoring_manager.h"
#include "osm/localLeader/election_identifier.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include <fstream>
#include <vector>
#include <dirent.h>
#include <algorithm>

namespace ElectionIdentifierTest
{

	bool val;
	std::string expt;
	std::string mk_dir;	
	std::string osd_glb;
	std::string command;

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
		
	TEST(ElectionIdentifierTest, ElectionIdentifierTest)
	{
		cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr = get_config_parser();
		std::string path = parser_ptr->osm_info_file_pathValue();
		
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
							s.find("osd_global_leader_information") != std::string::npos || s.find("node_info_file") != std::string::npos) {
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
		
		cool::SharedPtr<config_file::OSMConfigFileParser>config_parser;
		GLUtils gl_utils_obj;
		config_parser = gl_utils_obj.get_config();
		expt = config_parser->export_pathValue();
		mk_dir = expt + "/.osd_meta_config";
	
		command = "mkdir " + mk_dir;
                std::system(command.c_str());

		osd_glb = expt + "/.osd_meta_config" + "/osd_global_leader_information_1.0";
                command = "touch " + osd_glb;
                std::system(command.c_str());
		
		boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
		boost::shared_ptr<gl_monitoring::MonitoringImpl> monitoring (new gl_monitoring::MonitoringImpl(whandler3));
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));

		std::list<boost::shared_ptr<recordStructure::Record> > record_list;
		record_list.push_back(nifrecord1);
		record_list.push_back(nifrecord2);
		val = whandler3->add_record(record_list);
		OSDLOG(INFO, "record added"<< val);
		ASSERT_EQ(val, true);
		enum network_messages::NodeStatusEnum stat = network_messages::NEW;
		boost::mutex mtx;
		boost::shared_ptr<HfsChecker> hfs_checker;
		hfs_checker.reset(new HfsChecker(config_parser));

		std::string id = "HN0101_60123";
		std::string gfs_path = config_parser->osm_info_file_pathValue();

		std::string infofile = gfs_path + "/osd_config" + "/osd_global_leader_information.info";
		ofstream fin;
		fin.open(infofile.c_str());

		fin << "hydra042_61014\t";
		fin << "192.168.123.13\t";
		fin << "61014";
  
		fin.close();  
		
		boost::shared_ptr<ElectionManager> elec_mgr;
		elec_mgr.reset(new ElectionManager(id, config_parser, hfs_checker));
	
		boost::shared_ptr<MonitoringManager> monitor_mgr;
        	monitor_mgr.reset(new MonitoringManager(hfs_checker, config_parser));
		
	        OSDLOG(DEBUG, "###########node info file"<< path);
		
		boost::shared_ptr<ElectionIdentifier> elec_idtf;
		hfs_checker->stop();
		elec_idtf.reset(new ElectionIdentifier(elec_mgr, hfs_checker, monitor_mgr, config_parser, stat, mtx));
	
		OsmUtils osm_utils;
		std::string lock_file;
		std::string gl = "1.0";
		std::string elect;


		std::string election_version = gl + "-" + osm_utils.ItoStr(0);
		lock_file = gfs_path + "/LOCK_" + election_version;

		command = "touch " + lock_file;
		std::system(command.c_str());

		elect = gfs_path + "/ELECT_FILE";
		command = "touch " + elect;
		std::system(command.c_str());

		osd_glb = gfs_path + "osd_global_leader_information_2";
		command = "touch " + osd_glb;
		std::system(command.c_str());
		
		boost::shared_ptr<boost::thread> eleThread;
                eleThread.reset(new boost::thread(boost::bind(&ElectionIdentifier::start, elec_idtf)));
		sleep(30);
		elec_idtf->stop_election_identifier();
                eleThread->join();

		boost::filesystem::remove(boost::filesystem::path(expt + "/.osd_meta_config" + "/osd_global_leader_information_1.0"));
		boost::filesystem::remove(boost::filesystem::path(path + "/osd_global_leader_information_2"));
		boost::filesystem::remove(boost::filesystem::path(path + "/node_info_file.info"));
		boost::filesystem::remove(boost::filesystem::path(lock_file));
		boost::filesystem::remove(boost::filesystem::path(elect));  
	}
	
}
