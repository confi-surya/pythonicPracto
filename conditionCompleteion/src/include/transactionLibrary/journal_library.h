#ifndef __TRANSACTION_JOURNAL_LIBRARY_H_987__
#define __TRANSACTION_JOURNAL_LIBRARY_H_987__

#include <boost/shared_ptr.hpp>

#include "libraryUtils/record_structure.h"
#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/config_file_parser.h"
#include "libraryUtils/job_manager.h"

namespace transaction_memory
{
	class TransactionStoreManager;
}

namespace transaction_write_handler
{
	class TransactionWriteHandler;
}

namespace transaction_library
{

class JournalManager
/* Class manages journal file */
{
	public:
		JournalManager(
			std::string & journal_path,
			boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
			uint8_t node_id
		);
		~JournalManager();
		void add_entry(boost::shared_ptr<recordStructure::Record> record, boost::function<void(void)> const & done);
		void add_recovered_data_in_journal(std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_entries,
		boost::function<void(void)> const & done
		);
		//
		boost::shared_ptr<transaction_write_handler::TransactionWriteHandler> get_journal_writer();
	private:
		boost::shared_ptr<transaction_write_handler::TransactionWriteHandler> journal_writer_ptr;
	        boost::shared_ptr<job_manager::SyncJobs> sync_jobs_ptr;
		boost::shared_ptr<job_manager::JobQueueMgr<job_manager::Job> > queue_ptr;
		boost::shared_ptr<job_manager::JobExecutor<job_manager::Job> > job_executor_ptr;
		boost::thread job_executor_thread;
};

class Journal
{
	public:
		Journal(
			std::string & journal_path,
			boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
			uint8_t node_id
		);
		~Journal();
		void add_entry(boost::shared_ptr<recordStructure::Record> record, boost::function<void(void)> const & done);
                void add_recovered_data_in_journal(std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_entries,
		boost::function<void(void)> const & done
		);
		boost::shared_ptr<transaction_write_handler::TransactionWriteHandler> get_journal_writer();

	private:
		boost::shared_ptr<JournalManager> journal_manager_ptr;
		std::string journal_path;

};

}
#endif
