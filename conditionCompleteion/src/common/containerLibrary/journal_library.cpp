#include "containerLibrary/journal_library.h"

namespace container_library
{

JournalInterface::~JournalInterface()
{
}

JournalImpl::~JournalImpl()
{
	OSDLOG(DEBUG,  "JournalImpl destructor called");
	this->job_executor_ptr->stop();
	this->job_executor_thread.join();	
	OSDLOG(DEBUG,  "Job Executor joined, destruction of JournalImpl complete");
}

JournalImpl::JournalImpl(std::string journal_path, boost::shared_ptr<MemoryManager> memory_ptr,
		cool::SharedPtr<config_file::ConfigFileParser> parser_ptr, uint8_t node_id)
	: memory_ptr(memory_ptr)
{
	OSDLOG(DEBUG,  "Constructing JournalImpl");
	this->journal_writer_ptr.reset(new journal_manager::ContainerJournalWriteHandler(journal_path, parser_ptr,node_id));
	this->sync_jobs_ptr.reset(new job_manager::SyncJobs(this->journal_writer_ptr, this->memory_ptr));
	this->queue_ptr.reset(new job_manager::JobQueueMgr<job_manager::Job>(this->sync_jobs_ptr));
	this->job_executor_ptr.reset(new job_manager::JobExecutor<job_manager::Job>(this->queue_ptr));
	this->job_executor_thread = boost::thread(boost::bind(&job_manager::JobExecutor<job_manager::Job>::execute_for_container, this->job_executor_ptr));
	OSDLOG(DEBUG,  "Job Executor created, JournalImpl construction complete");
}

boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> JournalImpl::get_journal_writer()
{
	return this->journal_writer_ptr;
}

void JournalImpl::add_recovered_entry_in_queue(
	std::list<recordStructure::FinalObjectRecord>& object_records,
	boost::function<void(void)> const & done
)
{
	//Add all the records in the Job queue
	// Is the callback required for this function
	OSDLOG(INFO,"Start adding recovered entries in queue ");
	std::list<boost::shared_ptr<job_manager::Job> > job_ptr_list;
	std::list<recordStructure::FinalObjectRecord>::iterator it = object_records.begin();
	for ( ;it != (--object_records.end()); it++ )
	{
		boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(NULL));
		boost::shared_ptr<job_manager::Job> job_ptr = 
                        boost::make_shared<job_manager::WriteJob>(
                                job_manager::WriteJob(
                                     boost::make_shared<recordStructure::FinalObjectRecord>(*it),
                                     callback_ptr,
                                     this->journal_writer_ptr,
				     false	// False specifies that this is the data owned by any other container service
                                )
                        );
		job_ptr_list.push_back(job_ptr);	
	}
	boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(done));
	boost::shared_ptr<job_manager::Job> job_ptr =
                boost::make_shared<job_manager::WriteJob>(
                         job_manager::WriteJob(
                              boost::make_shared<recordStructure::FinalObjectRecord>(*it),
                              callback_ptr,
                              this->journal_writer_ptr,
			      false
                              )
                );
        job_ptr_list.push_back(job_ptr);
	this->queue_ptr->push_job(job_ptr_list);

}

void JournalImpl::add_bulk_delete_in_queue(
        std::list<recordStructure::FinalObjectRecord>& object_records,
        boost::function<void(void)> const & done
)
{
	//Add all the records in the Job queue
	//Is the callback required for this function
	std::list<boost::shared_ptr<job_manager::Job> > job_ptr_list;
        std::list<recordStructure::FinalObjectRecord>::iterator it = object_records.begin();
        for ( ;it != (--object_records.end()); it++ )
        {
                boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(NULL));
                boost::shared_ptr<job_manager::Job> job_ptr =
                        boost::make_shared<job_manager::WriteJob>(
                                job_manager::WriteJob(
                                     boost::make_shared<recordStructure::FinalObjectRecord>(*it),
                                     callback_ptr,
                                     this->journal_writer_ptr,
				     true
				)
                        );
                job_ptr_list.push_back(job_ptr);
        }
        boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(done));
        boost::shared_ptr<job_manager::Job> job_ptr =
                boost::make_shared<job_manager::WriteJob>(
                         job_manager::WriteJob(
                              boost::make_shared<recordStructure::FinalObjectRecord>(*it),
                              callback_ptr,
                              this->journal_writer_ptr,
			      true
			)
                );
        job_ptr_list.push_back(job_ptr);
        this->queue_ptr->push_job(job_ptr_list);
}

void JournalImpl::add_entry_in_queue(
	std::string const & path,
	recordStructure::ObjectRecord & record_info_obj,
	boost::function<void(void)> const & done
)
{
	recordStructure::FinalObjectRecord record_obj(path, record_info_obj);
	boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer(done));
	boost::shared_ptr<job_manager::Job> job_ptr = 
			boost::make_shared<job_manager::WriteJob>(
				job_manager::WriteJob(
					boost::make_shared<recordStructure::FinalObjectRecord>(record_obj),
					callback_ptr,
					this->journal_writer_ptr,
					true
				)
			);
	std::list<boost::shared_ptr<job_manager::Job> > job_ptr_list;
	job_ptr_list.push_back(job_ptr);
	this->queue_ptr->push_job(job_ptr_list);
}


} // namespace ContainerLibrary
