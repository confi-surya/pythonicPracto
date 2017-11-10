#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/index_block_builder.h"
#include "containerLibrary/index_block_writer.h"

namespace indexBlockReaderWriterTest
{

TEST(indexBlockReaderWriterTest, writeAndReadTest)
{
	boost::shared_ptr<containerInfoFile::IndexBlock> index(new containerInfoFile::IndexBlock());
	std::string name1("name");
	std::string name2("pname");
	std::string name3("ttname");

	index->add(name1);
	index->add(name2);
	index->add(name3);
	
	containerInfoFile::IndexBlockWriter writer;
	containerInfoFile::block_buffer_size_tuple index_tuple = writer.get_writable_block(index);

	containerInfoFile::IndexBlockBuilder reader;

	boost::shared_ptr<containerInfoFile::IndexBlock> index_second = reader.create_index_block_from_memory(index_tuple.get<0>(), 3);
	
	ASSERT_TRUE(index_second->exists(name1));
	ASSERT_TRUE(index_second->exists(name2));
	ASSERT_TRUE(index_second->exists(name3));

	delete index_tuple.get<0>();
}

}
