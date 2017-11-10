#include <iostream>
#include <boost/filesystem.hpp>

#include <cool/debug.h>
#include "libraryUtils/journal.h"
#include "libraryUtils/file_handler.h"
#include "cool/cool.h"

#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/file_handler.h"
#include "libraryUtils/object_storage_exception.h"

using namespace object_storage_log_setup;
using namespace OSDException;

namespace journal_manager
{

JournalReadHandler::JournalReadHandler(
		std::string path,
		cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
		)
	: path(path)
{

	this->find_latest_journal_file(path);
	this->deserializor.reset(new serializor_manager::Deserializor());
	this->last_commit_offset = std::make_pair(0,0);
	this->last_checkpoint_offset = std::make_pair(0,0);
	OSDLOG(INFO, "Latest Recovery file id found is " << this->latest_file_id);
	this->checkpoint_interval = parser_ptr->checkpoint_intervalValue();
	this->buffer.resize(MAX_READ_CHUNK_SIZE);
	this->cur_buffer_ptr = &this->buffer[0];
	OSDLOG(DEBUG, "JournalReadHandler constructed");
}

void JournalReadHandler::init()
{
	OSDLOG(DEBUG, "Journal path is: " << make_journal_path(this->path, this->latest_file_id));
	this->file_handler.reset(new fileHandler::FileHandler(make_journal_path(this->path, this->latest_file_id), fileHandler::OPEN_FOR_READ));
}

void JournalReadHandler::init_journal()
{
	OSDLOG(DEBUG, "Journal path is: " << make_journal_path(this->path, this->latest_file_id));
        this->file_handler.reset(new fileHandler::FileHandler(make_journal_path(this->path, this->latest_file_id), fileHandler::OPEN_FOR_READ_WRITE));
}



void JournalReadHandler::find_latest_journal_file(std::string journal_path)
{

	namespace fs = boost::filesystem;
	fs::path someDir(journal_path);
	fs::directory_iterator end_iter;

	this->latest_file_id = 0;
	this->next_file_id = 1;
	this->oldest_file_id = std::numeric_limits<uint64_t>::max();

	for( fs::directory_iterator dir_iter(someDir) ; dir_iter != end_iter ; ++dir_iter)
	{
		size_t pos = dir_iter->path().string().find(JOURNAL_FILE_NAME);
		if (fs::is_regular_file(dir_iter->path()) && pos != std::string::npos)
		{
			string suffix = dir_iter->path().string().substr(pos + JOURNAL_FILE_NAME.size() + 1);
			uint64_t number = boost::lexical_cast<uint64_t>(suffix);
			this->oldest_file_id = ::min(this->oldest_file_id, number);
			if (fs::file_size(dir_iter->path()))
			{
				OSDLOG(DEBUG, "Journal file with ID " << number << " is not empty");
				this->latest_file_id = ::max(this->latest_file_id, number);
				this->next_file_id = ::max(this->next_file_id, number + 1);
			}
			else
			{
				OSDLOG(DEBUG, "Journal file with ID " << number << " is empty");
				this->next_file_id = ::max(this->next_file_id, number);
			}
		}
	OSDLOG(DEBUG,"Oldest File Id : " << this->oldest_file_id << "Latest File Id : " << this->latest_file_id);
	}
}

JournalReadHandler::~JournalReadHandler()
{
}

JournalOffset JournalReadHandler::get_last_commit_offset()
{
	return this->last_commit_offset;
}


bool JournalReadHandler::recover_last_checkpoint()
{
	OSDLOG(DEBUG, "Recovering checkpoint");
	int64_t length = this->file_handler->get_end_offset();
	int64_t offset = ((length-1)/this->checkpoint_interval) * this->checkpoint_interval;
	this->last_checkpoint_offset = std::make_pair(latest_file_id, offset);
	OSDLOG(INFO, "Last checkpoint offset is : " << this->last_checkpoint_offset);
	return this->process_last_checkpoint(offset);
}

void JournalReadHandler::clean_journal_files()
{
	if (this->oldest_file_id == std::numeric_limits<uint64_t>::max())
		return;
	this->file_handler.reset();
	OSDLOG(DEBUG,"Oldest File ID : " << this->oldest_file_id << "Latest File ID : " << this->latest_file_id);
	for (uint64_t i = this->oldest_file_id; i <= this->latest_file_id; i++)
	{
		OSDLOG(INFO, "Cleaning Journal file " << make_journal_path(this->path, i) << " after recovery");
		boost::filesystem::remove(make_journal_path(this->path, i));
	}

}

uint64_t JournalReadHandler::get_next_file_id() const
{
	return this->next_file_id;
}

uint64_t JournalReadHandler::get_current_file_id() const
{
	return this->latest_file_id;
}

void JournalReadHandler::recover_snapshot()
{
	OSDLOG(INFO, "Recovering snapshot");
	this->file_handler->seek(SNAPSHOT_OFFSET);
	this->process_snapshot();
}

void JournalReadHandler::set_recovering_offset()
{
	this->process_recovering_offset();
}

void JournalReadHandler::prepare_read_record()
{
	this->buffer.resize(MAX_READ_CHUNK_SIZE+1);
	this->rest_size = 0;
	this->end_of_file = false;
	this->file_offset = std::make_pair(this->last_commit_offset.first, this->file_handler->tell());
}

inline void JournalReadHandler::skip_padding()
{
	OSDLOG(INFO, "Skipping padding");
	while (this->rest_size > 0 && *this->cur_buffer_ptr == fileHandler::PADDING)
	{
		this->rest_size --;
		this->cur_buffer_ptr ++;
		this->file_offset.second += 1;
	}
}

std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > JournalReadHandler::get_next_journal_record()
{
        this->skip_padding();
        recordStructure::Record * record = NULL;
        if (this->rest_size < serializor_manager::HEADER_SIZE
                        || this->rest_size < this->deserializor->get_record_size(this->cur_buffer_ptr))
        {
                if (this->end_of_file)
                {
                        return std::make_pair(record, std::make_pair(this->file_offset, 0));
                }                                                       
                this->buffer.resize(RECORDS_READ_CHUNK_SIZE);
                ::memcpy(&this->buffer[0], &this->buffer[RECORDS_READ_CHUNK_SIZE - rest_size], rest_size);
                int64_t start_reading_offset = this->file_handler->tell();
		int64_t read_record_size = this->file_handler->read(&this->buffer[0] + this->rest_size, RECORDS_READ_CHUNK_SIZE - this->rest_size);
                DASSERT(read_record_size != -1, "read failure");
                this->rest_size += read_record_size;
                this->cur_buffer_ptr = &this->buffer[0];
                int64_t record_size = this->deserializor->get_record_size(this->cur_buffer_ptr); 
                OSDLOG(DEBUG,"Record size is : " << record_size);
                while ( this->rest_size < record_size )
                {
                        OSDLOG(DEBUG,"Continue Reading File");
                        this->buffer.resize(this->rest_size + RECORDS_READ_CHUNK_SIZE);
			int64_t read_bytes = this->file_handler->read(&this->buffer[0] + this->rest_size, RECORDS_READ_CHUNK_SIZE);
                        this->rest_size += read_bytes;
                        DASSERT(read_bytes != -1, "read failure");
                        read_record_size += read_bytes;
                        //TODO aparauliya, Below condition is not suffucient for end of file. it can
                        //also hit in case of HFS is not able to read desirable chunk size
                        if ( read_bytes != RECORDS_READ_CHUNK_SIZE )
                        {
                                this->end_of_file = true;
                                OSDLOG(INFO,"End of Journal File");
                                break;
                        }
                }
                if ( !(this->rest_size < record_size) )
                {
                        OSDLOG(INFO, "File handle at offset: " << this->file_handler->tell());
                        this->cur_buffer_ptr = &this->buffer[0];
                        this->skip_padding();
                        if (this->end_of_file == true and this->rest_size == 0)
                        {
                                OSDLOG(INFO, "File decoding complete");
                                return std::make_pair(record, std::make_pair(this->file_offset, 0));
                        }
                }
                else
                {
                        if (this->end_of_file == true)
                        {
                                if ( this->rest_size == 0)
                                {
                                        OSDLOG(INFO, "File decoding complete");
                                        return std::make_pair(record, std::make_pair(this->file_offset, 0));
                                }
                                OSDLOG(INFO,"TRUNCATING FILE");
                                //Truncate the file from the last read offset
                                int ret = this->file_handler->truncate(start_reading_offset);
                                if ( ret != -1 )
                                {
                                        this->file_handler->seek(start_reading_offset);
                                        return std::make_pair(record, std::make_pair(this->file_offset, 0));
                                }
                                else
                                {
                                        DASSERT(ret != -1, "Truncate failure");
                                }
                        }
                        else
                        {
                                OSDLOG(FATAL, "Journal File Corrupted. Recovery is not possible");
                                PASSERT(false, "Journal File Corrupted. Recovery is not possible");
                                return std::make_pair(record, std::make_pair(this->file_offset, 0));
                        }

                }
       }
        int64_t record_size = this->deserializor->get_record_size(this->cur_buffer_ptr);
        if (this->rest_size < serializor_manager::HEADER_SIZE || this->rest_size < record_size and this->rest_size != 0)
        {
                OSDLOG(FATAL, "Remaining buffer size less than header size");
                PASSERT(false, "Remaining buffer size less than header size");
                return std::make_pair(record, std::make_pair(this->file_offset, 0));
        }
        if (record_size == -1)
        {
                OSDLOG(INFO, "Invalid record size (-1) while recovering from journal");
                return std::make_pair(record, std::make_pair(this->file_offset,0));
        }
	OSDLOG(DEBUG, "Record size is " << record_size);
        record = this->deserializor->deserialize(this->cur_buffer_ptr, record_size);
        if (record == NULL)
        {
                this->file_handler->truncate(this->last_checkpoint_offset.second);
                PASSERT(false, "Invalid record while recovering from journal");
                return std::make_pair(record, std::make_pair(this->file_offset,0));
        }

        //OSDLOG(INFO,"REST SIZE IS 2: " << this->rest_size);
        JournalOffset offset = this->file_offset;
        this->rest_size -= record_size;
        this->cur_buffer_ptr += record_size;
        this->file_offset.second += record_size;
        return std::make_pair(record, std::make_pair(offset, record_size));
}

//std::pair<recordStructure::Record *, JournalOffset> JournalReadHandler::get_next_record()
std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > JournalReadHandler::get_next_record()
{
	this->skip_padding();
	recordStructure::Record * record = NULL;
	if (this->rest_size < serializor_manager::HEADER_SIZE
			|| this->rest_size < this->deserializor->get_record_size(this->cur_buffer_ptr))
	{
		if (this->end_of_file)
		{
			return std::make_pair(record, std::make_pair(this->file_offset, 0));
		}
		::memcpy(&this->buffer[0], &this->buffer[MAX_READ_CHUNK_SIZE - rest_size], rest_size);
		int64_t read_bytes = this->file_handler->read(&this->buffer[0] + this->rest_size, MAX_READ_CHUNK_SIZE - this->rest_size);
		OSDLOG(INFO, "File handle at offset: " << this->file_handler->tell());
		DASSERT(read_bytes != -1, "read failure");
		if (read_bytes != MAX_READ_CHUNK_SIZE - rest_size)
		{
			this->end_of_file = true;
		}
		this->buffer.resize(read_bytes + rest_size);
		this->cur_buffer_ptr = &this->buffer[0];
		this->rest_size += read_bytes;
		this->skip_padding();
		if (this->end_of_file == true and this->rest_size == 0)
		{
			OSDLOG(INFO, "File decoding complete");
			return std::make_pair(record, std::make_pair(this->file_offset, 0));
		}
	}
	int64_t record_size = this->deserializor->get_record_size(this->cur_buffer_ptr);
	if ((this->rest_size < serializor_manager::HEADER_SIZE) || (this->rest_size < record_size and this->rest_size != 0))
	{
		OSDLOG(FATAL, "Remaining buffer size less than header size");
		PASSERT(false, "Remaining buffer size less than header size");
		return std::make_pair(record, std::make_pair(this->file_offset, 0));
	}
	if (record_size == -1)
	{
		OSDLOG(INFO, "Invalid record size (-1) while recovering from journal");
		return std::make_pair(record, std::make_pair(this->file_offset,0));
	}
	record = this->deserializor->deserialize(this->cur_buffer_ptr, record_size);
	if (record == NULL)
	{
		PASSERT(false, "Invalid record while recovering from journal");
		return std::make_pair(record, std::make_pair(this->file_offset,0));
	}
	JournalOffset offset = this->file_offset;
	this->rest_size -= record_size;
	this->cur_buffer_ptr += record_size;
	this->file_offset.second += record_size;

	return std::make_pair(record, std::make_pair(offset, record_size));
}

void JournalReadHandler::recover_journal_till_eof()
{
        OSDLOG(DEBUG, "Starting recovery till eof");
        this->prepare_read_record();
        //std::pair<recordStructure::Record *, JournalOffset> result;
        std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > result;
        while (true)
        {
                result = this->get_next_journal_record();

                if (result.first == NULL)
                {
                        OSDLOG(INFO, "Record is null");
                        break;
                }
                this->recover_objects(result.first, result.second);
                delete result.first;
        }
        if (!this->check_if_latest_journal())
        {
                this->recover_journal_till_eof();
        }
        OSDLOG(DEBUG, "Recovery complete. Exiting");
}

void JournalReadHandler::recover_till_eof()
{
	OSDLOG(DEBUG, "Starting recovery till eof");
	this->prepare_read_record();
	//std::pair<recordStructure::Record *, JournalOffset> result;
	std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > result;
	while (true)
	{
		result = this->get_next_record();
		
		if (result.first == NULL)
		{
			OSDLOG(INFO, "Record is null");
			break;
		}
		this->recover_objects(result.first, result.second);
		delete result.first;
	}
	if (!this->check_if_latest_journal())
	{
		this->recover_till_eof();	
	}
	OSDLOG(DEBUG, "Recovery complete. Exiting");
}

bool JournalReadHandler::check_if_latest_journal()
{
	return true;
}

uint64_t JournalReadHandler::get_node_id_from_unique_id(uint64_t unique_id)
{
	uint64_t node_id = unique_id >> 56;
	return node_id;
}

JournalWriteHandler::JournalWriteHandler(
	std::string path,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	uint8_t node_id
):
	active_file_id(0),
	serializor(new serializor_manager::Serializor()),
	path(path)
{
	this->checkpoint_interval = parser_ptr->checkpoint_intervalValue();
	this->next_checkpoint_offset = this->checkpoint_interval;
	this->max_file_length = parser_ptr->max_journal_sizeValue(); //tmp done asrivastava
}

uint64_t JournalWriteHandler::get_unique_file_id(uint64_t node_id, uint64_t file_id)
{
	uint64_t unique_id;
	uint64_t tmp = MAX_ID_LIMIT & file_id;
	node_id = node_id << 56;
	unique_id = tmp | node_id;
	return unique_id;
}

JournalWriteHandler::~JournalWriteHandler()
{
	if(this->file_handler != NULL)
	{
		this->file_handler->close();
	}
	delete serializor;
}

JournalOffset JournalWriteHandler::get_last_sync_offset()
{
	return this->last_sync_offset;
}

void JournalWriteHandler::set_active_file_id(uint64_t file_id)
{
	OSDLOG(INFO, "Setting active file id " << file_id);
	this->active_file_id = this->oldest_file_id = file_id;
	OSDLOG(DEBUG,"Journal file : " << make_journal_path(this->path, this->active_file_id));
	this->file_handler.reset(new fileHandler::FileHandler(make_journal_path(this->path, this->active_file_id), fileHandler::OPEN_FOR_APPEND));
	this->last_commit_offset = std::make_pair(this->active_file_id, 0);
	this->last_sync_offset = std::make_pair(this->active_file_id, 0);
}

void JournalWriteHandler::set_upgrade_active_file_id()
{
        OSDLOG(DEBUG,"Journal file : " << this->path + "/" + JOURNAL_FILE_NAME + ".tmp" + boost::lexical_cast<std::string>(this->active_file_id));
        this->file_handler.reset(new fileHandler::FileHandler(this->path + "/" + JOURNAL_FILE_NAME + ".tmp", fileHandler::OPEN_FOR_APPEND));
}

void JournalWriteHandler::rename_file()
{
        OSDLOG(DEBUG,"Journal file : " << this->path + "/" + JOURNAL_FILE_NAME + ".tmp" + boost::lexical_cast<std::string>(this->active_file_id));
        boost::filesystem::rename(make_journal_path(this->path, this->active_file_id), this->path + "/" + JOURNAL_FILE_NAME + ".tmp");        
}


JournalOffset JournalWriteHandler::get_current_journal_offset()
{
	return std::make_pair(this->active_file_id, this->file_handler->tell());
}

bool JournalWriteHandler::write(recordStructure::Record * record)
{

	boost::mutex::scoped_lock lock(this->mtx);

	if (this->active_file_id == 0)
	{
		// called without recovery, only for test
		this->set_active_file_id(1);
	}

	//int64_t current_offset = this->file_handler->tell();
	int64_t current_offset = this->file_handler->seek_end();
	int64_t record_size = this->serializor->get_serialized_size(record);
	OSDLOG(DEBUG, "Writing in Journal File at offset: " << current_offset << " record size: " << record_size);
	if (current_offset == 0)
	{
		this->process_snapshot();
		this->remove_old_files();
	}
	else if(((this->max_file_length - current_offset) < record_size))
	{
		this->add_padding_bytes((this->max_file_length - current_offset));
		this->file_handler->close();
		this->rotate_file(); // open new file

		this->process_snapshot();
		this->remove_old_files();
	}
	else if((((this->next_checkpoint_offset - current_offset) <=  record_size) and current_offset > 0))
	{
		OSDLOG(INFO, "Adding checkpoint at offset: " << this->next_checkpoint_offset);
		this->add_padding_bytes((this->next_checkpoint_offset - current_offset));
		this->process_checkpoint();
		this->next_checkpoint_offset += this->checkpoint_interval;
		OSDLOG(DEBUG, "Next checkpoint will be at offset: " << this->next_checkpoint_offset);
	}
	this->process_write(record, record_size);

	return true;
}

void JournalWriteHandler::rotate_file()
{
	OSDLOG(INFO, "Rotating Journal file " << this->active_file_id);
	this->active_file_id ++;
	this->file_handler->open(make_journal_path(this->path, this->active_file_id), fileHandler::OPEN_FOR_APPEND);
}

void JournalWriteHandler::add_padding_bytes(int64_t padding_bytes)
{
	this->file_handler->insert_padding(padding_bytes);
}

bool JournalWriteHandler::sync()
{
	// TODO handle errors
	this->last_sync_offset = this->get_current_journal_offset();
	return this->file_handler->sync();
}

std::string const make_journal_path(std::string const path, uint64_t const file_id)
{
	return path + "/" + JOURNAL_FILE_NAME + "." + boost::lexical_cast<std::string>(file_id);
}

void JournalWriteHandler::close_journal()
{
	if(this->file_handler) {
		this->file_handler->close();
	}
}

} // namespace journal_manager
