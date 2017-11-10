#ifndef __GL_JOURNAL_HANDLER_845__
#define __GL_JOURNAL_HANDLER_845__

#include "libraryUtils/journal.h"
#include "osm/globalLeader/component_manager.h"

namespace journal_manager
{

#ifndef OBJECT_STORAGE_CONFIG
	static const std::string JOURNAL_PATH = "/export/global_leader_journal";
#else
	static const std::string JOURNAL_PATH = "/remote/hydra042/gmishra/journal_dir";
#endif

class GLJournalWriteHandler : public JournalWriteHandler
{
	public:
		GLJournalWriteHandler(
			std::string path,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
			boost::shared_ptr<component_manager::ComponentMapManager> map_ptr,
			boost::function<std::vector<std::string>(void)> get_state_change_ids
			);
		void process_snapshot();
		void process_checkpoint();
		void process_write(recordStructure::Record * record, int64_t record_size);
		void remove_old_files();
		void remove_all_journal_files();
	private:
		uint64_t latest_state_change_id;
		boost::shared_ptr<component_manager::ComponentMapManager> map_ptr;
		boost::function<std::vector<std::string>(void)> get_state_change_ids;
};

} // namespace journal_manager

#endif
