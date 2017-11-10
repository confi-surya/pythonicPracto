#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "accountLibrary/non_static_data_block.h"

namespace nonStaticDataBlockTest
{

TEST(NonStaticDataBlockTest, listStructure)
{
        accountInfoFile::NonStaticDataBlock non_static_data_block;

        std::string hash1("1ABC1!$abc1");
        std::string hash2("2ABC1!$abc2");
        std::string hash3("3ABC1!$abc3");

        non_static_data_block.add(hash1, 10, 20, 30, 40, false );
        non_static_data_block.add(hash2, 100, 200, 300, 400, false );

        ASSERT_TRUE(non_static_data_block.exists(hash1));
        ASSERT_TRUE(non_static_data_block.exists(hash2));
        ASSERT_FALSE(non_static_data_block.exists(hash3));

        accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord sdr1 = non_static_data_block.get(hash2);
        ASSERT_EQ(sdr1.deleted, false);

        non_static_data_block.remove(hash1);
        accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord sdr2 = non_static_data_block.get(hash1);
        ASSERT_EQ(sdr2.hash, "");

}

TEST(NonStaticDataBlockTest, mapStructure)
{
        accountInfoFile::NonStaticDataBlock non_static_data_block;

        std::string hash1("1ABC1!$abc1");
        std::string hash2("2ABC1!$abc2");
        std::string hash3("3ABC1!$abc3");

        non_static_data_block.add_record_in_map(hash1, 10, 20, 30, 40, false );
        non_static_data_block.add_record_in_map(hash2, 100, 200, 300, 400, false);

        std::map<std::string, accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord>::iterator sdr_itr = non_static_data_block.get_non_static_map_iterator();
        ASSERT_EQ(sdr_itr->second.hash, hash1);
        ASSERT_EQ(sdr_itr->second.deleted, false);

	sdr_itr++;
	
        ASSERT_EQ(sdr_itr->second.hash, hash2);
        ASSERT_EQ(sdr_itr->second.deleted, false);
	
	ASSERT_EQ(uint16_t(2), non_static_data_block.get_map_size());
	
	non_static_data_block.remove_record_from_map(hash1);
	
	ASSERT_EQ(uint16_t(1), non_static_data_block.get_map_size());

	std::map<std::string, accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord>::iterator sdr_itr_1 = non_static_data_block.get_non_static_map_iterator();
	
	ASSERT_EQ(sdr_itr_1->second.hash, hash2);
        ASSERT_EQ(sdr_itr_1->second.deleted, false);

	non_static_data_block.update_record_in_map(hash2, 1000, 2000, 3000, 4000, true);

	
	std::map<std::string, accountInfoFile::NonStaticDataBlock::NonStaticDataBlockRecord>::iterator sdr_itr_2 = non_static_data_block.get_non_static_map_iterator();

        ASSERT_EQ(sdr_itr_2->second.put_timestamp, uint64_t(1000));
        ASSERT_EQ(sdr_itr_2->second.deleted, true);
	
}

}
