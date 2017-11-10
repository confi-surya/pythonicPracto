#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "accountLibrary/static_data_block.h"
#include <string>

namespace StaticDataBlockTest
{

TEST(StaticDataBlockTest, classMethodTest)
{
        accountInfoFile::StaticDataBlock static_data_block;
	
	static_data_block.add(1,std::string("One"));
	static_data_block.add(2,std::string("Two"));
	static_data_block.add(3,std::string("Three"));
	static_data_block.add(4,std::string("Four"));
	static_data_block.add(5,std::string("Five"));
	static_data_block.add(6,std::string("Six"));
	static_data_block.add(7,std::string("Seven"));
	
	// interface get_static_data_record_by_index test
	accountInfoFile::StaticDataBlock::StaticDataBlockRecord stbr_test(static_data_block.get_static_data_record_by_index(2));
	ASSERT_EQ(uint64_t(3), stbr_test.ROWID);
	ASSERT_EQ("Three", stbr_test.name);

	//static data slicing test
	std::vector<accountInfoFile::StaticDataBlock::StaticDataBlockRecord> static_data_record_slice = static_data_block.get_static_data_records_by_indeces(2,5);
	ASSERT_EQ(uint64_t(3), static_data_record_slice[0].ROWID);
	ASSERT_EQ("Three", static_data_record_slice[0].name);
	ASSERT_EQ(uint64_t(6), static_data_record_slice[static_data_record_slice.size()-1].ROWID);
	ASSERT_EQ("Six", static_data_record_slice[static_data_record_slice.size()-1].name);

	//remove data element test
	ASSERT_EQ(uint64_t(7), static_data_block.get_size());
	
	static_data_block.remove_static_data_record_by_index(4);
	
	ASSERT_EQ(uint64_t(6), static_data_block.get_size());
	
	ASSERT_EQ(uint64_t(6), static_data_block.get_static_data_record_by_index(4).ROWID);
        ASSERT_EQ("Six", static_data_block.get_static_data_record_by_index(4).name);
}

}
