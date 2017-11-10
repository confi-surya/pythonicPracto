#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <string>

#include "accountLibrary/non_static_data_block_builder.h"
#include "accountLibrary/non_static_data_block_writer.h"

namespace NonStaticDataBlockReaderWriterTest
{

TEST(NonStaticDataBlockReaderWriterTest, listWriteAndReadTest)
{
        boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block(new accountInfoFile::NonStaticDataBlock);

	std::string hash1("hash_value1:ABC1234");
	std::string hash2("hash_value2:ABC1234");
	std::string hash3("hash_value3:ABC1234");
	
	non_static_data_block->add(hash1, 10, 20, 30, 40, false);
	non_static_data_block->add(hash2, 100, 200, 300, 400, false);
	non_static_data_block->add(hash3, 1000, 2000, 3000, 4000, true);

	accountInfoFile::NonStaticDataBlockWriter writer;
	accountInfoFile::block_buffer_size_tuple non_static_data_tuple = writer.get_writable_block(non_static_data_block);

	accountInfoFile::NonStaticDataBlockBuilder reader;

	boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block_second = reader.create_block_from_memory(non_static_data_tuple.get<0>());
	

	ASSERT_TRUE(non_static_data_block_second->exists(hash1));
	ASSERT_TRUE(non_static_data_block_second->exists(hash2));
	ASSERT_TRUE(non_static_data_block_second->exists(hash3));


        non_static_data_block_second = reader.create_map_block_from_memory(non_static_data_tuple.get<0>());

        std::map<std::string, accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord>::iterator map_itr = non_static_data_block_second->get_non_static_map_iterator();

        ASSERT_EQ(uint64_t(10), (map_itr->second).put_timestamp);
        ASSERT_EQ(hash1, map_itr->second.hash);

        ++map_itr;
        ASSERT_EQ(uint64_t(100), (map_itr->second).put_timestamp);
        ASSERT_EQ(false, (map_itr->second).deleted);
        ASSERT_EQ(hash2, map_itr->second.hash);

        ++map_itr;
        ASSERT_EQ(hash3, map_itr->second.hash);
        ASSERT_EQ(true, (map_itr->second).deleted);

        delete non_static_data_tuple.get<0>();

		
}

TEST(NonStaticDataBlockReaderWriterTest, mapWriteAndReadTest)
{
        boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block(new accountInfoFile::NonStaticDataBlock);

        std::string hash1("hash_value1:ABC1234");
        std::string hash2("hash_value2:ABC1234");
        std::string hash3("hash_value3:ABC1234");

        non_static_data_block->add_record_in_map(hash1, 10, 20, 30, 40, false );
        non_static_data_block->add_record_in_map(hash2, 100, 200, 300, 400, false);
        non_static_data_block->add_record_in_map(hash3, 1000, 2000, 3000, 4000, true);

        accountInfoFile::NonStaticDataBlockWriter writer;
        accountInfoFile::block_buffer_size_tuple non_static_data_tuple = writer.get_writable_map_block(non_static_data_block);

        accountInfoFile::NonStaticDataBlockBuilder reader;

        boost::shared_ptr<accountInfoFile::NonStaticDataBlock> non_static_data_block_second = reader.create_map_block_from_memory(non_static_data_tuple.get<0>());

	std::map<std::string, accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord>::iterator map_itr = non_static_data_block_second->get_non_static_map_iterator(); 

        ASSERT_EQ(uint64_t(10), (map_itr->second).put_timestamp);
        ASSERT_EQ(hash1, map_itr->second.hash);
	
	++map_itr;
	ASSERT_EQ(uint64_t(100), (map_itr->second).put_timestamp);
        ASSERT_EQ(false, (map_itr->second).deleted);
        ASSERT_EQ(hash2, map_itr->second.hash);
	
	++map_itr;
        ASSERT_EQ(hash3, map_itr->second.hash);
        ASSERT_EQ(true, (map_itr->second).deleted);
        

	non_static_data_block->add(hash1, 10, 20, 30, 40, false);
        non_static_data_block->add(hash2, 100, 200, 300, 400, false);
        non_static_data_block->add(hash3, 1000, 2000, 3000, 4000, false);

        non_static_data_block_second = reader.create_block_from_memory(non_static_data_tuple.get<0>());

        ASSERT_TRUE(non_static_data_block_second->exists(hash1));
        ASSERT_TRUE(non_static_data_block_second->exists(hash2));
        ASSERT_TRUE(non_static_data_block_second->exists(hash3));

        delete non_static_data_tuple.get<0>();

}
}
