#include <iostream>
#include <tr1/tuple>
#include <vector>

#include "transactionLibrary/transaction_read_handler.h"
#include "transactionLibrary/transaction_memory.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace transaction_read_handler
{

TransactionRecovery::TransactionRecovery(
	std::string & path,
	boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
):
	journal_reader_ptr(new transaction_read_handler::TransactionReadHandler(path, memory_mgr, parser_ptr))
{
}

TransactionRecovery::~TransactionRecovery()
{
}

bool TransactionRecovery::perform_recovery(std::list<int32_t>& component_list)
{
	this->journal_reader_ptr->init_journal();
	this->journal_reader_ptr->recover_snapshot();
	this->journal_reader_ptr->recover_last_checkpoint();
	this->journal_reader_ptr->set_recovering_offset();
	this->journal_reader_ptr->recover_journal_till_eof();
	this->journal_reader_ptr->mark_unknown_components(component_list);
	return true;
}

uint64_t TransactionRecovery::get_next_file_id()
{
	return this->journal_reader_ptr->get_next_file_id();
}

void TransactionRecovery::clean_journal_files()
{
	this->journal_reader_ptr->clean_journal_files();
}

TransactionReadHandler::TransactionReadHandler(
	std::string & path,
	boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
):
	JournalReadHandler(path, parser_ptr),
	memory_mgr_ptr(memory_mgr_ptr)
{
	OSDLOG(DEBUG, "TransactionReadHandler constructed");
}

bool TransactionReadHandler::process_recovering_offset()
{
	return true;
}

TransactionReadHandler::~TransactionReadHandler()
{
}

bool TransactionReadHandler::process_last_checkpoint(int64_t offset)
{
	OSDLOG(INFO, osd_transaction_lib_log_tag << "Recovering checkpoint at offset :" << offset);
	if (offset <= 0)
	{
		OSDLOG(INFO, osd_transaction_lib_log_tag << "Checkpoint not found in Journal file");
		return false;
	}
	this->file_handler->seek(offset);
	this->prepare_read_record();
        if ( this->journal_version == 0 )
        {
                this->checkpoint_interval = 5242880;
        }

	this->prepare_read_record();
	std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > result = this->get_next_journal_record();
	if ( result.first == NULL )
        {
                OSDLOG(INFO, osd_transaction_lib_log_tag << "Last Checkpoint found in Journal File is corrupted.");
                OSDLOG(INFO, osd_transaction_lib_log_tag << "Searching for second last checkpoint.");
                return this->recover_last_checkpoint();
        }
	recordStructure::CheckpointRecord * checkpoint_ptr = dynamic_cast<recordStructure::CheckpointRecord *>(result.first);
	this->memory_mgr_ptr->set_last_transaction_id(checkpoint_ptr->get_last_transaction_id());
	std::vector<int64_t> id_list = checkpoint_ptr->get_id_list();
	this->active_id_list = std::set<int64_t>(id_list.begin(), id_list.end());
	std::set<int64_t>::iterator it = this->active_id_list.begin();
	for ( ; it != this->active_id_list.end(); ++it)
	{
		OSDLOG(DEBUG, "ID found in Checkpoint Header is " << *it);
	}
	delete result.first;
	return true;
}

bool TransactionReadHandler::process_snapshot()
{
	OSDLOG(DEBUG, osd_transaction_lib_log_tag << "Recovering snapshot");
	this->file_handler->seek(0);
	this->last_commit_offset = std::make_pair(this->latest_file_id, 0);
	this->prepare_read_record();
	std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > result = this->get_next_journal_record();
	OSDLOG(INFO,"Record Size of Snap Shot" << result.second.second);
        if ( result.second.second <= 28 )
        {
                OSDLOG(DEBUG,"Setting Journal Version to 0");
                this->journal_version = 0;
                this->checkpoint_interval = 5242880;

        }
        else
        {
                OSDLOG(DEBUG,"Setting Journal Version to " << result.first->get_journal_version());
                this->journal_version = result.first->get_journal_version();
        }
        this->file_handler->seek(0);
        return true;
}

void TransactionReadHandler::mark_unknown_components(std::list<int32_t>& component_list)
{

        this->memory_mgr_ptr->mark_unknown_memory(component_list);

}
bool TransactionReadHandler::recover_objects(recordStructure::Record * record, std::pair<JournalOffset, int64_t> offset1)
{
	JournalOffset offset = offset1.first;
	int type = record->get_type();
	OSDLOG(DEBUG, osd_transaction_lib_log_tag << "Recovering object of type: " << type);
	switch (type)
	{
		case recordStructure::SNAPSHOT_HEADER_RECORD:
			{
				recordStructure::SnapshotHeaderRecord * snapshot_header = dynamic_cast<recordStructure::SnapshotHeaderRecord *>(record);
				this->memory_mgr_ptr->set_last_transaction_id(snapshot_header->get_last_transaction_id());
				break;
			}
		case recordStructure::OPEN_RECORD:
			{
				recordStructure::ActiveRecord *active_ptr = dynamic_cast<recordStructure::ActiveRecord *>(record);
				OSDLOG(INFO, "Recovering ActiveRecord " << active_ptr->get_object_name() << 
						" with ID " << active_ptr->get_transaction_id());
				int operation = active_ptr->get_operation_type();
				std::string object_name = active_ptr->get_object_name();
				std::string request_method = active_ptr->get_request_method();
				std::string object_id = active_ptr->get_object_id();
				int64_t id = active_ptr->get_transaction_id();

				boost::shared_ptr<recordStructure::ActiveRecord> record_ptr(new recordStructure::ActiveRecord(operation, object_name, request_method, object_id, id));
				OSDLOG(DEBUG,"Record Size is : " << offset1.second);
				if ( ( offset1.second - 16 ) >= 174 )
				{
					record_ptr->set_time(active_ptr->get_time());
				}
				else
					record_ptr->set_time(0);
				if (offset < this->last_checkpoint_offset)
				{
					OSDLOG(DEBUG, "Recovering active records before checkpoint");
					std::set<int64_t>::iterator it = this->active_id_list.find(active_ptr->get_transaction_id());
					if (it != this->active_id_list.end())
					{
						OSDLOG(DEBUG, "This record found in Checkpoint Active List");
						this->memory_mgr_ptr->acquire_lock(record_ptr);
						this->active_id_list.erase(it);
					}
				}
				else
				{
					OSDLOG(DEBUG, "Current offset is greater than Checkpoint offset");
					this->memory_mgr_ptr->acquire_lock(record_ptr);
				}	
				break;
			}
		case recordStructure::CLOSE_RECORD:
			{
				recordStructure::CloseRecord * close_ptr = dynamic_cast<recordStructure::CloseRecord *>(record);
				OSDLOG(INFO, "Close Record for ID " << close_ptr->get_transaction_id());
				this->memory_mgr_ptr->release_lock_by_id(close_ptr->get_transaction_id());
				break;
			}
		default:
			break;
	}
	//Below Log is causing Major Performance Degrade
	//OSDLOG(INFO, "Type: " << type << ". Map contains: " << this->memory_mgr_ptr->get_memory().size() << " elements");
	return true;
}

} // namespace transaction_read_handler
