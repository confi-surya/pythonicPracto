#ifndef __JOURNAL_H_2323__
#define __JOURNAL_H_2323__

#include <boost/thread/mutex.hpp>

#include "libraryUtils/file_handler.h"
#include "libraryUtils/serializor.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/config_file_parser.h"

#define SNAPSHOT_OFFSET 0
#define MAX_READ_CHUNK_SIZE 65536
#define RECORDS_READ_CHUNK_SIZE 524288

#define MAX_ID_LIMIT 72057594037927935

namespace journal_manager
{

typedef recordStructure::JournalOffset JournalOffset;

const static std::string JOURNAL_FILE_NAME = "journal.log";

class JournalReadHandler
{
	public:
		JournalReadHandler(std::string path, cool::SharedPtr<config_file::ConfigFileParser> parser_ptr);
		virtual ~JournalReadHandler();
		void init();
		void init_journal();
		bool recover_last_checkpoint();
		virtual bool process_snapshot() = 0;
		void recover_snapshot();
		void set_recovering_offset();
		void recover_till_eof();
		void recover_journal_till_eof();
		virtual bool process_recovering_offset() = 0;
		virtual bool recover_objects(recordStructure::Record * record, std::pair<JournalOffset, int64_t> offset) = 0;
		virtual bool process_last_checkpoint(int64_t last_checkpoint_offset) = 0;
		void find_latest_journal_file(std::string path);
		uint64_t get_node_id_from_unique_id(uint64_t);
		JournalOffset get_last_commit_offset();
		void clean_journal_files();
		uint64_t get_next_file_id() const;
		uint64_t get_current_file_id() const;

	protected:
		boost::shared_ptr<serializor_manager::Deserializor> deserializor;
		boost::shared_ptr<fileHandler::FileHandler> file_handler;
		int64_t checkpoint_interval;
		JournalOffset last_commit_offset;
		JournalOffset last_checkpoint_offset;
		uint64_t latest_file_id;
		uint64_t next_file_id;
		uint64_t oldest_file_id;
		virtual bool check_if_latest_journal();
		std::string path;

		// for recovery
		void prepare_read_record();
		std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > get_next_record();
		std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > get_next_journal_record();
	private:
		std::vector<char> buffer;
		char * cur_buffer_ptr;
		uint32_t rest_size;
		JournalOffset file_offset;
		bool end_of_file;
		void skip_padding();
};



class JournalWriteHandler
{
	public:
		JournalWriteHandler(std::string path, cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,uint8_t node_id);
		virtual ~JournalWriteHandler();
		bool write(recordStructure::Record * record);
		void rotate_file();
		bool sync();
		JournalOffset get_last_sync_offset();
		JournalOffset get_current_journal_offset();
		void  set_active_file_id(uint64_t);
		void  set_upgrade_active_file_id();
		void  rename_file();
		uint64_t get_unique_file_id(uint64_t, uint64_t);
		virtual void process_snapshot() = 0;
		virtual void process_write(recordStructure::Record * record, int64_t record_size) = 0;
		virtual void process_checkpoint() = 0;
		virtual void remove_old_files() = 0;
		void close_journal();
	protected:
		void add_padding_bytes(int64_t bytes);
		uint64_t active_file_id;
		serializor_manager::Serializor * serializor;
		boost::shared_ptr<fileHandler::FileHandler> file_handler;
		int64_t max_record_length;
		int64_t next_checkpoint_offset;
		int64_t max_file_length;
		JournalOffset last_commit_offset;
		JournalOffset last_sync_offset;
		int64_t checkpoint_interval;
		std::string path;
		uint64_t oldest_file_id;
	private:
		boost::mutex mtx;
};

std::string const make_journal_path(std::string const path, uint64_t const file_id);
/* {
	return path + std::string("/journal.log.") + boost::lexical_cast<std::string>(file_id);
}*/

}// journal namespace ends
#endif
