#include "libraryUtils/helper.h"
#include "transactionLibrary/transaction_memory.h"
#include "transactionLibrary/transaction_write_handler.h"

#include "gtest/gtest.h"

TEST(snapshot_test, journal){
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

	int id = memory.acquire_lock(record);
	ASSERT_GT(id, -1);

	char object_name2[] = {'A', 'B'};
	char method2[] = {'G', 'E', 'T'};

	helper_utils::Request * request2 = new helper_utils::Request(object_name2, method2);
	recordStructure::ActiveRecord * record2 = new recordStructure::ActiveRecord(request2);

	id = memory.acquire_lock(record2);
	ASSERT_GT(id, -1);

	transaction_write_handler::TransactionWriteHandler writer(cstr, memory, parser_ptr);
	writer.write(record);
	writer.write(record2);
	writer.sync();
}
