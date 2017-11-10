#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"


#include "accountLibrary/account_info_file_manager.h"
#include "accountLibrary/record_structure.h"

namespace setTest
{

TEST(statTest, setAccountStatTest)
{
	std::string path(boost::filesystem::current_path().string());

	boost::shared_ptr<accountInfoFile::AccountInfoFileManager> a_file_man(new accountInfoFile::AccountInfoFileManager());

	boost::shared_ptr<recordStructure::AccountStat> account_stat(new recordStructure::AccountStat());

	account_stat->account = "myaccount";
	account_stat->created_at = 1000;
	account_stat->put_timestamp = 1001;
	account_stat->delete_timestamp = 1002;
	account_stat->container_count = 5;
	account_stat->object_count = 10;
	account_stat->bytes_used = 1024;
	account_stat->hash = "ABCD";
	account_stat->id = "A-B-C-D";
	account_stat->status = "DELETED";
	account_stat->status_changed_at = 2000;
	account_stat->metadata = "lang=Hindi";

	//create account info-file
	a_file_man->create_account("Service_1234", path, account_stat);

        boost::shared_ptr<recordStructure::AccountStat> aa_stat = a_file_man->get_account_stat(path);

	boost::shared_ptr<recordStructure::AccountStat> a_stat = a_file_man->get_account_stat(path);
	

        EXPECT_EQ(account_stat->account, a_stat->account);
        EXPECT_EQ(account_stat->created_at, a_stat->created_at);
        EXPECT_EQ(account_stat->put_timestamp, a_stat->put_timestamp);
        EXPECT_EQ(account_stat->delete_timestamp, a_stat->delete_timestamp);
        EXPECT_EQ(account_stat->container_count, a_stat->container_count);
        EXPECT_EQ(account_stat->object_count, a_stat->object_count);
        EXPECT_EQ(account_stat->bytes_used, a_stat->bytes_used);
        EXPECT_EQ(account_stat->hash, a_stat->hash);
        EXPECT_EQ(account_stat->id, a_stat->id);
        EXPECT_EQ(account_stat->status, a_stat->status);
        EXPECT_EQ(account_stat->status_changed_at, a_stat->status_changed_at);
        EXPECT_EQ("lang=Hindi", a_stat->metadata);
	

	account_stat->metadata = "lang=English";
	a_file_man->set_account_stat(path, account_stat);

	boost::shared_ptr<recordStructure::AccountStat>	a_stat_2 = a_file_man->get_account_stat(path);

	EXPECT_EQ(account_stat->account, a_stat_2->account);
	EXPECT_EQ(account_stat->created_at, a_stat_2->created_at);
	EXPECT_EQ(account_stat->put_timestamp, a_stat_2->put_timestamp);
	EXPECT_EQ(account_stat->delete_timestamp, a_stat_2->delete_timestamp);
	EXPECT_EQ(account_stat->container_count, a_stat_2->container_count);
	EXPECT_EQ(account_stat->object_count, a_stat_2->object_count);
	EXPECT_EQ(account_stat->bytes_used, a_stat_2->bytes_used);
	EXPECT_EQ(account_stat->hash, a_stat_2->hash);
	EXPECT_EQ(account_stat->id, a_stat_2->id);
	EXPECT_EQ(account_stat->status, a_stat_2->status);
	EXPECT_EQ(account_stat->status_changed_at, a_stat_2->status_changed_at);
	EXPECT_EQ("lang=English", a_stat_2->metadata);

	a_file_man->delete_account(path);
        EXPECT_FALSE(boost::filesystem::exists(boost::filesystem::path(path.append("/objectStorage.infoFile"))));
        //boost::filesystem::remove(boost::filesystem::path(path.append("/objectStorage.infoFile")));
}

}
	
	
