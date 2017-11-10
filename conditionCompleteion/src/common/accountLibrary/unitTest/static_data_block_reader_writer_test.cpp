#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <string>

#include <boost/shared_ptr.hpp>

#include "accountLibrary/file.h"
#include "accountLibrary/static_data_block_writer.h"
#include "accountLibrary/static_data_block_builder.h"
#include "accountLibrary/non_static_data_block_writer.h"
#include "accountLibrary/non_static_data_block_builder.h"
#include "accountLibrary/stat_block_writer.h"
#include "accountLibrary/stat_block_builder.h"
#include "accountLibrary/trailer_block_writer.h"
#include "accountLibrary/trailer_block_builder.h"
#include "accountLibrary/record_structure.h"
#include "accountLibrary/reader.h"
#include "accountLibrary/writer.h"

namespace StaticDataBlockRWTest
{

TEST(StaticDataBlockReaderWriterTest, writeAndReadTest)
{
	std::string tempFile("myaccount_create");
        std::ofstream fileTmp;
        fileTmp.open(tempFile.c_str(),  std::ios::out | std::ios::binary);
        fileTmp.close();


        boost::shared_ptr<recordStructure::AccountStat> account_stat(new recordStructure::AccountStat());

        account_stat->account = "myaccount_create";
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

	
        
        boost::shared_ptr<accountInfoFile::FileOperation> file(new accountInfoFile::FileOperation("myaccount_create"));
	boost::shared_ptr<accountInfoFile::StaticDataBlockWriter> static_data_block_writer(new accountInfoFile::StaticDataBlockWriter);
	boost::shared_ptr<accountInfoFile::StaticDataBlockBuilder> static_data_block_reader(new accountInfoFile::StaticDataBlockBuilder);
	boost::shared_ptr<accountInfoFile::NonStaticDataBlockWriter> non_static_data_block_writer(new accountInfoFile::NonStaticDataBlockWriter);
	boost::shared_ptr<accountInfoFile::NonStaticDataBlockBuilder> non_static_data_block_reader(new accountInfoFile::NonStaticDataBlockBuilder);
        boost::shared_ptr<accountInfoFile::StatBlockBuilder> stat_reader(new accountInfoFile::StatBlockBuilder);
        boost::shared_ptr<accountInfoFile::StatBlockWriter> stat_writer(new accountInfoFile::StatBlockWriter);
	boost::shared_ptr<accountInfoFile::TrailerBlockWriter> trailer_block_writer(new accountInfoFile::TrailerBlockWriter);
	boost::shared_ptr<accountInfoFile::TrailerBlockBuilder> trailer_block_builder(new accountInfoFile::TrailerBlockBuilder);
	

        accountInfoFile::Writer writer(file , 
					static_data_block_writer,
					non_static_data_block_writer,
					non_static_data_block_reader,
					stat_writer, stat_reader,
					trailer_block_writer,
					trailer_block_builder);
	accountInfoFile::Reader reader(file, static_data_block_reader, non_static_data_block_reader, stat_reader, trailer_block_builder);

	writer.write_file(account_stat); // write stat
	
	// static Data
	int64_t ROWID1= 1;
	std::string name1("my_first_container");
	int64_t ROWID2= 2;
	std::string name2("my_second_container");
	boost::shared_ptr<accountInfoFile::StaticDataBlock> static_data_block(new accountInfoFile::StaticDataBlock);
	static_data_block->add(ROWID1, name1);
	static_data_block->add(ROWID2, name2);
	
	// non-static data
        std::string hash1("my_first_container");
        std::string hash2("my_second_container");
        boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block(new accountInfoFile::NonStaticDataBlock);
        non_static_data_block->add(hash1, 1, 2, 3, 4, false);
        non_static_data_block->add(hash2, 11, 22, 33, 44, false);

	//stat section
	boost::shared_ptr<recordStructure::AccountStat> account_stat_2 = reader.read_stat_block();
	
	// trailer section
	boost::shared_ptr<accountInfoFile::TrailerBlock> trailer = reader.read_trailer_block();
	
	writer.write_static_data_block(static_data_block, non_static_data_block, account_stat_2, trailer); // write non static data block*/

	// testing : non-static data
	boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block_2 = reader.read_non_static_data_block();
	
	ASSERT_TRUE(non_static_data_block_2->exists(hash1));
	ASSERT_TRUE(non_static_data_block_2->exists(hash2));

	// testing : static data
	boost::shared_ptr<accountInfoFile::StaticDataBlock> static_data_block_2 = reader.read_static_data_block();
	

        accountInfoFile::StaticDataBlock::StaticDataBlockRecord stbr_test_1(static_data_block_2->get_static_data_record_by_index(0));
        ASSERT_EQ(uint64_t(1), stbr_test_1.ROWID);
        ASSERT_EQ("my_first_container", stbr_test_1.name);

	accountInfoFile::StaticDataBlock::StaticDataBlockRecord stbr_test_2(static_data_block_2->get_static_data_record_by_index(1));
        ASSERT_EQ(uint64_t(2), stbr_test_2.ROWID);
        ASSERT_EQ("my_second_container", stbr_test_2.name);
}

}
