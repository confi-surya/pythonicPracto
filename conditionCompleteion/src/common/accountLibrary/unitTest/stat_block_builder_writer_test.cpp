#include "gtest/gtest.h"

#include "accountLibrary/stat_block_builder.h"
#include "accountLibrary/stat_block_writer.h"
#include "accountLibrary/record_structure.h"

namespace statBlockBuilderTest
{

TEST(StatBlockBuilderTest, BuildFromValueTest)
{

	
	accountInfoFile::StatBlockBuilder builder;
	boost::shared_ptr<recordStructure::AccountStat> stat = builder.create_block_from_content("account1", 1000, 1001, 2000,
							 5, 10, 1024, "ABC123", "A-B-C-123", "DELETED", 1111, "lang=English");

        ASSERT_EQ(uint64_t(1000), stat->get_created_at());
        ASSERT_EQ("account1", stat->get_account());
        ASSERT_EQ("lang=English", stat->get_metadata());
}


TEST(StatBlockBuilderTest, builedFromBufferTest)
{
        
        std::string account("account1");
        uint64_t created_at=1000;
        uint64_t put_timestamp=1001;
        uint64_t delete_timestamp=2000;
        uint64_t container_count=5;
        uint64_t object_count=10;
        uint64_t bytes_used=1024;
        std::string hash("ABC123");
        std::string id("A-B-C-123");
        std::string status("");
        uint64_t status_changed_at=1111;
        std::string metadata("lang=English");

	boost::shared_ptr<recordStructure::AccountStat> stat(new recordStructure::AccountStat(
								account, created_at, put_timestamp, delete_timestamp,
								container_count, object_count, bytes_used, hash, id,
								status, status_changed_at, metadata));

	boost::shared_ptr<accountInfoFile::StatBlockWriter> stat_block_writer(new accountInfoFile::StatBlockWriter);
	boost::shared_ptr<accountInfoFile::StatBlockBuilder> stat_block_reader(new accountInfoFile::StatBlockBuilder);

	accountInfoFile::block_buffer_size_tuple stat_tuple = stat_block_writer->get_writable_block(stat);

	boost::shared_ptr<recordStructure::AccountStat> account_stat = stat_block_reader->create_block_from_buffer(stat_tuple.get<0>());
	
	ASSERT_EQ("account1", account_stat->get_account());
	ASSERT_EQ(uint64_t(1000), account_stat->get_created_at());
	ASSERT_EQ(uint64_t(1001), account_stat->get_put_timestamp());
	ASSERT_EQ(uint64_t(2000), account_stat->get_delete_timestamp());
	ASSERT_EQ(uint64_t(5), account_stat->get_container_count());
	
	delete stat_tuple.get<0>();
	
}

}

