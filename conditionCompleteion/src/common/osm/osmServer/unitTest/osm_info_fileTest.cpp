#define GTEST_USE_OWN_TR1_TUPLE 0 
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "libraryUtils/config_file_parser.h"
#include "osm/osmServer/osm_info_file.h"
#include "osm/osmServer/ring_file.h"

namespace OsmInfoFileTest
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

std::string service_name = "container-server";

/*std::string command = "whoami";
FILE *proc = popen(command.c_str(),"r");
char buf[1024];
while(!feof(proc) && fgets(buf,sizeof(buf),proc))
{
	std::cout << buf << std::endl;
}
fgets(buf,sizeof(buf),proc);
std::string username(buf);
username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
std::cout<<username<<std::endl;
*/

TEST(OsmInfoFileTest, NodeinfoFileWriter_Initialize_Test)
{
	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while(!feof(proc) && fgets(buf,sizeof(buf),proc))
	{
	        std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	std::cout<<username<<std::endl;

	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler1( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler1->init());
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler2( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));

	ASSERT_TRUE(whandler2->init(osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/account_component_info_file.info"));
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler3( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler3->init(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/container_component_info_file.info"));
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler4( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler4->init(osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/accountUpdater_component_info_file.info"));
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler5( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler5->init(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/object_component_info_file.info"));
	//ASSERT_FALSE(whandler2->init(11));
}


TEST(OsmInfoFileTest, NodeInfoFileWriter_AddRecord_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	boost::shared_ptr<gl_monitoring::MonitoringImpl> monitoring (new gl_monitoring::MonitoringImpl(whandler3));

	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED));

	std::list<boost::shared_ptr<recordStructure::Record> > record_list;
	record_list.push_back(nifrecord1);
	record_list.push_back(nifrecord2);
	ASSERT_TRUE(whandler3->add_record(record_list));
}	

TEST(OsmInfoFileTest, NodeInfoFileWriter_RecoverRecord_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3( new osm_info_file_manager::NodeInfoFileReadHandler(path));
	ASSERT_TRUE(rhandler3->recover_record());
	
}

TEST(OsmInfoFileTest, NodeInfoFileWriter_UpdateRecord_Test)
{
	std::string existing_node_id = "HN0101_60123";
	std::string nonexisting_node_id = "HN0701_60123";
	int updated_node_status = gl_monitoring::FAILED;
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler3->update_record(existing_node_id, updated_node_status));
	ASSERT_FALSE(whandler3->update_record(nonexisting_node_id, updated_node_status));
}

TEST(OsmInfoFileTest, NodeInfoFileWriter_DeleteRecord_Test)
{
	std::string existing_node_id = "HN0102_60123";
	std::string nonexisting_node_id = "HN0701_60123";
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler3( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler3->delete_record(existing_node_id));
	ASSERT_FALSE(whandler3->delete_record(nonexisting_node_id));

}

TEST(OsmInfoFileTest, NodeInfoFileReader_ListNodeIds_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3( new osm_info_file_manager::NodeInfoFileReadHandler(path));
	std::list<std::string> list_of_node_ids_received = rhandler3->list_node_ids();
	ASSERT_EQ(1, int (list_of_node_ids_received.size()));
	//ASSERT_EQ(list_of_node_ids_received.front(), list_of_node_ids_received.back()); 
}

TEST(OsmInfoFileTest, NodeInfoFileReader_Get_Record_From_Id_Test)
{
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler3( new osm_info_file_manager::NodeInfoFileReadHandler(path));
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd(new recordStructure::NodeInfoFileRecord);
	node_info_file_rcrd = rhandler3->get_record_from_id("HN0101_60123");
	if (node_info_file_rcrd)
	{
	ASSERT_EQ("HN0101_60123", node_info_file_rcrd->get_node_id());
	ASSERT_EQ("192.168.131.123", node_info_file_rcrd->get_node_ip());
	ASSERT_EQ(60123, node_info_file_rcrd->get_localLeader_port());
	ASSERT_EQ(61006, node_info_file_rcrd->get_accountService_port());
	ASSERT_EQ(61007, node_info_file_rcrd->get_containerService_port());
	ASSERT_EQ(61008, node_info_file_rcrd->get_objectService_port());
	ASSERT_EQ(61009, node_info_file_rcrd->get_accountUpdaterService_port());
	ASSERT_EQ(gl_monitoring::FAILED, node_info_file_rcrd->get_node_status());
	}
	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while(!feof(proc) && fgets(buf,sizeof(buf),proc))
	{
	        std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	std::cout<<username<<std::endl;
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
}

TEST(OsmInfoFileTest, RingFile_remakering_Test)
{
	boost::shared_ptr<ring_file_manager::RingFileWriteHandler> whandler(new ring_file_manager::RingFileWriteHandler(path));
	ASSERT_TRUE(whandler->remakering());
}

TEST(OsmInfoFileTest, RingFile_get_fs_list_Test)
{
	boost::shared_ptr<ring_file_manager::RingFileReadHandler> rhandler(new ring_file_manager::RingFileReadHandler(path));
	std::vector<std::string> fs_list = rhandler->get_fs_list();
	ASSERT_EQ(10, int (fs_list.size()));
}

TEST(OsmInfoFileTest, RingFile_get_account_level_dir_list_Test)
{
	boost::shared_ptr<ring_file_manager::RingFileReadHandler> rhandler(new ring_file_manager::RingFileReadHandler(path));
	std::vector<std::string> account_level_dir_list = rhandler->get_account_level_dir_list();
	ASSERT_EQ(10, int (account_level_dir_list.size()));
}

TEST(OsmInfoFileTest, RingFile_get_container_level_dir_list_Test)
{
	boost::shared_ptr<ring_file_manager::RingFileReadHandler> rhandler(new ring_file_manager::RingFileReadHandler(path));
	std::vector<std::string> container_level_dir_list = rhandler->get_container_level_dir_list();
	ASSERT_EQ(10, int (container_level_dir_list.size()));
}

TEST(OsmInfoFileTest, RingFile_get_object_level_dir_list_Test)
{
	boost::shared_ptr<ring_file_manager::RingFileReadHandler> rhandler(new ring_file_manager::RingFileReadHandler(path));
	std::vector<std::string> object_level_dir_list = rhandler->get_object_level_dir_list();
	ASSERT_EQ(20, int (object_level_dir_list.size()));
}

TEST(OsmInfoFileTest, ComponentInfoFile_Add_record_Test)
{
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord1(new recordStructure::ComponentInfoRecord("HN0101_60123_account-server", "192.168.131.2", 61006));	
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord2(new recordStructure::ComponentInfoRecord("HN0102_60123_container-server", "192.168.131.12", 61007));
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord3(new recordStructure::ComponentInfoRecord("HN0103_60123_accountUpdater-server", "192.168.131.32", 61009));
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord4(new recordStructure::ComponentInfoRecord("HN0104_60123_object-server", "192.168.131.32", 61008));
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord5(new recordStructure::ComponentInfoRecord("HN0105_60123_object-server", "192.168.131.32", 61008));
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord6(new recordStructure::ComponentInfoRecord("HN0106_60123_object-server", "192.168.131.32", 61008));
        std::vector<recordStructure::ComponentInfoRecord> cifr_list1;
        cifr_list1.clear();
//        cifr_list1.push_back(*cifrecord1);
//        cifr_list2.push_back(*cifrecord2);
//        cifr_list3.push_back(*cifrecord3);
	for (int i=0; i<512; i++)
	{
        	cifr_list1.push_back(*cifrecord1);
	}
        std::vector<recordStructure::ComponentInfoRecord> cifr_list2;
        cifr_list2.clear();
	for (int i=0; i<512; i++)
	{
        	cifr_list2.push_back(*cifrecord2);
	}
        std::vector<recordStructure::ComponentInfoRecord> cifr_list3;
        cifr_list3.clear();
	for (int i=0; i<512; i++)
	{
        	cifr_list3.push_back(*cifrecord3);
	}

	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record1( new recordStructure::CompleteComponentRecord(cifr_list1, 0.11, 11));
        boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record2( new recordStructure::CompleteComponentRecord(cifr_list2, 0.11, 11));
        boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record3( new recordStructure::CompleteComponentRecord(cifr_list3, 0.11, 11));

	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler1( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler1->add_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE, complete_record2));
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler2( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler2->add_record(osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE, complete_record1));
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler3( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler3->add_record(osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE, complete_record3));
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler4( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_TRUE(whandler4->add_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, cifrecord4));
	//ASSERT_TRUE(whandler4->add_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, cifrecord5));
	//ASSERT_TRUE(whandler4->add_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, cifrecord6));
}

TEST(OsmInfoFileTest, ComponentInfoFile_recover_record_Test)
{
	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record_1(new recordStructure::CompleteComponentRecord);
        boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler1( new osm_info_file_manager::ComponentInfoFileReadHandler(path));
	//ASSERT_TRUE(rhandler1->recover_record(osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE));
	ASSERT_TRUE(rhandler1->recover_distributed_service_record(osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE, complete_record_1));
	std::cout<<"Account GMap Version "<<complete_record_1->get_service_version()<<std::endl;
	ASSERT_FLOAT_EQ(0.11, complete_record_1->get_service_version());
	ASSERT_EQ(11, complete_record_1->get_gl_version());

	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record_2(new recordStructure::CompleteComponentRecord);
        boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler2(new osm_info_file_manager::ComponentInfoFileReadHandler(path));
	//ASSERT_TRUE(rhandler2->recover_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE));
	ASSERT_TRUE(rhandler1->recover_distributed_service_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE, complete_record_2));
	std::cout<<"Container GMap Version "<<complete_record_2->get_service_version()<<std::endl;
	ASSERT_FLOAT_EQ(0.11, complete_record_2->get_service_version());
	ASSERT_EQ(11, complete_record_2->get_gl_version());

	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record_3(new recordStructure::CompleteComponentRecord);
        boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler3(new osm_info_file_manager::ComponentInfoFileReadHandler(path));
	//ASSERT_TRUE(rhandler3->recover_record(osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE));
	ASSERT_TRUE(rhandler1->recover_distributed_service_record(osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE, complete_record_3));
	std::cout<<"AccountUpdater GMap Version "<<complete_record_3->get_service_version()<<std::endl;
	ASSERT_FLOAT_EQ(0.11, complete_record_3->get_service_version());
	ASSERT_EQ(11, complete_record_3->get_gl_version());

	std::vector<recordStructure::ComponentInfoRecord> comp_record_list;
        boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler4(new osm_info_file_manager::ComponentInfoFileReadHandler(path));
        //ASSERT_TRUE(rhandler4->recover_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE));
	ASSERT_TRUE(rhandler4->recover_object_service_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, comp_record_list));
	ASSERT_EQ(1, int(comp_record_list.size()));
}

TEST(OsmInfoFileTest, ComponentInfoFile_Update_Record_Test)
{
        boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler1( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord5(new recordStructure::ComponentInfoRecord("HN0105_60123_object-server", "192.168.131.123", 61008));
	ASSERT_TRUE(whandler1->update_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, cifrecord5));
	ASSERT_FALSE(whandler1->update_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE, cifrecord5));
}

TEST(OsmInfoFileTest, ComponentInfoFile_delete_record_Test)
{
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler1( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	ASSERT_FALSE(whandler1->delete_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, "HN0100_60123_object-server"));
	//ASSERT_TRUE(whandler1->delete_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, "HN0106_60123_object-server"));
	ASSERT_FALSE(whandler1->delete_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE, "HN0106_60123_object-server"));
	//ASSERT_TRUE(whandler1->delete_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, "HN0105_60123_object-server"));
	ASSERT_TRUE(whandler1->delete_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, "HN0104_60123_object-server"));
	
	std::vector<recordStructure::ComponentInfoRecord> comp_record_list;
        boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler4(new osm_info_file_manager::ComponentInfoFileReadHandler(path));
        //ASSERT_TRUE(rhandler4->recover_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE));
	ASSERT_TRUE(rhandler4->recover_object_service_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, comp_record_list));
	ASSERT_EQ(0, int(comp_record_list.size()));

	std::string command = "whoami";
	FILE *proc = popen(command.c_str(),"r");
	char buf[1024];
	while(!feof(proc) && fgets(buf,sizeof(buf),proc))
	{
	        std::cout << buf << std::endl;
	}
	std::string username(buf);
	username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
	std::cout<<username<<std::endl;
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/object_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/node_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/account_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/container_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/accountUpdater_component_info_file.info"));
	boost::filesystem::remove(boost::filesystem::path("/export/home/" + username + "/ring_file.ring"));
}
} //namespace OsmInfoFileTest
