#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "osm/globalLeader/journal_reader.h"
#include "osm/globalLeader/journal_library.h"
#include "osm/globalLeader/state_change_initiator.h"
#include "osm/globalLeader/component_manager.h"

//using namespace std::tr1;
namespace GLJournalTest
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


/*cool::SharedPtr<config_file::ConfigFileParser> const get_journal_config_parser()
{
        cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
        if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/osm1.conf"));
        else
                config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/osm1.conf");
        return config_file_builder.buildConfiguration();
}
*/

std::vector<std::string> get_state_change_ids()
{
	std::vector<std::string> return_list;
	return_list.push_back("HN0101_60123_account-updater-server");
	return_list.push_back("HN0102_60123_account-updater-server");
	return return_list;
}

void done()
{

}

TEST(JournalLibraryTest, JournalLibraryTestprepare_transient_record_for_journalTest)
{
	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while ( !feof(proc) && fgets(buf,sizeof(buf),proc) )
	{
		//std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	std::string journal_path = "/export/home/" + username;
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));

	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord3(new recordStructure::NodeInfoFileRecord("HN0103_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord4(new recordStructure::NodeInfoFileRecord("HN0104_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	std::list<boost::shared_ptr<recordStructure::Record> > record_list;
	record_list.push_back(nifrecord1);
	record_list.push_back(nifrecord2);
	record_list.push_back(nifrecord3);
	record_list.push_back(nifrecord4);
	whandler3->add_record(record_list);
	

	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));
	boost::shared_ptr<global_leader::JournalLibrary> journal_lib_ptr(new global_leader::JournalLibrary(
							journal_path, 
							//parser_ptr,
							map_manager,
							boost::bind(&get_state_change_ids)
							));
        boost::shared_ptr<component_balancer::ComponentBalancer> comp_balancer (new component_balancer::ComponentBalancer(map_manager, parser_ptr));
        std::vector<std::string> node_add;
        std::vector<std::string> node_del;
        std::vector<std::string> node_rec;
	node_add.clear();
        node_del.clear();
        node_rec.clear();
        node_add.push_back("HN0101_60123_account-updater-server");
        std::string accup_serv_name = "account-updater-server";
        comp_balancer->get_new_planned_map(accup_serv_name, node_add, node_del, node_rec);
        node_add.push_back("HN0102_60123_account-updater-server");
	std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> new_planned_map_AU_pair;
	new_planned_map_AU_pair = comp_balancer->get_new_planned_map(accup_serv_name, node_add, node_del, node_rec);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map_AU = new_planned_map_AU_pair.first;
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map_AU = new_planned_map_AU.find("HN0101_60123_account-updater-server");
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2_AU;
	arg2_AU.clear();
	arg2_AU = it_internal_planned_map_AU->second;
	map_manager->update_transient_map(accup_serv_name, arg2_AU);
	std::cout << "Global Map version is: " << map_manager->get_global_map_version() << std::endl;
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > t_map = map_manager->get_transient_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator it_t_map = t_map.find("account-updater-server");
	if (it_t_map != t_map.end())
	{
		//cout<<"=--=-==-=="<<std::tr1::get<1>(it_t_map->second)<<std::endl;
		ASSERT_FLOAT_EQ( 0.1, std::tr1::get<1>(it_t_map->second));
	}
	ASSERT_EQ(1, int(t_map.size())); 
	
	std::vector<boost::shared_ptr<recordStructure::Record> > node_rec_records;
	journal_lib_ptr->prepare_transient_record_for_journal(accup_serv_name, node_rec_records);
	boost::shared_ptr<recordStructure::Record> mount_rec(
		new recordStructure::GLMountInfoRecord("HN0101_CS", "HN0102")
		);
	node_rec_records.push_back(mount_rec);
	ASSERT_EQ(2, int(node_rec_records.size()));
	
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/journal.log.1"));
	journal_lib_ptr->add_entry_in_journal(node_rec_records, boost::bind(&done));

	journal_lib_ptr->get_journal_writer();


	std::string path = "/export/home/" + username;

	GLUtils util_obj;
	cool::SharedPtr<config_file::OSMConfigFileParser> comp_parser_ptr = util_obj.get_config();
	cool::SharedPtr<config_file::ConfigFileParser> journal_parser_ptr = util_obj.get_journal_config();
	boost::shared_ptr<component_manager::ComponentMapManager> map_mgr_ptr(new component_manager::ComponentMapManager(comp_parser_ptr));
	std::vector<std::string> failed_nodes;
	gl_journal_read_handler::GlobalLeaderRecovery rec_obj(path, map_mgr_ptr, journal_parser_ptr, failed_nodes);
	rec_obj.perform_recovery();

	ASSERT_FLOAT_EQ(0.1, map_mgr_ptr->get_transient_map_version());
	std::cout << "Global Map version is: " << map_mgr_ptr->get_global_map_version() << std::endl;
	map_mgr_ptr->recover_from_component_info_file();
	map_mgr_ptr->merge_basic_and_transient_map();

	//ASSERT_FLOAT_EQ(4.0, map_mgr_ptr->get_global_map_version());

	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/accountUpdater_component_info_file.info"));
}

} //namespace JournalTest
