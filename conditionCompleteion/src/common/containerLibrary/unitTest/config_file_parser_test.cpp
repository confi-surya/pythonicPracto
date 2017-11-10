#include "gmock/gmock.h"
#include <algorithm>
#include "gtest/gtest.h"

#include "libraryUtils/config_file_parser.h"

namespace config_file_parser_test
{

TEST(ConfigFileParserTest, get_file_writer_threads_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	uint32_t count = parser_ptr->file_writer_threads_countValue();
	ASSERT_EQ(count, uint32_t(10));
}

TEST(ConfigFileParserTest, get_transaction_journal_path_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	std::string transaction_journal_path = parser_ptr->transaction_journal_pathValue();
	transaction_journal_path.erase(std::remove(transaction_journal_path.begin(), transaction_journal_path.end(), '\"' ), transaction_journal_path.end());
//	std::cout << transaction_journal_path << std::endl;
	ASSERT_EQ(transaction_journal_path, "trans_journal.log");
}

TEST(ConfigFileParserTest, get_container_journal_path_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	std::string container_journal_path = parser_ptr->container_journal_pathValue();
	container_journal_path.erase(std::remove(container_journal_path.begin(), container_journal_path.end(), '\"' ), container_journal_path.end());
	ASSERT_EQ(container_journal_path, "container_journal.log");
}

TEST(ConfigFileParserTest, get_timeout_value_test)
{
	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	uint32_t count = parser_ptr->timeoutValue();
	ASSERT_EQ(count, uint32_t(300));
}

} // namespace config_file_parser_test
