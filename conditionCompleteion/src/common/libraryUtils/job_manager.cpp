//#include "containerLibrary/job_manager.h"
#include "libraryUtils/job_manager.h"

namespace job_manager
{


WriteJob::WriteJob(
	//boost::shared_ptr<recordStructure::FinalObjectRecord> record_ptr,
	boost::shared_ptr<recordStructure::Record> record_ptr,
	boost::shared_ptr<CallbackMaintainer> callback_ptr,
	boost::shared_ptr<journal_manager::JournalWriteHandler> writer_ptr,
	bool memory_flag
) :
	callback_ptr(callback_ptr),
	writer_ptr(writer_ptr)
{
	this->record_ptr = record_ptr;
	this->memory_flag = memory_flag;
}

Job::~Job()
{
}

void WriteJob::write()
{
	this->writer_ptr->write(this->record_ptr.get());
}

void WriteJob::execute_callback()
{
	if (this->callback_ptr->success)
	{
		this->callback_ptr->success();
		OSDLOG(DEBUG, "WriteJob::execute_callback complete");
	}
}


SyncJobs::SyncJobs(boost::shared_ptr<journal_manager::JournalWriteHandler> sync_ptr,
		boost::shared_ptr<container_library::MemoryManager> memory_ptr)
	: sync_ptr(sync_ptr), memory_ptr(memory_ptr)
{
}

SyncJobs::SyncJobs(boost::shared_ptr<journal_manager::JournalWriteHandler> sync_ptr): sync_ptr(sync_ptr)
{
}

void SyncJobs::execute()
{
	OSDLOG(DEBUG, "SyncJobs::execute-----------------------------------");
	this->sync_ptr->sync();
}

boost::shared_ptr<container_library::MemoryManager> SyncJobs::get_memory_ptr()
{
	return this->memory_ptr;
}

JournalOffset SyncJobs::get_last_sync_offset()
{
	return this->sync_ptr->get_last_sync_offset();
}

void CallbackMaintainer::failure()
{
}

void CallbackMaintainer::timeout()
{
}


} // namespace job_manager
