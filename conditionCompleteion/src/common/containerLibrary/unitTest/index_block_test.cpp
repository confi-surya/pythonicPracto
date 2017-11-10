#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/index_block.h"

namespace indexBlockTest
{

TEST(IndexBlockTest, contructorTest)
{
	containerInfoFile::IndexBlock index;

	index.add("name");
	index.add("naam-dusra");

	std::string name("name");
	std::string name1("name");
	ASSERT_TRUE(index.exists(name));
	ASSERT_TRUE(index.exists(name1));

	index.remove("name");
	ASSERT_FALSE(index.exists(name));
}

}
