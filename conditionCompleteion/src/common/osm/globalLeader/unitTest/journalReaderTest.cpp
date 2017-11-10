#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "osm/globalLeader/journal_reader.h"
#include "osm/globalLeader/component_manager.h"

//using namespace std::tr1;
namespace JournalTest
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


cool::SharedPtr<config_file::ConfigFileParser> const get_journal_config_parser()
{
        cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
        if (osd_config)
		config_file_builder.readFromFile(std::string(osd_config).append("/gl_journal.conf"));
        else
                config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/gl_journal.conf");
        return config_file_builder.buildConfiguration();
}

TEST(GLJournalReadHandlerTest, RecoveryTest)
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
	std::string path = "/export/home/" + username;

	cool::SharedPtr<config_file::OSMConfigFileParser> comp_parser_ptr = get_config_parser();
	cool::SharedPtr<config_file::ConfigFileParser> journal_parser_ptr = get_journal_config_parser();
	boost::shared_ptr<component_manager::ComponentMapManager> map_mgr_ptr(new component_manager::ComponentMapManager(comp_parser_ptr));
	std::vector<std::string> failed_node_id;
	gl_journal_read_handler::GlobalLeaderRecovery rec_obj(path, map_mgr_ptr, journal_parser_ptr, failed_node_id);
	rec_obj.perform_recovery();
	ASSERT_EQ(1, int(map_mgr_ptr->get_transient_map().size()));
	
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > tmap = map_mgr_ptr->get_transient_map();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator itr_test = tmap.find(ACCOUNT_UPDATER);
	ASSERT_EQ(256, int( (std::tr1::get<0>(itr_test->second)).size()));
	ASSERT_EQ(0, (std::tr1::get<1>(itr_test->second))); //TODO: version should be 0.1
	/*
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator itttt = (std::tr1::get<0>(itr_test->second)).begin();
	std::cout << "Transient Map recovered from journal is:\n";
	for(; itttt != (std::tr1::get<0>(itr_test->second)).end(); ++itttt)
		std::cout << "(" << (*itttt).first << ", " << (*itttt).second.get_service_id() << ")" << std::endl;
	*/
	ASSERT_EQ(1, int(rec_obj.get_current_file_id()));
	ASSERT_EQ(2, int(rec_obj.get_next_file_id()));
	rec_obj.clean_journal_files();
	
	/*gl_journal_read_handler::GLJournalReadHandler read_hand(path, map_mgr_ptr, journal_parser_ptr);
	ASSERT_TRUE(read_hand.process_last_checkpoint(1)); //No Implementation
	ASSERT_TRUE(read_hand.process_snapshot()); //No Implementation
	ASSERT_TRUE(read_hand.process_recovering_offset()); //No Implementation
	*/

}

} //namespace JournalTest
