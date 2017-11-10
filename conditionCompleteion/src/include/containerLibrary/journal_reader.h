#ifndef __CONTAINER_JOURNAL_READER_H_31023__
#define __CONTAINER_JOURNAL_READER_H_31023__

#include "libraryUtils/journal.h"
#include "containerLibrary/container_library.h"
#include "containerLibrary/updater_info.h"


namespace container_read_handler
{

using recordStructure::JournalOffset;

class ContainerJournalReadHandler: public journal_manager::JournalReadHandler
{               
	public: 
		ContainerJournalReadHandler(
				std::string path,
				boost::shared_ptr<container_library::MemoryManager> memory_obj,
				cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
			);
		~ContainerJournalReadHandler();
		void mark_unknown_components(std::list<int32_t>& component_list);
	protected:
/*		bool recover_last_checkpoint();
		bool recover_snapshot();
		bool set_recovering_offset();
*/		bool recover_objects(recordStructure::Record *record_ptr, std::pair<JournalOffset, int64_t> offset);
		bool process_snapshot();
		bool process_recovering_offset();
		bool process_last_checkpoint(int64_t last_checkpoint_offset);
	private:
		std::map<std::string, std::list<recordStructure::ObjectRecord> > temp_map;
		boost::shared_ptr<container_library::MemoryManager> memory_obj;
		int64_t next_read_offset;
		bool check_if_latest_journal();
};

class ContainerRecovery
{
	public:
		ContainerRecovery(
			std::string path,
			boost::shared_ptr<container_library::MemoryManager> memory_obj,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
		);
		bool perform_recovery(std::list<int32_t>& component_list);
		void clean_journal_files();
		uint64_t get_next_file_id() const;
		uint64_t get_current_file_id() const;
	private:
		boost::shared_ptr<ContainerJournalReadHandler> recovery_obj;
};

} // namespace container_read_handler

#endif
