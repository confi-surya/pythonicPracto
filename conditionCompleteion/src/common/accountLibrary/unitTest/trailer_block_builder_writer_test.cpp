#include "gtest/gtest.h"

#include "accountLibrary/trailer_block_builder.h"
#include "accountLibrary/trailer_block_writer.h"

namespace trailerBlockBlockTest
{

TEST(TrailerBlockBuilderTest, BuildFromValueTest)
{
        accountInfoFile::TrailerBlockBuilder builder;
        boost::shared_ptr<accountInfoFile::TrailerBlock> trailer = builder.create_block_from_content(0, 300, 200, 100);

        EXPECT_EQ(uint64_t(0), trailer->get_static_data_block_offset());
        EXPECT_EQ(uint64_t(300), trailer->get_non_static_data_block_offset());
        EXPECT_EQ(uint64_t(500), trailer->get_stat_block_offset());
        EXPECT_EQ(uint64_t(600), trailer->get_trailer_block_offset());
}

TEST(TrailerBlockBuilderTest, builedFromBufferTest)
{
        boost::shared_ptr<accountInfoFile::TrailerBlock> trailer(new accountInfoFile::TrailerBlock(0, 300, 500, 600));

	accountInfoFile::TrailerBlockWriter writer;
	accountInfoFile::block_buffer_size_tuple trailer_tuple =  writer.get_writable_block(trailer);
        
	accountInfoFile::TrailerBlockBuilder builder;
        boost::shared_ptr<accountInfoFile::TrailerBlock> tp = builder.create_block_from_buffer(trailer_tuple.get<0>());

        EXPECT_EQ(uint64_t(0), tp->get_static_data_block_offset());
        EXPECT_EQ(uint64_t(300), tp->get_non_static_data_block_offset());
        EXPECT_EQ(uint64_t(500), tp->get_stat_block_offset());
	EXPECT_EQ(uint64_t(600), tp->get_trailer_block_offset());

	
        tp->set_static_data_block_offset(100);
        tp->set_non_static_data_block_offset(200);

	accountInfoFile::block_buffer_size_tuple trailer_tuple_2 = writer.get_writable_block(tp);

        boost::shared_ptr<accountInfoFile::TrailerBlock> trailer_second = builder.create_block_from_buffer(trailer_tuple_2.get<0>());

        EXPECT_EQ(uint64_t(100), trailer_second->get_static_data_block_offset());
        EXPECT_EQ(uint64_t(200), trailer_second->get_non_static_data_block_offset());
        EXPECT_EQ(uint64_t(500), trailer_second->get_stat_block_offset());
	EXPECT_EQ(uint64_t(600), trailer_second->get_trailer_block_offset());

	delete trailer_tuple.get<0>();
	delete trailer_tuple_2.get<0>();
}

}

