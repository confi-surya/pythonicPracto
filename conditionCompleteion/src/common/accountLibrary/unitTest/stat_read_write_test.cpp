#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "accountLibrary/file.h"
#include "accountLibrary/reader.h"
#include "accountLibrary/writer.h"


namespace readerTest
{

TEST(readerTest, writeAndReadTest)
{
        std::string testFile("Stat_test.infoFile");

        std::ofstream fileTmp;
        fileTmp.open(testFile.c_str(),  std::ios::out | std::ios::binary);
	fileTmp.close();

        boost::shared_ptr<accountInfoFile::TrailerBlock> trailer(new accountInfoFile::TrailerBlock());

        accountInfoFile::StatBlockBuilder builder;

 	boost::shared_ptr<recordStructure::AccountStat> account_stat_block = builder.create_block_from_content("account_name_ABC",
                                                1000, 1001, 1002, 5, 10, 1024, "ABC123", "A-B-C-123", "DELETED", 1111, "lang=English");

        boost::shared_ptr<accountInfoFile::FileOperation> file(new accountInfoFile::FileOperation(testFile));

        boost::shared_ptr<accountInfoFile::StatBlockBuilder> statBuilder(new accountInfoFile::StatBlockBuilder);
        boost::shared_ptr<accountInfoFile::StatBlockWriter> statWriter(new accountInfoFile::StatBlockWriter);

        boost::shared_ptr<accountInfoFile::TrailerBlockBuilder> trailerBuilder(new accountInfoFile::TrailerBlockBuilder);
        boost::shared_ptr<accountInfoFile::TrailerBlockWriter> trailerWriter(new accountInfoFile::TrailerBlockWriter);

        accountInfoFile::Writer writer(file, statWriter, trailerWriter, trailerBuilder);
        accountInfoFile::Reader reader(file, statBuilder, trailerBuilder);

        writer.writeFile(account_stat_block, trailer);
   
	boost::shared_ptr<recordStructure::AccountStat> statSecond = reader.read_stat_block();

        EXPECT_STREQ("account_name_ABC", statSecond->account.c_str());
        EXPECT_EQ(uint64_t(1000), statSecond->created_at);
        ASSERT_EQ(uint64_t(1001), statSecond->put_timestamp);
        ASSERT_EQ(uint64_t(1002), statSecond->delete_timestamp);
        ASSERT_EQ(uint64_t(5), statSecond->container_count);
        ASSERT_EQ(uint64_t(10), statSecond->object_count);
        ASSERT_EQ(uint64_t(1024), statSecond->bytes_used);
        ASSERT_STREQ("ABC123", statSecond->hash.c_str());
        ASSERT_STREQ("A-B-C-123", statSecond->id.c_str());
        ASSERT_STREQ("DELETED", statSecond->status.c_str());
        ASSERT_EQ(uint64_t(1111), statSecond->status_changed_at);
        ASSERT_STREQ("lang=English", statSecond->metadata.c_str());

        EXPECT_TRUE(boost::filesystem::exists(boost::filesystem::path(testFile)));
        boost::filesystem::remove(boost::filesystem::path(testFile));
}

}
