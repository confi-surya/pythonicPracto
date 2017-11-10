#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <string>

#include <boost/shared_ptr.hpp>

#include "accountLibrary/file.h"
#include "accountLibrary/non_static_data_block_writer.h"
#include "accountLibrary/non_static_data_block_builder.h"
#include "accountLibrary/stat_block_writer.h"
#include "accountLibrary/stat_block_builder.h"
#include "accountLibrary/trailer_block_writer.h"
#include "accountLibrary/trailer_block_builder.h"
#include "accountLibrary/record_structure.h"
#include "accountLibrary/reader.h"
#include "accountLibrary/writer.h"

namespace NonStaticDataBlockRWTest
{

TEST(NonStaticDataBlockReaderWriterTest, writeAndReadTest)
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
	boost::shared_ptr<accountInfoFile::NonStaticDataBlockWriter> non_static_data_block_writer(new accountInfoFile::NonStaticDataBlockWriter);
	boost::shared_ptr<accountInfoFile::NonStaticDataBlockBuilder> non_static_data_block_reader(new accountInfoFile::NonStaticDataBlockBuilder);
        boost::shared_ptr<accountInfoFile::StatBlockBuilder> stat_reader(new accountInfoFile::StatBlockBuilder);
        boost::shared_ptr<accountInfoFile::StatBlockWriter> stat_writer(new accountInfoFile::StatBlockWriter);
	boost::shared_ptr<accountInfoFile::TrailerBlockWriter> trailer_block_writer(new accountInfoFile::TrailerBlockWriter);
	boost::shared_ptr<accountInfoFile::TrailerBlockBuilder> trailer_block_builder(new accountInfoFile::TrailerBlockBuilder);
	

        accountInfoFile::Writer writer(file , non_static_data_block_writer, stat_writer, stat_reader,
					trailer_block_writer, trailer_block_builder);
	accountInfoFile::Reader reader(file, non_static_data_block_reader, stat_reader, trailer_block_builder);

	writer.write_file(account_stat); // write stat

        std::string hash1("hash_value1:ABC1234");
        std::string hash2("hash_value2:ABC1234");
        std::string hash3("hash_value3:ABC1234");
        std::string hash4("hash_value4:ABC1234");
        std::string hash_false("hash_value");


	boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block(new accountInfoFile::NonStaticDataBlock);
        non_static_data_block->add_record_in_map(hash1, 10, 20, 30, 40, false );
        non_static_data_block->add_record_in_map(hash2, 100, 200, 300, 400, false);
        non_static_data_block->add_record_in_map(hash3, 1000, 2000, 3000, 4000, true);
        non_static_data_block->add_record_in_map(hash4, 10000, 20000, 30000, 40000, true);
		
	boost::shared_ptr<recordStructure::AccountStat> account_stat_2 = reader.read_stat_block();
	boost::shared_ptr<accountInfoFile::TrailerBlock> trailer = reader.read_trailer_block();
	
	writer.write_non_static_data_map_block(non_static_data_block, account_stat_2, trailer); // write non static data block*/

	boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block_2 = reader.read_non_static_data_block();
	
	ASSERT_TRUE(non_static_data_block_2->exists(hash1));
	ASSERT_TRUE(non_static_data_block_2->exists(hash2));
	ASSERT_TRUE(non_static_data_block_2->exists(hash3));
	ASSERT_TRUE(non_static_data_block_2->exists(hash4));
	ASSERT_FALSE(non_static_data_block_2->exists(hash_false));

	writer.write_non_static_data_block(non_static_data_block_2, account_stat_2, trailer);

	boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block_3= reader.read_non_static_data_map_block();

	non_static_data_block_3->add(hash_false, 10000, 20000, 30000, 40000, true);
	
	ASSERT_TRUE(non_static_data_block_2->exists(hash1));
        ASSERT_TRUE(non_static_data_block_2->exists(hash2));
        ASSERT_TRUE(non_static_data_block_2->exists(hash3));
        ASSERT_TRUE(non_static_data_block_2->exists(hash4));
        ASSERT_FALSE(non_static_data_block_2->exists(hash_false));


}

}
