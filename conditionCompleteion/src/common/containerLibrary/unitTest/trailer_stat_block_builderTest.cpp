#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/python/extract.hpp>

#include "containerLibrary/trailer_stat_block_builder.h"
#include "containerLibrary/trailer_stat_block_writer.h"

namespace trailerBlockBlockTest
{

TEST(TrailerBlockBuilderTest, BuildFromValueTest)
{
	containerInfoFile::TrailerAndStatBlockBuilder builder;

	boost::python::dict metadata;
	metadata["lang"] = "English";
	boost::shared_ptr<recordStructure::ContainerStat> stat = builder.create_stat_from_content("account1", 
		"container1", 1000, 1001, 2000, 1, 1024, "ABC123", "A-B-C-123", "DELETED", 1111, metadata);

	boost::shared_ptr<containerInfoFile::Trailer> trailer = builder.create_trailer_from_content(600, 500, 400, 300, 200, 100);

	ASSERT_EQ(int64_t(1000), stat->created_at);
	ASSERT_STREQ("account1", stat->account.c_str());
	std::string str =  boost::python::extract<std::string>(stat->metadata["lang"]);
	ASSERT_STREQ("English", str.c_str());
	
	ASSERT_EQ(uint64_t(600), trailer->sorted_record);
	ASSERT_EQ(uint64_t(500), trailer->unsorted_record);
	ASSERT_EQ(uint64_t(400), trailer->index_offset);
	ASSERT_EQ(uint64_t(300), trailer->data_index_offset);
	ASSERT_EQ(uint64_t(200), trailer->data_offset);
	ASSERT_EQ(uint64_t(100), trailer->stat_offset);
}

TEST(TrailerBlockBuilderTest, builedFromBufferTest)
{

	containerInfoFile::TrailerAndStatBlockBuilder builder;

	boost::python::dict metadata;
        metadata["lang"] = "English";
	boost::shared_ptr<recordStructure::ContainerStat> stat = builder.create_stat_from_content("account1",
			"container1", 1000, 1001, 2000, 1, 1024, "ABC123", "A-B-C-123", "DELETED", 1111, metadata);

	boost::shared_ptr<containerInfoFile::Trailer> trailer = builder.create_trailer_from_content(600, 500, 400, 300, 200, 100);

	containerInfoFile::TrailerStatBlockWriter writer;
	containerInfoFile::block_buffer_size_tuple stat_tuple = writer.get_writable_block(stat, trailer);
	boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, \
			boost::shared_ptr<containerInfoFile::Trailer> > blocks_tuple = builder.create_trailer_and_stat(stat_tuple.get<0>());
			boost::shared_ptr<containerInfoFile::Trailer> trailerSecond = blocks_tuple.get<1>();
	boost::shared_ptr<recordStructure::ContainerStat> statSecond= blocks_tuple.get<0>();

	ASSERT_EQ(int64_t(1000), statSecond->created_at);
	ASSERT_STREQ("account1", statSecond->account.c_str());
	ASSERT_STREQ("container1", statSecond->container.c_str());
	std::string str =  boost::python::extract<std::string>(stat->metadata["lang"]);
        ASSERT_STREQ("English", str.c_str());

	ASSERT_EQ(uint64_t(600), trailerSecond->sorted_record);
	ASSERT_EQ(uint64_t(500), trailerSecond->unsorted_record);
	ASSERT_EQ(uint64_t(400), trailerSecond->index_offset);
	ASSERT_EQ(uint64_t(300), trailerSecond->data_index_offset);
	ASSERT_EQ(uint64_t(200), trailerSecond->data_offset);
	ASSERT_EQ(uint64_t(100), trailerSecond->stat_offset);

	delete stat_tuple.get<0>();
}

}
