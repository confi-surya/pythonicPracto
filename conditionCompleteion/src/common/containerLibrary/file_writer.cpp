//#include <vector>
//#include <boost/bind.hpp>
//#include <boost/shared_ptr.hpp>

#include "containerLibrary/file_writer.h"
#include "libraryUtils/config_file_parser.h"
#include "containerLibrary/containerInfoFile_manager.h"
#include "libraryUtils/object_storage_log_setup.h"
#include "containerLibrary/updater_info.h"

namespace container_library
{

FileWriter::FileWriter(
	boost::shared_ptr<containerInfoFile::ContainerInfoFileManager> info_file_ptr,
	boost::shared_ptr<MemoryManager> memory_ptr,
	boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> container_journal_writer,
	boost::shared_ptr<updater_maintainer::UpdaterFileManager>  updater_ptr,
	cool::SharedPtr<config_file::ConfigFileParser> parser_ptr)
:
	info_file_ptr(info_file_ptr),
	memory_ptr(memory_ptr),
	container_journal_writer(container_journal_writer),
	updater_ptr(updater_ptr)
{
	this->thread_count = parser_ptr->file_writer_threads_countValue();
	this->initialize_worker_threads();
	
}

void FileWriter::initialize_worker_threads()
{
	this->work.reset(new boost::asio::io_service::work(this->io_service_obj));
	for (uint32_t i = 1; i <= this->thread_count; i++)
	{
		this->thread_pool.create_thread(boost::bind(&boost::asio::io_service::run, &this->io_service_obj));
	}
}

FileWriter::~FileWriter()
{
	this->stop_worker_threads();
//	this->work.reset();
//	this->thread_pool.join_all();
}

void FileWriter::stop_worker_threads()
{
	this->work.reset();
	this->thread_pool.join_all();
	this->io_service_obj.reset();
}

void FileWriter::write_in_info(std::string path)
{
	OSDLOG(INFO, "Flushing object entries for " << path << " in Info File");
	// get the unknown status of container	
	bool unknown_status = false;
	RecordList * records = this->memory_ptr->get_flush_records(path, unknown_status, false);
	if (records == NULL || records->size() == 0)
	{
		// another flush_container or compaction of the container is running
		return;
	}
	container_journal_writer->process_start_container_flush(path);
	JournalOffset sync_off = (--records->end())->first;
	int32_t component_number = this->memory_ptr->get_component_name(path);
	//std::map<int32_t,boost::shared_ptr<updater_maintainer::UpdaterFileManager> >
	//			::iterator it = this->updater_ptr_map.find(component_number);

	OSDLOG(INFO, "Flushing object entries for component " << component_number << " in Info File");
	/*if ( it != this->updater_ptr_map.end())
	{
		it->second->write_in_file(path);
	}*/
	
	this->updater_ptr->write_in_file(component_number, path);
	this->info_file_ptr->flush_container(path, 
				records,
				unknown_status,
				boost::bind(&MemoryManager::release_flush_records, this->memory_ptr, path),
				boost::bind(&MemoryManager::delete_container, this->memory_ptr, path)
			);
//	this->memory_ptr->release_flush_records(path);
	container_journal_writer->process_end_container_flush(path, sync_off);

	JournalOffset last_commit_offset = this->memory_ptr->get_last_commit_offset();
	if (last_commit_offset.first == std::numeric_limits<uint64_t>::max())
	{
	       last_commit_offset = sync_off;
	}
	OSDLOG(INFO, "Flushing complete. Setting last commit offset in journal: " << last_commit_offset);

	container_journal_writer->set_last_commit_offset(last_commit_offset);
}

void FileWriter::write(std::string const & path)
{
	this->io_service_obj.post(boost::bind(&FileWriter::write_in_info, this, path));
}

} // namespace container_library
