#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/journal_library.h"

namespace journal_impl_test
{

TEST(journal_impl_test, constructor_test)
{

	container_library::JournalImpl obj;


/*	boost::shared_ptr<recordStructure::ObjectRecord> record_ptr(
				new recordStructure::ObjectRecord(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false)
				);
	boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer);

	boost::shared_ptr<journal_manager::JournalWriteHandler> journal_writer_ptr(new journal_manager::JournalWriteHandler);
	boost::shared_ptr<job_manager::SyncJobs> sync_jobs_ptr(new job_manager::SyncJobs(journal_writer_ptr));
	job_manager::JobQueueMgr job_queue_obj(sync_jobs_ptr);

	boost::shared_ptr<job_manager::WriteJob> write_job_obj1(new job_manager::WriteJob(record_ptr, callback_ptr));
	boost::shared_ptr<job_manager::WriteJob> write_job_obj2(new job_manager::WriteJob(record_ptr, callback_ptr));
//	job_queue_obj.push_job(write_job_obj1);

//	boost::shared_ptr<job_manager::BatchJob> batch_job_ptr = boost::make_shared<job_manager::BatchJob>(job_queue_obj.get_job());


        ASSERT_FALSE(batch_job_ptr == NULL);


}

} // namespace job_queue_mgr_test
