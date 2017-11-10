#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/python/extract.hpp>

//#include "containerLibrary/stat_block.h"
#include "containerLibrary/trailer_stat_block_builder.h"

namespace statBlockTest
{



TEST(StatBlockTest, getStatTest)
{
	std::string account("account1");
        std::string container("container1");
        uint64_t created_at=1000;
        uint64_t put_timestamp=1001;
        uint64_t delete_timestamp=2000;
        int64_t object_count=1;
        uint64_t bytes_used=1024;
	std::string hash("ABC123");
	std::string id("A-B-C-123");
	std::string status("");
        uint64_t status_changed_at=1111;
	boost::python::dict metadata;
	metadata["lang"] = "English";

        recordStructure::ContainerStat stat(account, container, created_at, put_timestamp, delete_timestamp, \
                        object_count, bytes_used, hash, id, status, status_changed_at, metadata);
	// test getStat

        ASSERT_EQ(int64_t(1000), stat.created_at);
	ASSERT_STREQ("account1", stat.account.c_str());
	std::string str =  boost::python::extract<std::string>(stat.metadata["lang"]);
	ASSERT_STREQ("English", str.c_str()); //boost::python::extract<std::string> extracted_val(stat.metadata[key]));

} 

/*
TEST(TrailerBlockTest, setGetStatTest)
{
        boost::shared_ptr<containerInfoFile::StatBlock::StatBlock_> sbPtr;
        sbPtr = stat.getStat();

        strcpy(sbPtr->metadata , "lang=French");
        containerInfoFile::StatBlock statSecond(sbPtr);

        boost::shared_ptr<containerInfoFile::StatBlock::StatBlock_> sbPtrSecond;
        sbPtrSecond = stat.getStat();

        ASSERT_STREQ("lang=French", sbPtrSecond->metadata);
}*/

} 
