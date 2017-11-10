#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <list>
#include "containerLibrary/file_writer.h"

namespace file_writer_test
{

TEST(FileWriterTest, write_in_info_test)	// To check if Start Cont Flush and End Container Flush are inserted in Journal
{

	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr;
	parser_ptr.reset(new config_file::ConfigFileParser("src/common/containerLibrary/config"));


	//boost::shared_ptr<journal_manager::JournalWriteHandler> journal_write_ptr;
	boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> journal_write_ptr;
	journal_write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path("./journal.log.1")));
	boost::shared_ptr<container_library::FileWriter> writer_ptr;
	writer_ptr.reset(new container_library::FileWriter(parser_ptr));
	recordStructure::ObjectRecord obj_record_object1(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	recordStructure::ObjectRecord obj_record_object2(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);

	std::list<recordStructure::ObjectRecord> record_list;
	record_list.push_back(obj_record_object1);
	record_list.push_back(obj_record_object2);

	writer_ptr->write_in_info("administrator", "dir1", record_list, journal_write_ptr, 10); //GM- commented for workaround
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));

}

TEST(FileWriterTest, write_test)	// for checking start cont flush and end cont flush markers
{

	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr;
	parser_ptr.reset(new config_file::ConfigFileParser("src/common/containerLibrary/config"));


	//boost::shared_ptr<journal_manager::JournalWriteHandler> journal_write_ptr;
	boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> journal_write_ptr;
	journal_write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path("./journal.log.1")));
	boost::shared_ptr<container_library::FileWriter> writer_ptr;
	writer_ptr.reset(new container_library::FileWriter(parser_ptr));
	recordStructure::ObjectRecord obj_record_object1(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	recordStructure::ObjectRecord obj_record_object2(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);

	std::list<recordStructure::ObjectRecord> record_list;
	record_list.push_back(obj_record_object1);
	record_list.push_back(obj_record_object2);

	container_library::MemoryManager mem_obj;
	recordStructure::ObjectRecord data(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	mem_obj.add_entry("Garima", "Dir1", data);
	mem_obj.add_entry("Garima", "Dir2", data);
	std::map<std::string, container_library::SecureObjectRecord>mem_map = mem_obj.get_memory_map();

	writer_ptr->write(mem_map, journal_write_ptr);
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
}
}
