#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/trailer_block.h"

namespace trailerBlockTest
{

TEST(TrailerBlockTest, contructorTest)
{
	containerInfoFile::Trailer trailer(500, 400, 300, 200, 120, 100, 0, 0);

	ASSERT_EQ(uint64_t(500), trailer.sorted_record);
	ASSERT_EQ(uint64_t(400), trailer.unsorted_record);
	ASSERT_EQ(uint64_t(300), trailer.index_offset);
	ASSERT_EQ(uint64_t(200), trailer.data_index_offset);
	ASSERT_EQ(uint64_t(120), trailer.data_offset);
	ASSERT_EQ(uint64_t(100), trailer.stat_offset);
}

TEST(TrailerBlockTest, emptyContructorTest)
{
	containerInfoFile::Trailer trailer;

	ASSERT_EQ(uint64_t(0), trailer.sorted_record);
        ASSERT_EQ(uint64_t(0), trailer.unsorted_record);
        ASSERT_EQ(uint64_t(0), trailer.data_index_offset);
        ASSERT_EQ(uint64_t(0), trailer.index_offset);
        ASSERT_EQ(uint64_t(0), trailer.stat_offset);
}

TEST(TrailerBlockTest, contructorTestWithValueReplaced)
{
	containerInfoFile::Trailer trailer(500, 400, 300, 200, 120, 100, 0, 0);

	trailer.sorted_record = 800;
	trailer.unsorted_record = 600;
	trailer.data_index_offset = 500;

	ASSERT_EQ(uint64_t(800), trailer.sorted_record);
        ASSERT_EQ(uint64_t(600), trailer.unsorted_record);
        ASSERT_EQ(uint64_t(500), trailer.data_index_offset);
        ASSERT_EQ(uint64_t(300), trailer.index_offset);
        ASSERT_EQ(uint64_t(100), trailer.stat_offset);
}

}
