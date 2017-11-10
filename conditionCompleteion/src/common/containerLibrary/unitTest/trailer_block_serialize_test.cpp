#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/trailer_block.h"

namespace trailerBlockTest
{

TEST(ser, contructorTest)
{
	containerInfoFile::Trailer trailer(400, 300, 200, 100, 0);

	trailer.setDataOffset(800);
	trailer.setVoidOffset(600);
	trailer.set_index_offset(300);
	trailer.setStatOffset(200);

	ASSERT_EQ(uint64_t(800), trailer.getDataOffset());
	ASSERT_EQ(uint64_t(600), trailer.getVoidOffset());
	ASSERT_EQ(uint64_t(300), trailer.getindexOffset());
	ASSERT_EQ(uint64_t(200), trailer.getStatOffset());
}

}
