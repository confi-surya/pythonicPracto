#include "libraryUtils/helper.h"
#include "transactionLibrary/transaction_memory.h"
#include "transactionLibrary/transaction_write_handler.h"

#include "gtest/gtest.h"

/*
TEST(helper_test, requet_test){
    string object_name = "test";
    string method_name = "GET";
    string response;

    Request request(object_name, method_name);

    response = request.get_object_id();

    ASSERT_EQ(response, object_name);
}*/

TEST(journal_test, serialize_test){
	char object_name[] = {'A', 'B'};
	char method[] = {'G', 'E', 'T'};
	std::string path = ("cpp.journal");
	char * cstr = new char[path.length() + 1];

	helper_utils::Request * request = new helper_utils::Request(object_name, method);
	recordStructure::ActiveRecord * record = new recordStructure::ActiveRecord(request);

	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
        config_file_builder.readFromFile("src/common/containerLibrary/config");
        cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	transaction_memory::TransactionStoreManager memory(parser_ptr);

	strcpy(cstr, path.c_str());
	transaction_write_handler::TransactionWriteHandler writer(cstr, memory, parser_ptr);

	writer.write(record);
	writer.sync();
}
