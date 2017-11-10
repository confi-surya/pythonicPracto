#ifndef __JOURNAL_LIBRARY_H__
#define __JOURNAL_LIBRARY_H__

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "journal_writer.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/job_manager.h"
#include "memory_writer.h"

namespace container_library
{

//class job_manager::Job; //not required as class is visible after including "libraryUtils/job_manager.h"
using recordStructure::JournalOffset;

class JournalInterface
{
	public:
		virtual void add_entry_in_queue(
			std::string const & path,
			recordStructure::ObjectRecord & record_info_obj,
			boost::function<void(void)> const & done
		) = 0;
		virtual void add_bulk_delete_in_queue(
                        std::list<recordStructure::FinalObjectRecord>& object_records,
                        boost::function<void(void)> const & done
                ) = 0;
		virtual void add_recovered_entry_in_queue(
			std::list<recordStructure::FinalObjectRecord>& object_records,
			boost::function<void(void)> const & done
		) = 0;
		virtual ~JournalInterface() = 0;
		virtual boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> get_journal_writer() = 0;
//		virtual void flushQueueInJournal(JobQueue & queueObject) = 0;
//		virtual void flushMarkerInJournal(std::string const & marker) = 0;
};

class JournalImpl: public JournalInterface
{
	boost::thread job_executor_thread;
	boost::shared_ptr<MemoryManager> memory_ptr;
	boost::shared_ptr<job_manager::SyncJobs> sync_jobs_ptr;
	boost::shared_ptr<job_manager::JobQueueMgr<job_manager::Job> > queue_ptr;
	boost::shared_ptr<job_manager::JobExecutor<job_manager::Job> > job_executor_ptr;
	boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> journal_writer_ptr;
	public:
		JournalImpl(std::string journal_path, boost::shared_ptr<MemoryManager> memory_ptr,
				cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,uint8_t node_id);
		~JournalImpl();
		void add_entry_in_queue(
			std::string const & path,
			recordStructure::ObjectRecord & record_info_obj,
			boost::function<void(void)> const & done
		);
		void add_bulk_delete_in_queue(
                        std::list<recordStructure::FinalObjectRecord>& object_records,
                        boost::function<void(void)> const & done
                );
                void add_recovered_entry_in_queue(
                        std::list<recordStructure::FinalObjectRecord>& object_records,
                	boost::function<void(void)> const & done
		);
		boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> get_journal_writer();
//		void flushQueueInJournal(QueueMaintainer & queueObject);
//		void flushMarkerInJournal(std::string const & marker);
//		void executeCallback();
};

} // namespace container_library

#endif
