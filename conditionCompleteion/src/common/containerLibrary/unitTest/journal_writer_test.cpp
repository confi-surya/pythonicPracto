#include "gmock/gmock.h"
#include <algorithm>
#include "gtest/gtest.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include "containerLibrary/journal_writer.h"

namespace journal_writer_test
{

TEST(JournalWriteHandlerTest, constructor_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();


	boost::shared_ptr<journal_manager::JournalWriteHandler> write_ptr;
	write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path("./journal.log.1")));
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
}

TEST(JournalWriteHandlerTest, write_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	recordStructure::ObjectRecord *obj_record_object = new recordStructure::ObjectRecord(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	boost::shared_ptr<journal_manager::JournalWriteHandler> write_ptr;
	write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	EXPECT_TRUE(write_ptr->write(obj_record_object));
	EXPECT_TRUE(write_ptr->sync());
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
}

TEST(ContainerJournalWriteHandler, process_snapshot_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	boost::shared_ptr<journal_manager::JournalWriteHandler> write_ptr;
	write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	write_ptr->process_snapshot();
	EXPECT_TRUE(write_ptr->sync());
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
}

TEST(ContainerJournalWriteHandler, process_checkpoint_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	boost::shared_ptr<journal_manager::JournalWriteHandler> write_ptr;
	write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	recordStructure::ObjectRecord *obj_record_object = new recordStructure::ObjectRecord(9, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	for (int i = 0; i < 100; i++)
		write_ptr->write(obj_record_object);
	write_ptr->sync();

	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
	boost::filesystem::remove(boost::filesystem::path("./journal.log.2"));
}

TEST(ContainerJournalWriteHandler, process_write_test)		// test for checking rotation of journal file
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	boost::shared_ptr<journal_manager::JournalWriteHandler> write_ptr;
	write_ptr.reset(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	recordStructure::ObjectRecord *obj_record_object = new recordStructure::ObjectRecord(9, "rotation", 21072014, 1024, "plain text", "cxwqbciuwe", false);

	for (int i = 0; i < 100; i++)
		write_ptr->write(obj_record_object);
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));
	boost::filesystem::remove(boost::filesystem::path("./journal.log.2"));
}


}
