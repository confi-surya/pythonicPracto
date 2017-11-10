#ifndef __TRANSACTION_READ_HANDLER_H__
#define __TRANSACTION_READ_HANDLER_H___

#include <set>

#include "transactionLibrary/transaction_memory.h"
#include "libraryUtils/journal.h"

namespace transaction_read_handler
{

using recordStructure::JournalOffset;


class TransactionReadHandler: public journal_manager::JournalReadHandler
{
	public:
		TransactionReadHandler(
			std::string& path,
			boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
		);
		~TransactionReadHandler();
		//void clean_journal_files(std::string journal_path);
		void mark_unknown_components(std::list<int32_t>& component_list);
	protected:
		bool process_last_checkpoint(int64_t offset);
		//bool set_recovering_offset();
		bool process_recovering_offset();
		bool process_snapshot();
		bool recover_objects(recordStructure::Record * record_ptr, std::pair<JournalOffset, int64_t> offset);
		boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr;
	private:
		std::set<int64_t> active_id_list;
		std::set<std::pair<int64_t, int64_t> > id_mapper;
		int64_t journal_version;
};

class TransactionRecovery
{
	public:
		TransactionRecovery(
			std::string & path,
			boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
		);
		~TransactionRecovery();
		bool perform_recovery(std::list<int32_t>& component_list);
		void clean_journal_files();
		uint64_t get_next_file_id();
	private:
		boost::shared_ptr<TransactionReadHandler> journal_reader_ptr;
};

}// transaction_read_handler namespace end
#endif
