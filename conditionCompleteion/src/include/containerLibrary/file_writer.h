#ifndef FILE_WRITER
#define FILE_WRITER

#include <map>
#include <list>
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>

#include "memory_writer.h"
#include "libraryUtils/record_structure.h"
#include "journal_writer.h"
#include "containerInfoFile_manager.h"

namespace updater_maintainer
{
	class UpdaterFileManager;
}

namespace container_library
{

class FileWriter
{
	private:
		boost::asio::io_service io_service_obj;
		boost::mutex io_mutex;
		boost::thread_group thread_pool;
		boost::shared_ptr<boost::asio::io_service::work> work;
		boost::shared_ptr<containerInfoFile::ContainerInfoFileManager> info_file_ptr;
		boost::shared_ptr<MemoryManager> memory_ptr;
		boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> container_journal_writer;
		boost::shared_ptr<updater_maintainer::UpdaterFileManager> updater_ptr;
		uint32_t thread_count;
	public:
		FileWriter(
			boost::shared_ptr<containerInfoFile::ContainerInfoFileManager> info_file_ptr,
			boost::shared_ptr<MemoryManager> memory_ptr,
			boost::shared_ptr<journal_manager::ContainerJournalWriteHandler> container_journal_writer,
			boost::shared_ptr<updater_maintainer::UpdaterFileManager> updater_ptr_map,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
		);
		~FileWriter();
		void initialize_worker_threads();
		void stop_worker_threads();
		void write(std::string const & path);
		void write_in_info(std::string path);
};


} // namespace container_library

#endif
