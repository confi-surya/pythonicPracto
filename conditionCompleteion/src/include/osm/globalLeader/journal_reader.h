#ifndef __GL_JOURNAL_READER_H_987__
#define __GL_JOURNAL_READER_H_987__

#include "libraryUtils/journal.h"
#include "osm/globalLeader/component_manager.h"

namespace gl_journal_read_handler
{

using recordStructure::JournalOffset;

class GLJournalReadHandler: public journal_manager::JournalReadHandler
{               
	public: 
		GLJournalReadHandler(
				std::string path,
				boost::shared_ptr<component_manager::ComponentMapManager> map_mgr_ptr,
				cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
				std::vector<std::string> nodes_failed
			);
		~GLJournalReadHandler();
		void mark_unknown_components(std::list<int32_t> component_list);
		void clean_recovery_processes();
	protected:
/*		bool recover_last_checkpoint();
		bool recover_snapshot();
		bool set_recovering_offset();
*/		bool recover_objects(recordStructure::Record *record_ptr, std::pair<JournalOffset, int64_t> offset);
		bool process_snapshot();
		bool process_recovering_offset();
		bool process_last_checkpoint(int64_t last_checkpoint_offset);
	private:
		boost::shared_ptr<component_manager::ComponentMapManager> map_mgr_ptr;
		int64_t next_read_offset;
		//bool check_if_latest_journal();
		std::vector<std::string> state_change_ids;
		std::map<std::string, std::string> rec_process_map;
		std::vector<std::string> failed_node_list;		
		void unmount_journal_fs(std::string failed_node_id, std::string dest_node_id);
};

class GlobalLeaderRecovery
{
	public:
		GlobalLeaderRecovery(
				std::string path,
				boost::shared_ptr<component_manager::ComponentMapManager> map_mgr_ptr,
				cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
				std::vector<std::string> failed_node_list
			);
		bool perform_recovery();
		uint64_t get_current_file_id() const;
		uint64_t get_next_file_id() const;
		void clean_journal_files();
	private:
		boost::shared_ptr<GLJournalReadHandler> recovery_obj;
};


} // namespace gl_journal_read_handler

#endif
