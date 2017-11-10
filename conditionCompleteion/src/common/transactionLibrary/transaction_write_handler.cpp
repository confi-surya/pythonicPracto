#include <iostream>
#include <boost/filesystem.hpp>

#include "libraryUtils/journal.h"
#include "transactionLibrary/transaction_write_handler.h"
#include "transactionLibrary/transaction_memory.h"
#include "libraryUtils/helper.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace transaction_write_handler
{

TransactionWriteHandler::TransactionWriteHandler(
	std::string & path,
	boost::shared_ptr<transaction_memory::TransactionStoreManager> memory_mgr_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	uint8_t node_id
):
	JournalWriteHandler(path, parser_ptr, node_id),
	memory_mgr_ptr(memory_mgr_ptr),
	last_transaction_id(0)
{
}

TransactionWriteHandler::~TransactionWriteHandler()
{
}

void TransactionWriteHandler::process_write(recordStructure::Record * record, int64_t record_size)
{	
	char *buffer = new char[record_size];
	memset(buffer, '\0', record_size);
	if (record->get_type() == recordStructure::OPEN_RECORD)
	{
		recordStructure::ActiveRecord *active_ptr = dynamic_cast<recordStructure::ActiveRecord *>(record);
		DASSERT(this->last_transaction_id < active_ptr->get_transaction_id(), "transaction id must be incremental");
		this->last_transaction_id = active_ptr->get_transaction_id();
	}
	this->serializor->serialize(record, buffer, record_size);
	this->file_handler->write(buffer, record_size);
	delete[] buffer;
}

void TransactionWriteHandler::process_sync()
{
}

void TransactionWriteHandler::process_checkpoint()
{
	OSDLOG(INFO, "Writing checkpoint in Transaction Journal");
	std::vector<int64_t> id_list = this->memory_mgr_ptr->get_active_id_list();
	recordStructure::Record * record = new recordStructure::CheckpointRecord(this->last_transaction_id, id_list);
	int64_t size = this->serializor->get_serialized_size(record);
	this->process_write(record, size);
	delete record;
}

void TransactionWriteHandler::process_snapshot()
{
	OSDLOG(INFO, "Writing snapshot in Transaction Journal");

	int64_t record_size = 0, total_size = 0, snapshot_header_size = 0;
	recordStructure::SnapshotHeaderRecord snapshot_header(this->last_transaction_id);
	snapshot_header_size += this->serializor->get_serialized_size(&snapshot_header);
	total_size += snapshot_header_size;
	std::map<int32_t, std::pair<std::map<int64_t,
		boost::shared_ptr<recordStructure::ActiveRecord> >, bool> > mem_map 
                                                                        = this->memory_mgr_ptr->get_memory();
        typedef std::map<int32_t, std::pair<
                std::map<int64_t,boost::shared_ptr<recordStructure::ActiveRecord> >, bool> >::iterator mem_it;
	typedef std::map<int64_t, boost::shared_ptr<recordStructure::ActiveRecord> >::iterator it;
	for ( mem_it it1 = mem_map.begin(); it1 != mem_map.end(); it1++ )
        {
                for(it it2 = it1->second.first.begin(); it2 != it1->second.first.end(); it2++ )
                {
                        recordStructure::Record * record = (it2->second).get();
                        total_size += this->serializor->get_serialized_size(record);
                }
        }

	char *buffer = new char[total_size];
	memset(buffer, '\0', total_size);
	char *tmp_buf = buffer;
	this->serializor->serialize(&snapshot_header, tmp_buf, snapshot_header_size);
	tmp_buf += snapshot_header_size;
	for ( mem_it it1 = mem_map.begin(); it1 != mem_map.end(); it1++ )
        {
                for (it it2 = it1->second.first.begin(); it2 != it1->second.first.end(); it2++ )
                {
                        recordStructure::Record * record = (it2->second).get();
                        record_size = this->serializor->get_serialized_size(record);
                        this->serializor->serialize(record, tmp_buf, record_size);
                        OSDLOG(DEBUG, "Writing record in snapshot " << it2->second->get_object_name());
                        tmp_buf += record_size;
                }
        }
	this->file_handler->write(buffer, total_size);
	this->next_checkpoint_offset = this->checkpoint_interval;
	delete[] buffer;
}

void TransactionWriteHandler::remove_old_files()
{
	for (uint64_t i = this->oldest_file_id; i < this->active_file_id; i++)
	{
		OSDLOG(INFO, osd_transaction_lib_log_tag << "Removing old journal file " << journal_manager::make_journal_path(this->path, i));
		boost::filesystem::remove(journal_manager::make_journal_path(this->path, i));
	}
	this->oldest_file_id = this->active_file_id;
}

} // namespace transaction_write_handler
