#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include "libraryUtils/journal.h"
#include "containerLibrary/journal_writer.h"
#include "libraryUtils/config_file_parser.h"

#include "libraryUtils/object_storage_log_setup.h"

using namespace object_storage_log_setup;

namespace journal_manager
{

ContainerJournalWriteHandler::ContainerJournalWriteHandler(
	std::string path,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr,
	uint8_t node_id)
:
	JournalWriteHandler(path, parser_ptr, node_id)
{
}

void ContainerJournalWriteHandler::process_snapshot()
{
	OSDLOG(INFO,  "Writing snapshot in Journal");
	recordStructure::Record * record = new recordStructure::SnapshotHeader(this->last_commit_offset);
	int64_t record_size = this->serializor->get_serialized_size(record);
	this->process_write(record, record_size);
	this->next_checkpoint_offset = this->checkpoint_interval;	//TODO : GM hack to be removed by logic 
	delete record;

}

void ContainerJournalWriteHandler::remove_old_files()
{
	OSDLOG(INFO, "Oldest journal ID is " << this->oldest_file_id << ". Removing Container Journal Files except for IDs from " << this->last_commit_offset.first << " to " << this->active_file_id);
	for (uint64_t i = this->oldest_file_id; i < this->last_commit_offset.first; i++)
	{
		boost::filesystem::remove(make_journal_path(this->path, i));
	}
	this->oldest_file_id = this->last_commit_offset.first;
}


void ContainerJournalWriteHandler::process_start_container_flush(std::string path)
{
	OSDLOG(INFO,  "Starting container flush for container " << path << " in journal file " << this->active_file_id);
	recordStructure::Record * record = new recordStructure::ContainerStartFlushMarker(path);
	this->write(record);
	delete record;
}

void ContainerJournalWriteHandler::process_end_container_flush(
	std::string path,
	JournalOffset last_commit_offset
)
{
	//OSDLOG(INFO,  "ContainerJournalWriteHandler::process_end_container_flush: " << path);
	recordStructure::Record * record = new recordStructure::ContainerEndFlushMarker(path, last_commit_offset);
	this->write(record);
	delete record;
	
}

void ContainerJournalWriteHandler::process_accept_component_journal(
	std::list<int32_t> component_number_list
)
{
        OSDLOG(INFO,  "Starting journal write after accept components");
	recordStructure::Record * record = new recordStructure::ContainerAcceptComponentMarker(component_number_list);
	this->write(record);
	delete record;
}

void ContainerJournalWriteHandler::set_last_commit_offset(JournalOffset offset)
{
	this->last_commit_offset = offset;
}

void ContainerJournalWriteHandler::process_checkpoint()
{
	OSDLOG(INFO,  "Writing checkpoint in journal");
	recordStructure::Record * record = new recordStructure::CheckpointHeader(this->last_commit_offset);
	int64_t record_size = this->serializor->get_serialized_size(record);
	this->process_write(record, record_size);
	delete record;
}

void ContainerJournalWriteHandler::process_write(recordStructure::Record * record_ptr, int64_t record_size)
{
	char *buffer = new char[record_size];
	memset(buffer, '\0', record_size);
	this->serializor->serialize(record_ptr, buffer, record_size);
	this->file_handler->write(buffer, record_size);
	delete[] buffer;
}

ContainerJournalWriteHandler::~ContainerJournalWriteHandler()
{
}

/*JournalOffset ContainerJournalWriteHandler::get_last_commit_offset()
{
	return this->last_commit_offset;
}*/

} // namespace ContainerLibrary	
