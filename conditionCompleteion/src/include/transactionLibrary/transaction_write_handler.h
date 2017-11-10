#ifndef __TRANSACTION_WRITE_HANDLER__
#define __TRANSACTION_WRITE_HANDLER__

#include "libraryUtils/journal.h"
#include "libraryUtils/record_structure.h"
#include "transactionLibrary/transaction_memory.h"

#define SNAPSHOT_HEADER_SIZE 1048576

namespace transaction_write_handler
{

class TransactionWriteHandler: public journal_manager::JournalWriteHandler
{
	public:
		TransactionWriteHandler(
			std::string & path,
			boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
			uint8_t node_id
		);
		~TransactionWriteHandler();
		void process_write(recordStructure::Record * record, int64_t record_size);
		void process_sync();
		void process_checkpoint();
		void process_snapshot();
		void remove_old_files();
	protected:
		boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr;
	private:
//		void process_snapshot_internal(bool full);
		void remove_old_journal_file();
		int64_t last_transaction_id;
};

}// transaction_write_handler namespace ends
#endif
