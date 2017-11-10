#include "transactionLibrary/journal_library.h"
#include "transactionLibrary/transaction_memory.h"
#include "transactionLibrary/transaction_write_handler.h"

namespace transaction_library
{

Journal::Journal(
	std::string & journal_path,
	boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr, 
	uint8_t node_id
) :
	journal_manager_ptr(new JournalManager(journal_path, memory_mgr_ptr, parser_ptr, node_id))
{
}

Journal::~Journal()
{
	OSDLOG(DEBUG, "Destructing Journal");
}

boost::shared_ptr<transaction_write_handler::TransactionWriteHandler> Journal::get_journal_writer()
{
	return this->journal_manager_ptr->get_journal_writer();
}

void Journal::add_recovered_data_in_journal(
		std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_records,
                boost::function<void(void)> const & done
)
{
	this->journal_manager_ptr->add_recovered_data_in_journal(transaction_records,done);
}
/* Function is an interface method which calls the concrete implementation of journal
 *  * @param request: Request class containing all request specific values
 *   */
void Journal::add_entry(boost::shared_ptr<recordStructure::Record> record, boost::function<void(void)> const & done)
{
	this->journal_manager_ptr->add_entry(record, done);
}

JournalManager::JournalManager(
	std::string & journal_path,
	boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	uint8_t node_id
):
	journal_writer_ptr(new transaction_write_handler::TransactionWriteHandler(journal_path, memory_mgr_ptr, parser_ptr,node_id))
{
	this->sync_jobs_ptr.reset(new job_manager::SyncJobs(this->journal_writer_ptr));
	this->queue_ptr.reset(new job_manager::JobQueueMgr<job_manager::Job>(this->sync_jobs_ptr));
	this->job_executor_ptr.reset(new job_manager::JobExecutor<job_manager::Job>(this->queue_ptr));
	this->job_executor_thread = boost::thread(boost::bind(&job_manager::JobExecutor<job_manager::Job>::execute_for_transaction, this->job_executor_ptr));

}

boost::shared_ptr<transaction_write_handler::TransactionWriteHandler> JournalManager::get_journal_writer()
{
	return this->journal_writer_ptr;
}

JournalManager::~JournalManager()
{
	OSDLOG(DEBUG, "Destructing JournalManager");
	this->job_executor_ptr->stop();
	this->job_executor_thread.join();
	OSDLOG(DEBUG, osd_transaction_lib_log_tag << "Job Executor joined, destruction of JournalImpl complete");
}

void JournalManager::add_recovered_data_in_journal(
		std::list<boost::shared_ptr<recordStructure::ActiveRecord> >& transaction_records,
		boost::function<void(void)> const & done
)
{
	//Add all the records in the Job queue
        std::list<boost::shared_ptr<job_manager::Job> > job_ptr_list;
	std::list<boost::shared_ptr<recordStructure::ActiveRecord> >
              			  ::iterator it = transaction_records.begin();
        for ( ; it != (--transaction_records.end()); it++ )
        {
                boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(NULL));
                boost::shared_ptr<job_manager::Job> job_ptr =
                        boost::make_shared<job_manager::WriteJob>(
                                job_manager::WriteJob(
                                     *it,
                                     callback_ptr,
                                     this->journal_writer_ptr,
				     false
                                )
                        );
                job_ptr_list.push_back(job_ptr);
        }
        boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(done));
        boost::shared_ptr<job_manager::Job> job_ptr =
                boost::make_shared<job_manager::WriteJob>(
                         job_manager::WriteJob(
                              *it,
                              callback_ptr,
                              this->journal_writer_ptr,
			      false
                              )
                );
        job_ptr_list.push_back(job_ptr);
        this->queue_ptr->push_job(job_ptr_list);
}

void JournalManager::add_entry(boost::shared_ptr<recordStructure::Record> record, boost::function<void(void)> const & done)
{
	OSDLOG(DEBUG, "Adding entry in Journal");
	boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(done));
	boost::shared_ptr<job_manager::Job> job_ptr =
			boost::make_shared<job_manager::WriteJob>(
				job_manager::WriteJob(
					record,
					callback_ptr,
					this->journal_writer_ptr,
					true
				)
			);
	std::list<boost::shared_ptr<job_manager::Job> > job_ptr_list;
        job_ptr_list.push_back(job_ptr);
	this->queue_ptr->push_job(job_ptr_list);
}

} // namespace transaction_library
