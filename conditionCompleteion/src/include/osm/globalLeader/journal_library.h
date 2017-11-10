#ifndef __GL_JOURNAL_LIBRARY__
#define __GL_JOURNAL_LIBRARY__

#include "osm/globalLeader/journal_writer.h"
#include "libraryUtils/job_manager.h"

namespace component_manager
{
	class ComponentMapManager;
}

namespace global_leader
{

class JournalLibrary
{
	public:
		JournalLibrary(
			std::string journal_path,
			//cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
			boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr,
			boost::function<std::vector<std::string>(void)> get_state_change_ids
			);
		void add_entry_in_journal(
			std::vector<boost::shared_ptr<recordStructure::Record> > gl_records,
			boost::function<void(void)> const & done
			);
		void prepare_transient_record_for_journal(
			std::string service_id,
			std::vector<boost::shared_ptr<recordStructure::Record> > & node_rec_records
			);
		boost::shared_ptr<journal_manager::GLJournalWriteHandler> get_journal_writer();
	private:
		boost::shared_ptr<job_manager::JobQueueMgr<job_manager::Job> > queue_ptr;
		boost::shared_ptr<job_manager::JobExecutor<job_manager::Job> > job_executor_ptr;
		boost::thread job_executor_thread;
		boost::shared_ptr<journal_manager::GLJournalWriteHandler> journal_writer_ptr;
		boost::shared_ptr<ServiceToRecordTypeMapper> service_to_record_type_map_ptr;
		boost::shared_ptr<component_manager::ComponentMapManager> component_map_mgr;
};

} // namespace global_leader

#endif
