#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>

#include "libraryUtils/file_handler.h"
#include "accountLibrary/account_info_file_manager.h"
#include "accountLibrary/record_structure.h"

namespace setTest
{

TEST(addMultiContainerTest, addMultiContainerTest)
{
	std::string path(boost::filesystem::current_path().string());
	boost::shared_ptr<accountInfoFile::AccountInfoFileManager> a_file_man(new accountInfoFile::AccountInfoFileManager());


	boost::shared_ptr<recordStructure::AccountStat> account_stat(new recordStructure::AccountStat());

	account_stat->account = "myaccount";
	account_stat->created_at = 100;
	account_stat->put_timestamp = 101;
	account_stat->delete_timestamp = 102;
	account_stat->container_count = 0;
	account_stat->object_count = 10;
	account_stat->bytes_used = 1024;
	account_stat->hash = "ABCD";
	account_stat->id = "A-B-C-D";
	account_stat->status = "DELETED";
	account_stat->status_changed_at = 200;
	
	a_file_man->create_account("Service_1234", path, account_stat);

	StatusAndResult::AccountStatWithStatus a_stat_obj = a_file_man->get_account_stat("/remote/hydra042/dkushwaha/tmp/tmp", path);
	recordStructure::AccountStat a_stat = a_stat_obj.get_account_stat();


	for(long i=1; i <= 200; i ++)
	{
		std::string str = boost::lexical_cast<std::string>(i);
		boost::shared_ptr<recordStructure::ContainerRecord> container_record(new recordStructure::ContainerRecord(uint64_t(i), "cont_" + str, "10", "20", "oye",  uint64_t(30), uint64_t(40), false));
		a_file_man->add_container(path, container_record);
		StatusAndResult::AccountStatWithStatus a_stat1_obj = a_file_man->get_account_stat("/remote/hydra042/dkushwaha/tmp/tmp", path);
		if(i%17 == 0){
			recordStructure::AccountStat a_stat1 = a_stat1_obj.get_account_stat();
			EXPECT_EQ(i, a_stat1.container_count);
		}
	}

	a_file_man->delete_account("/remote/hydra042/dkushwaha/tmp/tmp", path);
}

}
	
	
