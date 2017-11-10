#include "containerLibrary/journal_reader.h"

namespace container_read_handler
{

ContainerRecovery::ContainerRecovery(
	std::string path,
	boost::shared_ptr<container_library::MemoryManager> memory_obj,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
)
{
	this->recovery_obj.reset(new ContainerJournalReadHandler(path, memory_obj, parser_ptr));
}

bool ContainerRecovery::perform_recovery(std::list<int32_t>& component_list)
{
	this->recovery_obj->init_journal();
	this->recovery_obj->recover_snapshot();
	this->recovery_obj->recover_last_checkpoint();
	this->recovery_obj->set_recovering_offset();
	
	this->recovery_obj->recover_journal_till_eof();
	this->recovery_obj->mark_unknown_components(component_list);
	return true;
}


void ContainerRecovery::clean_journal_files()
{
	this->recovery_obj->clean_journal_files();
}

uint64_t ContainerRecovery::get_next_file_id() const
{
	return this->recovery_obj->get_next_file_id();
}

uint64_t ContainerRecovery::get_current_file_id() const
{
	return this->recovery_obj->get_current_file_id();
}

ContainerJournalReadHandler::ContainerJournalReadHandler(
std::string path,
	boost::shared_ptr<container_library::MemoryManager> memory_obj,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
):
	journal_manager::JournalReadHandler(path, parser_ptr),
	memory_obj(memory_obj)
{
	//this->deserializor.reset(new serializor_manager::Deserializor());
	OSDLOG(DEBUG, "ContainerJournalReadHandler constructed");
}

ContainerJournalReadHandler::~ContainerJournalReadHandler()
{
}

bool ContainerJournalReadHandler::check_if_latest_journal()
{	OSDLOG(INFO,  "Recovery complete from " << journal_manager::make_journal_path(this->path, this->last_commit_offset.first));
	if (this->last_commit_offset.first != this->latest_file_id)
	{
		this->last_commit_offset.first ++;
		OSDLOG(INFO,  "Now recovering from " << journal_manager::make_journal_path(this->path, this->last_commit_offset.first));
		this->file_handler->close();
		this->file_handler->open(journal_manager::make_journal_path(this->path, this->last_commit_offset.first), fileHandler::OPEN_FOR_READ);
		this->file_handler->seek(0);
		return false;
	}
	return true;
}

bool ContainerJournalReadHandler::process_last_checkpoint(int64_t last_checkpoint_offset)
{
	this->file_handler->seek(last_checkpoint_offset);
	if (last_checkpoint_offset <= 0)
	{
		OSDLOG(DEBUG,  "Checkpoint not found in Journal file");
		return false;
	}
	OSDLOG(DEBUG,  "Last checkpoint found at offset: " << last_checkpoint_offset);
	this->prepare_read_record();

	std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > result = this->get_next_journal_record();
	if (result.first == NULL)
	{
		OSDLOG(INFO, osd_transaction_lib_log_tag << "Last Checkpoint found in Journal File is corrupted.");
                OSDLOG(DEBUG, osd_transaction_lib_log_tag << "Searching for second last checkpoint.");
                return this->recover_last_checkpoint();
	}
	recordStructure::CheckpointHeader * checkpoint_obj = dynamic_cast<recordStructure::CheckpointHeader *>(result.first);

	this->last_commit_offset = checkpoint_obj->get_last_commit_offset();

	OSDLOG(INFO, "Last commit offset in checkpoint header is " << this->last_commit_offset);
	DASSERT(checkpoint_obj, "MUST be checkpoint record at offset " << last_checkpoint_offset);
	delete result.first;
	return true;

}

bool ContainerJournalReadHandler::process_snapshot()
{
	this->prepare_read_record();
	std::pair<recordStructure::Record *, std::pair<JournalOffset, int64_t> > result = this->get_next_journal_record();
	if (result.first == NULL)
	{
		PASSERT(false, "Corrupted Snapshot Header. Aborting.");
	}
	recordStructure::SnapshotHeader * snapshot_obj = dynamic_cast<recordStructure::SnapshotHeader *>(result.first);

	this->last_commit_offset = snapshot_obj->get_last_commit_offset();

	DASSERT(snapshot_obj, "MUST be snapshot record");
	OSDLOG(DEBUG, "Last commit offset in snapshot header is " << this->last_commit_offset);
	delete result.first;
	return true;
}

void ContainerJournalReadHandler::mark_unknown_components(std::list<int32_t>& component_list)
{

	this->memory_obj->mark_unknown_memory(component_list);

}

bool ContainerJournalReadHandler::recover_objects(recordStructure::Record *record_ptr, std::pair<JournalOffset, int64_t> offset1)
{
	JournalOffset offset = offset1.first;
	int type = record_ptr->get_type();
	OSDLOG(INFO, "Type identified during objectRecovery: " << type << " at offset: " << offset.second);
	switch(type)
	{
		case recordStructure::CONTAINER_START_FLUSH:
				{
				break;
				}
		case recordStructure::CONTAINER_END_FLUSH:
				{
				recordStructure::ContainerEndFlushMarker * end_ptr = dynamic_cast<recordStructure::ContainerEndFlushMarker * >(record_ptr);
				this->memory_obj->remove_entry(end_ptr->get_path(), end_ptr->get_last_commit_offset());
				OSDLOG(DEBUG, "Removing entry " << end_ptr->get_path());
				break;
				}
		case recordStructure::CHECKPOINT_CONTAINER_JOURNAL:
				break;
		case recordStructure::FINAL_OBJECT_RECORD:
				{
				recordStructure::FinalObjectRecord * final_ptr = dynamic_cast<recordStructure::FinalObjectRecord * >(record_ptr);
				//bool flag = this->memory_obj->is_object_exist(final_ptr->get_path(), final_ptr->get_object_record());
				//if ( !flag )
				//{
				
					JournalOffset sync_offset = std::make_pair(offset.first, offset.second+offset1.second);
					this->memory_obj->add_entry(final_ptr->get_path(), final_ptr->get_object_record(), offset, sync_offset, true);
					OSDLOG(DEBUG, "Object " << final_ptr->get_path() << "/" << final_ptr->get_object_record().get_name() << " loaded in memory while recovery");
				//}
				//else
				//{
				//	OSDLOG(DEBUG, "Object " << final_ptr->get_path() << "/" << final_ptr->get_object_record().get_name() << "did not load in memory while recovery because it is already present");
				//}
				break;
				}
		case recordStructure::SNAPSHOT_CONTAINER_JOURNAL:
				break;
		case recordStructure::ACCEPT_COMPONENTS_MARKER:
				{
				OSDLOG(DEBUG, "ACCEPT_COMPONENTS_MARKER present in journal, Removing from memory")
				recordStructure::ContainerAcceptComponentMarker * accept_ptr =  
						dynamic_cast<recordStructure::ContainerAcceptComponentMarker * >(record_ptr);
				this->memory_obj->remove_components(accept_ptr->get_comp_list());
				break;
				}
		default:
				break;
	}
	//OSDLOG(DEBUG, "Records in memory: " << this->memory_obj->get_memory_map().size());
	OSDLOG(INFO, "Containers in memory: " << this->memory_obj->get_container_count() << ". Max Records in memory may be: " << this->memory_obj->get_record_count());
	return true;
	
}

bool ContainerJournalReadHandler::process_recovering_offset()
{	
    OSDLOG(DEBUG,  "Finding recovery offset");
	if (this->last_commit_offset.first != this->latest_file_id)
	{
		OSDLOG(INFO,  "File id found in process_recovery_offset is not same as Current Journal File");
		this->file_handler->close();
		this->file_handler->open(journal_manager::make_journal_path(this->path, this->last_commit_offset.first), fileHandler::OPEN_FOR_READ);
	}
	this->file_handler->seek(this->last_commit_offset.second);
	OSDLOG(INFO,  "For recovery, Journal file id is " << this->last_commit_offset.first << " and recovery will be done for records found after " << this->last_commit_offset.second << " offset.");
	return true;
}

} // namespace container_read_handler
