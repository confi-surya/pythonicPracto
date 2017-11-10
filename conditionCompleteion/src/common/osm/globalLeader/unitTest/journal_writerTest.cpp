#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "osm/globalLeader/journal_reader.h"
#include "osm/globalLeader/journal_writer.h"
#include "osm/globalLeader/journal_library.h"
#include "osm/globalLeader/state_change_initiator.h"
#include "osm/globalLeader/component_manager.h"

//using namespace std::tr1;
namespace GLJournalWriteHandlerTest
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

cool::SharedPtr<config_file::ConfigFileParser> journal_parser_ptr = get_journal_config_parser();


std::vector<std::string> get_state_change_ids()
{
	std::vector<std::string> return_list;
	return_list.push_back("HN0101_60123_account-updater-server");
	return_list.push_back("HN0102_60123_account-updater-server");
	return return_list;
}

TEST(GLJournalWriteHandlerTest, GLJournalWriteHandler_process_Test)
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


	boost::shared_ptr<component_manager::ComponentMapManager> map_manager (new component_manager::ComponentMapManager(parser_ptr));

	boost::shared_ptr<journal_manager::GLJournalWriteHandler> journal_writer(new journal_manager::GLJournalWriteHandler(
					journal_path,
					journal_parser_ptr,
					map_manager,
					boost::bind(&get_state_change_ids)
					));
	journal_writer->process_snapshot();
	journal_writer->remove_all_journal_files();
	journal_writer->remove_old_files();
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/journal/log.1"));
}

} //namespace JournalTest
