#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/file.h"

namespace fileOperationTest
{

TEST(fileOperationTest, openTest)
{
	std::string testFile("test.file");
	std::ofstream fileTmp;
	fileTmp.open(testFile.c_str(),  std::ios::out | std::ios::binary);
	fileTmp<<"out";
	fileTmp.close();

	containerInfoFile::FileOperation file(testFile);
	char * buff = new char [11];
	file.open();
	file.write("testString", 10, 0);
	file.read(buff, 10, 0);
	buff[10] = '\0';
	file.close();

	ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path(testFile)));
	ASSERT_STREQ(buff, "testString");
	boost::filesystem::remove(boost::filesystem::path(testFile));
	delete buff;
}

TEST(fileOperationTest, createTest)
{
	std::string testFile("account1/container1/test.file");
	containerInfoFile::FileOperation file(testFile);
	file.open_for_create();
	file.write("testString", 10, 0);
	file.close();
	ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path(testFile)));
	boost::filesystem::remove_all(boost::filesystem::path("account1"));
}

}
