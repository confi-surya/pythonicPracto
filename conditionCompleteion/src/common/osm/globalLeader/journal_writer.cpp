#include "osm/globalLeader/journal_writer.h"

using namespace object_storage_log_setup;

namespace journal_manager
{

GLJournalWriteHandler::GLJournalWriteHandler(
	std::string path,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	boost::shared_ptr<component_manager::ComponentMapManager> map_ptr,
	boost::function<std::vector<std::string>(void)> get_state_change_ids
):
	JournalWriteHandler(path, parser_ptr, 1),	//TODO the prototype of base class will change
	map_ptr(map_ptr),
	get_state_change_ids(get_state_change_ids)
{
	this->latest_state_change_id = 0;
}

void GLJournalWriteHandler::process_snapshot()
{
	OSDLOG(INFO, "Processing GL Snapshot: No snapshot in GL Journal");
/*	int64_t total_size = 0, record_size = 0, snapshot_header_size = 0;

//	recordStructure::GLSnapshotHeader snapshot_record(this->latest_state_change_id);
//	snapshot_header_size = this->serializor->get_serialized_size(&snapshot_record);
//	total_size += snapshot_header_size;

	//std::pair<std::vector<boost::shared_ptr<recordStructure::ServiceComponentCouple> >, float> trans_mem_records = 
	//						this->map_ptr->convert_full_transient_map_into_journal_record();
	std::vector<boost::shared_ptr<recordStructure::Record> > trans_mem_records = 
							this->map_ptr->convert_full_transient_map_into_journal_record();
	std::cout << "TMAp size : " << trans_mem_records.size() << std::endl;
//	recordStructure::Record * record = new recordStructure::TransientMapRecord(trans_mem_records.first, trans_mem_records.second);
	std::vector<boost::shared_ptr<recordStructure::Record> >::iterator it = trans_mem_records.begin();
	for ( ; it != trans_mem_records.end(); ++it )
	{
		total_size += this->serializor->get_serialized_size((*it).get());
	}
	char * buf = new char[total_size];
	char * tmp = buf;
	memset(buf, '\0', total_size);
//	this->serializor->serialize(&snapshot_record, tmp, snapshot_header_size);
//	tmp += snapshot_header_size;

	for ( it = trans_mem_records.begin(); it != trans_mem_records.end(); ++it )
	{
		record_size = this->serializor->get_serialized_size((*it).get());
		this->serializor->serialize((*it).get(), tmp, record_size);
		tmp += record_size;
	}
	
	this->file_handler->write(buf, total_size);
	this->next_checkpoint_offset = this->checkpoint_interval;
	delete[] buf;
*/
}

//UNCOVERED_CODE_BEGIN:
void GLJournalWriteHandler::process_checkpoint()
{
	OSDLOG(INFO, "Processing Checkpoint: No Checkpoint in GL Journal");
/*
	//get all state change IDs from State Change Table
	std::vector<std::string> state_change_ids = get_state_change_ids();
	recordStructure::Record * record = new recordStructure::GLCheckpointHeader(state_change_ids);
	int64_t size = this->serializor->get_serialized_size(record);
	this->process_write(record, size);
	delete record;
*/
}
//UNCOVERED_CODE_END

void GLJournalWriteHandler::process_write(recordStructure::Record * record, int64_t record_size)
{
	char *buffer = new char[record_size];
	memset(buffer, '\0', record_size);
	this->serializor->serialize(record, buffer, record_size);
	this->file_handler->write(buffer, record_size);
	delete[] buffer;

}

void GLJournalWriteHandler::remove_all_journal_files()
{
	OSDLOG(DEBUG, "Oldest journal file id: " << this->oldest_file_id << " and active file id: " << this->active_file_id);
	for (uint64_t i = this->oldest_file_id; i <= this->active_file_id; i++)
	{       
		OSDLOG(INFO, "Removing old journal file " << journal_manager::make_journal_path(this->path, i));    
		boost::filesystem::remove(journal_manager::make_journal_path(this->path, i));
        }
	this->oldest_file_id = this->active_file_id;
}

void GLJournalWriteHandler::remove_old_files()
{
	OSDLOG(DEBUG, "Oldest journal file id: " << this->oldest_file_id << " and active file id: " << this->active_file_id);
	for (uint64_t i = this->oldest_file_id; i < this->active_file_id; i++)
	{       
		OSDLOG(INFO, "Removing old journal file " << journal_manager::make_journal_path(this->path, i));    
		boost::filesystem::remove(journal_manager::make_journal_path(this->path, i));
        }
	this->oldest_file_id = this->active_file_id;
}

} // namespace journal_manager
