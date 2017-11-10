#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "accountLibrary/trailer_block.h"

namespace trailerBlockTest
{

TEST(TrailerBlockTest, contructorTest)
{
        accountInfoFile::TrailerBlock trailer(300, 200, 100, 0);

        ASSERT_EQ(uint64_t(300), trailer.get_static_data_block_offset());
        ASSERT_EQ(uint64_t(200), trailer.get_non_static_data_block_offset());
        ASSERT_EQ(uint64_t(100), trailer.get_stat_block_offset());
        ASSERT_EQ(uint64_t(0), trailer.get_trailer_block_offset());
}

TEST(TrailerBlockTest, emptyContructorTest)
{
        accountInfoFile::TrailerBlock trailer;

        trailer.set_static_data_block_offset(300);
        trailer.set_non_static_data_block_offset(200);
        trailer.set_stat_block_offset(100);
	trailer.set_trailer_block_offset(0);

        ASSERT_EQ(uint64_t(300), trailer.get_static_data_block_offset());
        ASSERT_EQ(uint64_t(200), trailer.get_non_static_data_block_offset());
        ASSERT_EQ(uint64_t(100), trailer.get_stat_block_offset());
	ASSERT_EQ(uint64_t(0), trailer.get_trailer_block_offset());
}

TEST(TrailerBlockTest, setValueTest)
{
        accountInfoFile::TrailerBlock trailer(1,2,3,4);

        trailer.set_static_data_block_offset(300);
        trailer.set_non_static_data_block_offset(200);
        trailer.set_stat_block_offset(100);
        trailer.set_trailer_block_offset(0);

        ASSERT_EQ(uint64_t(300), trailer.get_static_data_block_offset());
        ASSERT_EQ(uint64_t(200), trailer.get_non_static_data_block_offset());
        ASSERT_EQ(uint64_t(100), trailer.get_stat_block_offset());
        ASSERT_EQ(uint64_t(0), trailer.get_trailer_block_offset());
}

} //
