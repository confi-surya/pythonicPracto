#ifndef __JOB_MANAGER_H__
#define __JOB_MANAGER_H__

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition_variable.hpp>
#include <cool/debug.h>
#include "containerLibrary/memory_writer.h"
#include "containerLibrary/journal_writer.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/journal.h"
#include "libraryUtils/object_storage_log_setup.h"

using namespace object_storage_log_setup;

namespace job_manager
{

using recordStructure::JournalOffset;

class CallbackMaintainer
{
	public:
		boost::function<void(void)> success;
		void failure();
		void timeout();
		CallbackMaintainer(boost::function<void(void)> const & success = boost::function<void(void)>())
		:success(success)
		{}
};

class Job
{
	private:
		int job_id;
		boost::shared_ptr<CallbackMaintainer> callback_ptr;
	public:
		//boost::shared_ptr<recordStructure::FinalObjectRecord> record_ptr;
		boost::shared_ptr<recordStructure::Record> record_ptr;
		bool memory_flag;
		virtual ~Job() = 0;
		virtual void write() = 0;
		virtual void execute_callback() = 0;
//		recordStructure::ObjectRecord getObjectRecord() = 0;
//		CallbackMaintainer getCallback() = 0;

};

class WriteJob: public Job
{
	public:
/*		WriteJob(boost::shared_ptr<recordStructure::FinalObjectRecord> record,
				boost::shared_ptr<CallbackMaintainer> callback_obj,
				boost::shared_ptr<journal_manager::JournalWriteHandler> writer_ptr
		);
*/
		WriteJob(boost::shared_ptr<recordStructure::Record> record,
				boost::shared_ptr<CallbackMaintainer> callback_obj,
				boost::shared_ptr<journal_manager::JournalWriteHandler> writer_ptr,
				bool memory_flag
		);
		void write();
		void execute_callback();
//		recordStructure::ObjectRecord get_object_record();
//		CallbackMaintainer get_callback();
//		boost::shared_ptr<recordStructure::FinalObjectRecord> record_ptr;

	private:
		boost::shared_ptr<CallbackMaintainer> callback_ptr;
		uint64_t job_id;
		boost::shared_ptr<journal_manager::JournalWriteHandler> writer_ptr;
};

class SyncJobs
{
	public:
		SyncJobs(boost::shared_ptr<journal_manager::JournalWriteHandler> sync_ptr,
				boost::shared_ptr<container_library::MemoryManager> memory_ptr);
		SyncJobs(boost::shared_ptr<journal_manager::JournalWriteHandler> sync_ptr);
		void execute();
		JournalOffset get_last_sync_offset();
		boost::shared_ptr<container_library::MemoryManager> get_memory_ptr();
	private:
		uint64_t job_id;
		boost::shared_ptr<journal_manager::JournalWriteHandler> sync_ptr;
		boost::shared_ptr<container_library::MemoryManager> memory_ptr;
};

template<typename JobType>
class BatchJob
{
	private:
		std::list<boost::shared_ptr<JobType> > tasks;
		boost::shared_ptr<SyncJobs> sync_jobs_ptr;
		void remove_list();
		
	public:
		BatchJob(std::list<boost::shared_ptr<JobType> > tasks, boost::shared_ptr<SyncJobs> sync_jobs_ptr);
		void execute_for_container();
		void execute_for_transaction();
		void execute();
};

template<typename JobType>
void BatchJob<JobType>::execute()
{
	try
	{
		typedef typename std::list<boost::shared_ptr<JobType> >::iterator IT;
		OSDLOG(INFO, "Writing " << this->tasks.size() << " batch jobs in Journal");	
		for(IT it = this->tasks.begin(); it != this->tasks.end(); it++)
		{
			(*it)->write();
		}
		this->sync_jobs_ptr->execute();
		for(IT it = this->tasks.begin(); it != this->tasks.end(); it++)
		{
			(*it)->execute_callback();
		}
	}
	catch(...)
	{
		OSDLOG(FATAL, "Failed to write/sync jobs in Journal due to exception raise");
		PASSERT(false, "Failed to write jobs in Journal due to exception raise");
	}

}

template<typename JobType>
void BatchJob<JobType>::execute_for_container()
{
	typedef typename std::list<boost::shared_ptr<JobType> >::iterator IT;
	OSDLOG(INFO, osd_container_lib_log_tag << "Writing " << this->tasks.size() << " batch jobs in Journal");

	//record type1 is the object records comming from update_container interface
	std::pair<bool,std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> > > record_type1;
	record_type1.first = true;

	//record_type2 is the object records comming from accept_container interface
	std::pair<bool,std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> > > record_type2;
	record_type2.first = false;
	
	std::list<std::pair<bool,std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> > > >::iterator it1;
	//it1->second.reserve(this->tasks.size());
	//for(std::list<boost::shared_ptr<JobType> >::iterator it = this->tasks.begin(); it != this->tasks.end(); it++)
	
	for(IT it = this->tasks.begin(); it != this->tasks.end(); it++)
	{
		try
		{
			(*it)->write();
			if ( (*it)->memory_flag )
			{
				record_type1.second.push_back(boost::dynamic_pointer_cast<recordStructure::FinalObjectRecord>((*it)->record_ptr));
			}
			else
			{
				record_type2.second.push_back(boost::dynamic_pointer_cast<recordStructure::FinalObjectRecord>((*it)->record_ptr));
			}
		}
		catch(...)
		{
			OSDLOG(FATAL, osd_container_lib_log_tag << "Journal writing Failed in execute_for_container");
			PASSERT(false, "Journal writing Failed in execute_for_container"); 	
		}
	}
	std::list<std::pair<bool,std::vector<boost::shared_ptr<recordStructure::FinalObjectRecord> > > > records;
	if ( record_type1.second.size() > 0 )
	{
		records.push_back(record_type1);
	}
	if ( record_type2.second.size() > 0 )
	{
		records.push_back(record_type2);
	}	
	try
	{	
		JournalOffset prev_off = this->sync_jobs_ptr->get_last_sync_offset();
		OSDLOG(DEBUG, osd_container_lib_log_tag << "Syncing Journal, previous sync offset is: " << prev_off);
		this->sync_jobs_ptr->execute();
		JournalOffset sync_off = this->sync_jobs_ptr->get_last_sync_offset();
		OSDLOG(DEBUG, osd_container_lib_log_tag << "Sync complete, new sync offset is: " << sync_off << ". Now executing callbacks");
		this->sync_jobs_ptr->get_memory_ptr()->add_entries(records, prev_off, sync_off);
	
		for(IT it = this->tasks.begin(); it != this->tasks.end(); it++)
		{
			(*it)->execute_callback();
		}
		this->remove_list();
	}
	catch(...)
	{
		OSDLOG(FATAL, osd_container_lib_log_tag << "Journal sync Failed in execute_for_container");
		PASSERT(false, "Journal sync Failed in execute_for_container");
	}
}

template<typename JobType>
void BatchJob<JobType>::execute_for_transaction()
{
	try
	{
		typedef typename std::list<boost::shared_ptr<JobType> >::iterator IT;
		OSDLOG(INFO, osd_transaction_lib_log_tag << "Writing " << this->tasks.size() << " batch jobs in Journal");	
		for(IT it = this->tasks.begin(); it != this->tasks.end(); it++)
		{
			(*it)->write();
		}
		this->sync_jobs_ptr->execute();
		for(IT it = this->tasks.begin(); it != this->tasks.end(); it++)
		{
			(*it)->execute_callback();
		}
	}
	catch(...)
	{
		OSDLOG(FATAL, osd_transaction_lib_log_tag << "Failed to write/sync jobs in Journal due to exception raise");
		PASSERT(false, "Failed to write jobs in Journal due to exception raise");
	}
}

template<typename JobType>
void BatchJob<JobType>::remove_list()
{
	this->tasks.clear();
}

template<typename JobType>
BatchJob<JobType>::BatchJob(std::list<boost::shared_ptr<JobType> > tasks, boost::shared_ptr<SyncJobs> sync_jobs_ptr):tasks(tasks), sync_jobs_ptr(sync_jobs_ptr)
{

}


template<typename JobType>
class JobQueueMgr
{
	private:
		std::list<boost::shared_ptr<JobType> > tasks;
		boost::condition_variable taskPresentCondition;
		boost::mutex mtx;
		boost::shared_ptr<SyncJobs> sync_jobs_ptr;
	public:
		JobQueueMgr(boost::shared_ptr<SyncJobs> sync_jobs_ptr);
		void push_job(std::list<boost::shared_ptr<JobType> > job_ptr_list);
		boost::shared_ptr<JobType> pop_job();
		boost::shared_ptr<BatchJob<JobType> > get_job();
		uint64_t get_queue_size();		// Testing interface
		bool stop;
		void stop_queue();
};

template<typename JobType>
JobQueueMgr<JobType>::JobQueueMgr(boost::shared_ptr<SyncJobs> sync_jobs_ptr):sync_jobs_ptr(sync_jobs_ptr), stop(false)
{

}

template<typename JobType>
uint64_t JobQueueMgr<JobType>::get_queue_size()
{
	return this->tasks.size();
}

template<typename JobType>
void JobQueueMgr<JobType>::push_job(std::list<boost::shared_ptr<JobType> > job_ptr_list)
{
	boost::mutex::scoped_lock lock(mtx);
	typename std::list<boost::shared_ptr<JobType> >::iterator it = job_ptr_list.begin();
	for ( ; it != job_ptr_list.end(); it++ )
	{
		this->tasks.push_back(*it);
	}
	this->taskPresentCondition.notify_one();
}

template<typename JobType>
void JobQueueMgr<JobType>::stop_queue()
{
	OSDLOG(INFO, "JobQueueMgr<JobType>::stop_queue()");
	this->stop = true;
	this->taskPresentCondition.notify_one();
}

//TODO: obsolete
template<typename JobType>
boost::shared_ptr<JobType> JobQueueMgr<JobType>::pop_job()
{
	return this->tasks.front();
}

template<typename JobType>
boost::shared_ptr<BatchJob<JobType> > JobQueueMgr<JobType>::get_job()
{
	boost::mutex::scoped_lock lock(mtx);
	boost::shared_ptr<BatchJob<JobType> > ob;
	while (this->tasks.empty() and (!this->stop))
	{
		this->taskPresentCondition.wait(lock);
	}
	if (!this->tasks.empty())
	{
		ob.reset(new BatchJob<JobType>(this->tasks, this->sync_jobs_ptr));
		this->tasks.clear();
	}
	return ob;
}

template<typename JobType>
class JobExecutor
{
	boost::shared_ptr<JobQueueMgr<JobType> > job_queue_ptr;
	public:
		JobExecutor(boost::shared_ptr<JobQueueMgr<JobType> > job_queue_ptr);
		void execute_for_container();
		void execute_for_transaction();
		void execute_for_global_leader();
		void stop();
		
};

template<typename JobType>
JobExecutor<JobType>::JobExecutor(boost::shared_ptr<JobQueueMgr<JobType> > queue_ptr):job_queue_ptr(queue_ptr)
{
}

template<typename JobType>
void JobExecutor<JobType>::stop()
{
	this->job_queue_ptr->stop_queue();
}

template<typename JobType>
void JobExecutor<JobType>::execute_for_container()
{
	while(1)
	{
	{
		boost::shared_ptr<BatchJob<JobType> > batch_job_ptr = this->job_queue_ptr->get_job();
		if (batch_job_ptr)
		{
			batch_job_ptr->execute_for_container();
		}
		else
		{
			break;
		}
	}
	}
}

template<typename JobType>
void JobExecutor<JobType>::execute_for_transaction()
{
	while(1)
	{
	{
		boost::shared_ptr<BatchJob<JobType> > batch_job_ptr = this->job_queue_ptr->get_job();
		if (batch_job_ptr)
		{
			batch_job_ptr->execute_for_transaction();
		}
		else
		{
			break;
		}
	}
	}
	
}

template<typename JobType>
void JobExecutor<JobType>::execute_for_global_leader()
{
	while(1)
	{
	{
		boost::shared_ptr<BatchJob<JobType> > batch_job_ptr = this->job_queue_ptr->get_job();
		if (batch_job_ptr)
		{
			batch_job_ptr->execute();
		}
		else
		{
			break;
		}
	}
	}
}

} // namespace job_manager

#endif
