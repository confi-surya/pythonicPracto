#include "gtest/gtest.h"

#include "containerLibrary/void_block_builder.h"
#include "containerLibrary/void_block_writer.h"

namespace voidBlockBuilderTest
{

TEST(VoidBlockBuilderTest, BuildFromValueTest)
{

	
	//containerInfoFile::VoidBlockBuilder builder;
	boost::shared_ptr<containerInfoFile::VoidBlock> void_block(new containerInfoFile::VoidBlock(10000)); 
	void_block->get_free_space(5000);

        ASSERT_EQ(uint64_t(5000), void_block->get_size());

	void_block->get_free_space(15000);
        ASSERT_EQ(uint64_t(10000), void_block->get_size());
}

TEST(VoidBlockBuilderTest, ReadAndWriteTest)
{


        containerInfoFile::VoidBlockBuilder builder;
        containerInfoFile::VoidBlockWriter writer;
        boost::shared_ptr<containerInfoFile::VoidBlock> void_block(new containerInfoFile::VoidBlock(10000));
        void_block->get_free_space(5000);

	containerInfoFile::block_buffer_size_tuple void_tuple = writer.get_writable_block(void_block);
	boost::shared_ptr<containerInfoFile::VoidBlock> void_block_second = builder.create_void_block_from_buffer(void_tuple.get<0>());

        ASSERT_EQ(uint64_t(5000), void_block->get_size());

        void_block->get_free_space(15000);
        ASSERT_EQ(uint64_t(10000), void_block->get_size());
}


}

