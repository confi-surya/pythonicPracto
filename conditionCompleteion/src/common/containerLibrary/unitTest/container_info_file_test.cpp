#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

//#include "containerLibrary/stat_block.h"
#include "containerLibrary/containerInfoFile_manager.h"

namespace containerManagerTest
{



TEST(ContainerManagerTest, ddContainerTest)
{
	std::string account("account1");
        std::string container("container1");
        uint64_t created_at=1000;
        uint64_t put_timestamp=1001;
        uint64_t delete_timestamp=2000;
        int64_t object_count=1;
        uint64_t bytes_used=1024;
	std::string hash("ABC123");
	std::string id("A-B-C-123");
	std::string status("");
        uint64_t status_changed_at=1111;
	std::string metadata("lang=English");

        recordStructure::ContainerStat stat(account, container, created_at, put_timestamp, delete_timestamp, \
                        object_count, bytes_used, hash, id, status, status_changed_at, metadata);

	boost::filesystem::create_directories(boost::filesystem::path("account/container"));
	containerInfoFile::ContainerInfoFileBuilder container_infoFile_builder;

	boost::shared_ptr<containerInfoFile::ContainerInfoFile> container_create =
                                container_infoFile_builder.create_container_for_create("account/container/container.info");
	container_create->add_container(stat);

        ASSERT_EQ(int64_t(1000), stat.created_at);
	ASSERT_STREQ("account1", stat.account.c_str());
	ASSERT_STREQ("lang=English", stat.metadata.c_str());

	typedef std::list<std::pair<recordStructure::JournalOffset, recordStructure::ObjectRecord> > RecordList;
	recordStructure::JournalOffset off(0, 0);
	RecordList data;

	recordStructure::ObjectRecord record(1234, "naam-1", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record2(1234, "naam-do", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record1(1234, "naam-ek", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record7(1234, "naam-ek-1", 1230, 12345, "saman", "anda");

	data.push_back(std::make_pair(off, record));
        data.push_back(std::make_pair(off, record2));
        data.push_back(std::make_pair(off, record1));
        data.push_back(std::make_pair(off, record7));


	ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path("account/container/container.info")));

	boost::shared_ptr<containerInfoFile::ContainerInfoFile> container_refrence = 
				container_infoFile_builder.create_container_for_reference("account/container/container.info");

	boost::shared_ptr<recordStructure::ContainerStat> stat_second = container_refrence->get_container_stat();

	ASSERT_EQ(int64_t(1000), stat_second->created_at);
        ASSERT_STREQ("account1", stat_second->account.c_str());
        ASSERT_STREQ("lang=English", stat_second->metadata.c_str());

	recordStructure::ContainerStat stat_third(account, container, 1230, put_timestamp, delete_timestamp, \
                        object_count, bytes_used, hash, id, status, status_changed_at, metadata);

	boost::shared_ptr<containerInfoFile::ContainerInfoFile> container_modify =
			container_infoFile_builder.create_container_for_modify("account/container/container.info");

	container_modify->set_container_stat(stat_third);


	boost::shared_ptr<recordStructure::ContainerStat> stat_fifth = container_refrence->get_container_stat();
	ASSERT_EQ(int64_t(1230), stat_fifth->created_at);
        ASSERT_STREQ("account1", stat_fifth->account.c_str());
        ASSERT_STREQ("lang=English", stat_fifth->metadata.c_str());

	container_modify->flush_container(&data);

	RecordList data_second;

        recordStructure::ObjectRecord record3(1234, "naam-teen", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record4(1234, "naam-char", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record5(1234, "naam-paanch", 1230, 12345, "saman", "anda");

        data_second.push_back(std::make_pair(off, record3));
        data_second.push_back(std::make_pair(off, record4));
        data_second.push_back(std::make_pair(off, record5));


        container_modify->flush_container(&data_second);

	boost::shared_ptr<containerInfoFile::ContainerInfoFile> container_refrence_second = 
				container_infoFile_builder.create_container_for_reference("account/container/container.info");

	boost::shared_ptr<std::vector<recordStructure::ObjectRecord> > records_m(new std::vector<recordStructure::ObjectRecord>()); 
	std::list<recordStructure::ObjectRecord> records;
	container_refrence_second->get_object_list(records, records_m);

	std::list<recordStructure::ObjectRecord>::iterator it  = records.begin();
	ASSERT_STREQ(it->name.c_str(), "naam-1"); it++;
	ASSERT_STREQ(it->name.c_str(), "naam-char"); it++;
	ASSERT_STREQ(it->name.c_str(), "naam-do"); it++;
	ASSERT_STREQ(it->name.c_str(), "naam-ek"); it++;
	ASSERT_STREQ(it->name.c_str(), "naam-ek-1"); it++;
	ASSERT_STREQ(it->name.c_str(), "naam-paanch");it++;
	ASSERT_STREQ(it->name.c_str(), "naam-teen"); 
	boost::filesystem::remove_all(boost::filesystem::path("account"));

} 



} 
