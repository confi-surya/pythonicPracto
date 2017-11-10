#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "accountLibrary/fileBottomReaderWriter.h"
#include "accountLibrary/reader.h"
#include "accountLibrary/writer.h"

namespace readerTest
{

TEST(readerTest, writeAndReadTest)
{
        std::string testFile("writeAndReadTest.file");

        std::ofstream fileTmp;
        fileTmp.open(testFile.c_str(),  std::ios::out | std::ios::binary);
        char *buff = new char[10247];
        fileTmp.write(buff, 10247);
        fileTmp.close();

        boost::shared_ptr<accountInfoFile::Trailer> trailer(new accountInfoFile::Trailer(300, 200, 100));

        // accout_stat data
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

        boost::shared_ptr<accountInfoFile::StatBlock> stat(new accountInfoFile::StatBlock(account,
                        created_at, put_timestamp, delete_timestamp, container_count, 
                        object_count, bytes_used, hash, id, status, status_changed_at, metadata));


        boost::shared_ptr<accountInfoFile::FileOperation> file(new accountInfoFile::FileBottomReaderWriter(testFile));

        boost::shared_ptr<accountInfoFile::StatBlockBuilder> statBuilder(new accountInfoFile::StatBlockBuilder);
        boost::shared_ptr<accountInfoFile::StatBlockWriter> statWriter(new accountInfoFile::StatBlockWriter);
        boost::shared_ptr<accountInfoFile::TrailerBlockBuilder> trailerBuilder(new accountInfoFile::TrailerBlockBuilder);
        boost::shared_ptr<accountInfoFile::TrailerBlockWriter> trailerWriter(new accountInfoFile::TrailerBlockWriter);

        accountInfoFile::Writer writer(file, statWriter, trailerWriter, trailerBuilder);
        accountInfoFile::Reader reader(file, statBuilder, trailerBuilder);

        file->open();
        writer.writeFile(stat, trailer);
        file->close();

        file->open();
        boost::shared_ptr<accountInfoFile::Trailer> trailerSecond = reader.readTrailer();
        boost::shared_ptr<accountInfoFile::StatBlock> statSecond = reader.readStatBlock();
        file->close();


        boost::shared_ptr<accountInfoFile::StatBlock::StatBlock_> statData = statSecond->getStat();

	// stat test
        ASSERT_STREQ("account1", statData->account);
        ASSERT_EQ(uint64_t(1000), statData->created_at);
        ASSERT_EQ(uint64_t(1001), statData->put_timestamp);
        ASSERT_EQ(uint64_t(2000), statData->delete_timestamp);
        ASSERT_EQ(uint64_t(5), statData->container_count);
        ASSERT_EQ(uint64_t(10), statData->object_count);
        ASSERT_EQ(uint64_t(1024), statData->bytes_used);
        ASSERT_STREQ("ABC123", statData->hash);
        ASSERT_STREQ("A-B-C-123", statData->id);
        ASSERT_STRNE("del", statData->status);
        ASSERT_EQ(uint64_t(1111), statData->status_changed_at);
        EXPECT_STREQ("lang=English1", statData->metadata);

	//trailer test
        EXPECT_EQ(uint64_t(300), trailerSecond->getStaticDataOffset());
        EXPECT_EQ(uint64_t(400), trailerSecond->getNonStaticDataOffset());
        EXPECT_EQ(uint64_t(100), trailerSecond->getStatOffset());

        ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path(testFile)));
        boost::filesystem::remove(boost::filesystem::path(testFile));
}

}

