#ifndef __JOURNAL_WRITER_7879_H__
#define __JOURNAL_WRITER_7879_H__

#include "libraryUtils/file_handler.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/config_file_parser.h"
#include "libraryUtils/journal.h"
#include "libraryUtils/serializor.h"

namespace journal_manager
{

using recordStructure::JournalOffset;

class ContainerJournalWriteHandler: public JournalWriteHandler
{
	public:
		ContainerJournalWriteHandler(std::string path, cool::SharedPtr<config_file::ConfigFileParser> parser_ptr, uint8_t node_id);
		~ContainerJournalWriteHandler();
//		void processWrite(Record * record);
//		void process_sync();
//		void rotate_file();
//		int64_t get_current_journal_offset();
//		JournalOffset get_last_commit_offset();
		std::string get_last_journal_file();
		void set_last_commit_offset(JournalOffset offset);
		void process_checkpoint();
		void process_snapshot();
		void process_write(recordStructure::Record * record, int64_t record_size);
		void process_start_container_flush(std::string path);
		void process_end_container_flush(std::string path, JournalOffset last_commit_offset);
                void process_accept_component_journal(std::list<int32_t> component_number_list);
		void remove_old_files();
	private:
};

}
#endif
