#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "accountLibrary/account_info_file_manager.h"
#include "accountLibrary/record_structure.h"

namespace accountCreateRemove
{

TEST(statTest, createAccount)
{

        std::string account(boost::filesystem::current_path().string());
        boost::shared_ptr<accountInfoFile::AccountInfoFileManager> a_file_man(new accountInfoFile::AccountInfoFileManager());

        boost::shared_ptr<recordStructure::AccountStat> account_stat(new recordStructure::AccountStat());

        account_stat->account = "myaccount_create";
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
        account_stat->metadata = "lang=English";

	try
	{
		a_file_man->create_account("Service_1234", account, account_stat);
	}
	catch(...)
	{
		a_file_man->delete_account(account);
	}

	boost::shared_ptr<recordStructure::AccountStat> a_stat = a_file_man->get_account_stat(account);

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
        EXPECT_EQ("lang=English", a_stat->metadata);
}

TEST(statTest, deleteAccount)
{
	std::string account(boost::filesystem::current_path().string());

	std::string file_path = account + "/objectStorage.infoFile";

	EXPECT_TRUE(boost::filesystem::exists(boost::filesystem::path(file_path)));	

	boost::shared_ptr<accountInfoFile::AccountInfoFileManager> a_file_man(new accountInfoFile::AccountInfoFileManager());
	a_file_man->delete_account(account);

	EXPECT_FALSE(boost::filesystem::exists(boost::filesystem::path(file_path)));
}

}
