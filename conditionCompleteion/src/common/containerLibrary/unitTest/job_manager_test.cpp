#include <boost/filesystem.hpp>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/job_manager.h"

namespace job_queue_mgr_test
{

TEST(job_queue_mgr_test, push_job_test)
{
	//template typename job_manager::JobQueueMgr<job_manager::Job> ;
	recordStructure::ObjectRecord record_obj(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false);
	boost::shared_ptr<recordStructure::FinalObjectRecord> record_ptr(
				new recordStructure::FinalObjectRecord("./acc/cont", record_obj)
				);
	boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer);

	cool::config::ConfigurationBuilder<config_file::ConfigFileParser> config_file_builder;
	config_file_builder.readFromFile("src/common/containerLibrary/config");
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = config_file_builder.buildConfiguration();

	boost::shared_ptr<journal_manager::JournalWriteHandler> journal_writer_ptr(new journal_manager::ContainerJournalWriteHandler(".", parser_ptr));
	boost::shared_ptr<job_manager::SyncJobs> sync_jobs_ptr(new job_manager::SyncJobs(journal_writer_ptr));
	job_manager::JobQueueMgr<job_manager::Job> job_queue_obj(sync_jobs_ptr);

	boost::shared_ptr<job_manager::WriteJob> write_job_obj1(new job_manager::WriteJob(record_ptr, callback_ptr, journal_writer_ptr));
	boost::shared_ptr<job_manager::WriteJob> write_job_obj2(new job_manager::WriteJob(record_ptr, callback_ptr, journal_writer_ptr));
	job_queue_obj.push_job(write_job_obj1);
	job_queue_obj.push_job(write_job_obj2);
	ASSERT_EQ(job_queue_obj.get_queue_size(), uint64_t(2));
	boost::filesystem::remove(boost::filesystem::path("./journal.log.1"));

}
/*
TEST(job_queue_mgr_test, get_job_test)
{
	boost::shared_ptr<recordStructure::ObjectRecord> record_ptr(
				new recordStructure::ObjectRecord(1, "abc", 21072014, 1024, "plain text", "cxwqbciuwe", false)
				);
	boost::shared_ptr<job_manager::CallbackMaintainer> callback_ptr(new job_manager::CallbackMaintainer);

	boost::shared_ptr<journal_manager::JournalWriteHandler> journal_writer_ptr(new journal_manager::JournalWriteHandler);
	boost::shared_ptr<job_manager::SyncJobs> sync_jobs_ptr(new job_manager::SyncJobs(journal_writer_ptr));
	job_manager::JobQueueMgr job_queue_obj(sync_jobs_ptr);
	boost::shared_ptr<job_manager::BatchJob> batch_job_ptr = boost::make_shared<job_manager::BatchJob>(job_queue_obj.get_job());

	boost::shared_ptr<job_manager::WriteJob> write_job_obj1(new job_manager::WriteJob(record_ptr, callback_ptr));
	job_queue_obj.push_job(write_job_obj1);

	ASSERT_FALSE(batch_job_ptr == NULL);

}*/

} // namespace job_queue_mgr_test
