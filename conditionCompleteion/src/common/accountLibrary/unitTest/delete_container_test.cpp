#include <boost/filesystem.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include<list>

#include "accountLibrary/account_info_file_manager.h"
#include "accountLibrary/record_structure.h"

namespace listContainerTest
{

TEST(deleteContianerTest, deleteContainer)
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
        account_stat->metadata = "lang=English";

        a_file_man->create_account("Service_1234", path, account_stat);

        boost::shared_ptr<recordStructure::ContainerRecord> container_record_1(
					new recordStructure::ContainerRecord(1, "my_container_1", 10, 20, 30, 40, false));
        boost::shared_ptr<recordStructure::ContainerRecord> container_record_2(
					new recordStructure::ContainerRecord(10, "my_container_2", 100, 200, 300, 400, false));
        boost::shared_ptr<recordStructure::ContainerRecord> container_record_3(
					new recordStructure::ContainerRecord(100, "my_container_3", 1000, 2000, 3000, 4000, false));

        boost::shared_ptr<recordStructure::ContainerRecord> container_record_1_delete(
					new recordStructure::ContainerRecord(1, "my_container_1", 10, 20, 30, 40, true));
        boost::shared_ptr<recordStructure::ContainerRecord> container_record_2_delete(
					 new recordStructure::ContainerRecord(10, "my_container_2", 100, 200, 300, 400, true));
	boost::shared_ptr<recordStructure::ContainerRecord> container_record_3_update(
					new recordStructure::ContainerRecord(100, "my_container_3", 4, 3, 2, 1, false));

        a_file_man->add_container(path, container_record_1);
        a_file_man->add_container(path, container_record_2);
        a_file_man->add_container(path, container_record_3);

	a_file_man->delete_container(path, container_record_1_delete);
	a_file_man->delete_container(path, container_record_2_delete);
	a_file_man->delete_container(path, container_record_3_update); //update value using delete interface

	
	std::list<recordStructure::ContainerRecord> container_records = a_file_man->list_container(path);

	ASSERT_EQ(uint64_t(0), container_records.size());
	
	/* /std::list<recordStructure::ContainerRecord>::iterator it = container_records.begin();

	EXPECT_EQ(uint64_t(100), it->get_ROWID());
	EXPECT_EQ(std::string("my_container_3"), it->get_name());
	EXPECT_EQ(uint64_t(4), it->get_put_timestamp());
	EXPECT_EQ(uint64_t(3), it->get_delete_timestamp());
	EXPECT_EQ(uint64_t(2), it->get_object_count());
	EXPECT_EQ(uint64_t(1), it->get_bytes_used());
	EXPECT_EQ(false, it->get_deleted()); */
	
	a_file_man->delete_account(path);
}
}
