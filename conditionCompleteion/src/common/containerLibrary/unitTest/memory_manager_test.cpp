#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/memory_writer.h"

namespace memory_manager_test
{

TEST(memory_manager_test, add_entry_test)
{
        container_library::MemoryManager mem_obj;
        recordStructure::ObjectRecord data(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
        mem_obj.add_entry("admin/Dir1", data);
	std::map<std::string, container_library::SecureObjectRecord>mem_map = mem_obj.get_memory_map();
	ASSERT_EQ(mem_map.begin()->first, "admin/Dir1");
	ASSERT_EQ(mem_map.begin()->second.get_record().front().get_row_id(), uint64_t(1));
	ASSERT_EQ(mem_map.begin()->second.get_record().front().get_name(), "abc");
	ASSERT_EQ(mem_map.begin()->second.get_record().front().get_deleted_flag(), false);
	ASSERT_EQ(mem_map.begin()->second.get_record().front().get_creation_time(), uint64_t(21072014));
	ASSERT_EQ(mem_map.begin()->second.get_record().front().get_size(), uint64_t(1024));
	ASSERT_EQ(mem_map.begin()->second.get_record().front().get_content_type(), "plain text");
	ASSERT_EQ(mem_map.begin()->second.get_record().front().get_etag(), "cxwqbciuwe");
	ASSERT_EQ(mem_obj.get_container_count(), uint32_t(1));
	ASSERT_EQ(mem_obj.get_record_count(), uint32_t(1));
	mem_obj.add_entry("admin/Dir2", data);
	mem_obj.add_entry("admin/Dir2", data);
	ASSERT_EQ(mem_obj.get_container_count(), uint32_t(2));
	ASSERT_EQ(mem_obj.get_record_count(), uint32_t(3));
}

TEST(memory_manager_test, reset_mem_size_test)
{
	container_library::MemoryManager mem_obj;
	mem_obj.reset_mem_size();
	ASSERT_EQ(mem_obj.get_container_count(), uint32_t(0));
	ASSERT_EQ(mem_obj.get_record_count(), uint32_t(0));
}

TEST(secure_object_record_test, constructor_test)
{
	recordStructure::ObjectRecord data(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	container_library::SecureObjectRecord secure_obj; 
	ASSERT_EQ(secure_obj.get_lock_status(), true);
}

TEST(secure_object_record_test, get_lock_status_test)
{
	container_library::SecureObjectRecord secure_obj; 
	ASSERT_EQ(secure_obj.get_lock_status(), true);	
}

TEST(secure_object_record_test, set_lock_test)
{
	container_library::SecureObjectRecord secure_obj;
	secure_obj.set_lock(false);
	ASSERT_EQ(secure_obj.get_lock_status(), false);

}
}
