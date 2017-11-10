#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>

#include "libraryUtils/file_handler.h"
#include "accountLibrary/account_info_file_manager.h"
#include "accountLibrary/record_structure.h"

namespace setTest
{

TEST(addContianerTest, addContainerTest)
{
	std::string path(boost::filesystem::current_path().string());
	boost::shared_ptr<accountInfoFile::AccountInfoFileManager> a_file_man(new accountInfoFile::AccountInfoFileManager());


	boost::shared_ptr<recordStructure::AccountStat> account_stat(new recordStructure::AccountStat());

	account_stat->account = "myaccount";
	account_stat->created_at = 1000;
	account_stat->put_timestamp = 1001;
	account_stat->delete_timestamp = 1002;
	account_stat->container_count = 5;
	account_stat->object_count = 10;
	account_stat->bytes_used = 1024;
	account_stat->hash = "ABCD";
	account_stat->id = "A-B-C-D";
	account_stat->status = "DELETED";
	account_stat->status_changed_at = 2000;
	account_stat->metadata = "lang=English";
	
	a_file_man->create_account("Service_1234", path, account_stat);

	boost::shared_ptr<recordStructure::AccountStat> a_stat = a_file_man->get_account_stat(path);

	EXPECT_EQ(account_stat->account, a_stat->account);
	EXPECT_EQ(account_stat->created_at, a_stat->created_at);
	EXPECT_EQ(account_stat->put_timestamp, a_stat->put_timestamp);
	EXPECT_EQ(account_stat->delete_timestamp, a_stat->delete_timestamp);
	EXPECT_EQ(account_stat->container_count, a_stat->container_count);
	EXPECT_EQ(account_stat->object_count, a_stat->object_count);
	EXPECT_EQ(account_stat->bytes_used, a_stat->bytes_used);
	EXPECT_EQ(account_stat->hash, a_stat->hash);
	EXPECT_EQ(account_stat->id, a_stat->id);
	EXPECT_EQ(account_stat->status, a_stat->status);
	EXPECT_EQ(account_stat->status_changed_at, a_stat->status_changed_at);
	EXPECT_EQ("lang=English", a_stat->metadata);


	boost::shared_ptr<recordStructure::ContainerRecord> container_record_1(new recordStructure::ContainerRecord(1, "my_container_111111111111111110", 10, 20, 30, 40, false));
	boost::shared_ptr<recordStructure::ContainerRecord> container_record_2(new recordStructure::ContainerRecord(10, "my_container_2", 100, 200, 300, 400, false));
	boost::shared_ptr<recordStructure::ContainerRecord> container_record_3(new recordStructure::ContainerRecord(10, "my_container_2", 1000, 2000, 3000, 4000, false));

	a_file_man->add_container(path, container_record_1);
	//a_file_man->add_container(path, container_record_2);
	a_file_man->add_container(path, container_record_3);

	

        boost::shared_ptr<fileHandler::FileHandler> file(new fileHandler::FileHandler("objectStorage.infoFile", fileHandler::OPEN_FOR_READ));
        boost::shared_ptr<accountInfoFile::StaticDataBlockBuilder> static_data_block_reader(new accountInfoFile::StaticDataBlockBuilder);
        boost::shared_ptr<accountInfoFile::NonStaticDataBlockBuilder> non_static_data_block_reader(new accountInfoFile::NonStaticDataBlockBuilder);
        boost::shared_ptr<accountInfoFile::TrailerStatBlockBuilder> stat_trailer_reader(new accountInfoFile::TrailerStatBlockBuilder);
	
	accountInfoFile::Reader reader(file, static_data_block_reader, non_static_data_block_reader, stat_trailer_reader);

	boost::hash<std::string> container_hash1;
	boost::hash<std::string> container_hash2;

	std::string hash1(boost::lexical_cast<std::string>(container_hash1("my_container_111111111111111110")));
	std::string hash2(boost::lexical_cast<std::string>(container_hash2("my_container_2")));

        // testing : non-static data
        boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block_2 = reader.read_non_static_data_block(0,2);
        EXPECT_TRUE(non_static_data_block_2->exists(hash1));
        EXPECT_TRUE(non_static_data_block_2->exists(hash2));

	ASSERT_EQ(uint64_t(2), non_static_data_block_2->get_size());

	std::vector<accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord>::iterator nsdb_it = non_static_data_block_2->get_iterator();
	EXPECT_EQ(hash1, nsdb_it->hash);
	EXPECT_EQ(uint64_t(10), nsdb_it->put_timestamp);
	EXPECT_EQ(uint64_t(20), nsdb_it->delete_timestamp);
	EXPECT_EQ(uint64_t(30), nsdb_it->object_count);
	EXPECT_EQ(uint64_t(40), nsdb_it->bytes_used);
	EXPECT_EQ(false, nsdb_it->deleted);

	nsdb_it++;

	EXPECT_EQ(hash2, nsdb_it->hash);
        EXPECT_EQ(uint64_t(1000), nsdb_it->put_timestamp);
        EXPECT_EQ(uint64_t(2000), nsdb_it->delete_timestamp);
        EXPECT_EQ(uint64_t(3000), nsdb_it->object_count);
        EXPECT_EQ(uint64_t(4000), nsdb_it->bytes_used);
        EXPECT_EQ(false, nsdb_it->deleted);
	
	/*  boost::shared_ptr<accountInfoFile::StaticDataBlock> static_data_block_2 = reader.read_static_data_block(1,2);
        accountInfoFile::StaticDataBlock::StaticDataBlockRecord stbr_test_1(static_data_block_2->get_static_data_record_by_index(0));
        ASSERT_EQ(uint64_t(1), stbr_test_1.ROWID);
        ASSERT_EQ("my_container_111111111111111110", stbr_test_1.name);


        accountInfoFile::StaticDataBlock::StaticDataBlockRecord stbr_test_2(static_data_block_2->get_static_data_record_by_index(1));
        ASSERT_EQ(uint64_t(10), stbr_test_2.ROWID);
        ASSERT_EQ("my_container_2", stbr_test_2.name);*/
	
	a_file_man->delete_account(path);
}

}
	
	
