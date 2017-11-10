#include "gmock/gmock.h"
#include <algorithm>
#include "gtest/gtest.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "containerLibrary/journal_reader.h"
#include "containerLibrary/journal_writer.h"

namespace journal_reader_test
{

TEST(ContainerJournalReadHandler, process_snapshot_test)
{	
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
	config_file_builder.readFromFile("src/common/containerLibrary/config");
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	boost::shared_ptr<journal_manager::JournalWriteHandler> write_ptr;
	write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	write_ptr->process_snapshot();

	boost::shared_ptr<journal_manager::JournalReadHandler> reader_ptr;
	boost::shared_ptr<container_library::MemoryManager> mem_obj(new container_library::MemoryManager);

	reader_ptr.reset(new container_read_handler::ContainerJournalReadHandler(".", mem_obj, parser_ptr));
	boost::shared_ptr<container_read_handler::ContainerJournalReadHandler> c_reader_ptr = boost::dynamic_pointer_cast<container_read_handler::ContainerJournalReadHandler>(reader_ptr);
	ASSERT_TRUE(reader_ptr->process_snapshot());
	ASSERT_EQ(c_reader_ptr->get_last_commit_offset(), int64_t(0));
	//ASSERT_EQ(c_reader_ptr->get_file_id(), "./journal.log.1");
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
}

TEST(ContainerJournalReadHandler, process_checkpoint_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
	config_file_builder.readFromFile("src/common/containerLibrary/config");
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();
	boost::shared_ptr<journal_manager::JournalWriteHandler> write_ptr;
	write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
//	write_ptr->process_snapshot();
	recordStructure::ObjectRecord *obj_record_object = new recordStructure::ObjectRecord(9, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	for (int i = 0; i < 60; i++)
		write_ptr->write(obj_record_object);

	boost::shared_ptr<journal_manager::JournalReadHandler> reader_ptr;

	boost::shared_ptr<container_library::MemoryManager> mem_obj(new container_library::MemoryManager);

	reader_ptr.reset(new container_read_handler::ContainerJournalReadHandler(".", mem_obj, parser_ptr));

	ASSERT_TRUE(reader_ptr->recover_last_checkpoint());
//	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
}

}
