#ifndef __UPDATER_FILE_MANAGER_H__
#define __UPDATER_FILE_MANAGER_H__

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "containerLibrary/memory_writer.h"
#include "libraryUtils/file_handler.h"
#include "libraryUtils/config_file_parser.h"

namespace updater_maintainer
{

#define UPDATER_THREAD_SLEEP_TIME 120

class UpdaterPointerManager
{
	public:
		UpdaterPointerManager(
			int32_t component_number, 
			std::string cont_name,
			std::string path);
		~UpdaterPointerManager();
		const time_t get_creation_time();
		std::list<std::string> get_cont_hash_list();
		boost::shared_ptr<fileHandler::FileHandler> get_file_handler();
		void set_creation_time(const clock_t creation_time);
		void add_cont_hash(std::string cont_hash);
		void set_file_descriptor(boost::shared_ptr<fileHandler::FileHandler> file_handler);
		void rotate_updater_file(int32_t component_number, 
					 std::string path, 
					 boost::shared_ptr<container_library::MemoryManager> memory_ptr);
		void write_in_file(int32_t component_number, 
				   std::string cont_name, 
				   uint32_t max_cont_hash_list_size, 
				   std::string path,
				   boost::shared_ptr<container_library::MemoryManager> memory_ptr);
	private:
		std::list<std::string> cont_hash_list;
		clock_t creation_time;
		boost::shared_ptr<fileHandler::FileHandler> file_handler;
		boost::mutex mutex;
		void dump_mem_in_file(int32_t component_number,
					boost::shared_ptr<container_library::MemoryManager> memory_ptr);
		void create_new_file(int32_t component_number, std::string path);
                const std::string get_current_time();
		std::string get_container_name(std::string cont_path);
};


class UpdaterFileManager
{
	public:
		void write_in_file(int32_t component_number, std::string cont_path);
		UpdaterFileManager(
			boost::shared_ptr<container_library::MemoryManager> memory_ptr,
			cool::SharedPtr<config_file::ConfigFileParser> parser_ptr
		);
		~UpdaterFileManager();
		void remove_component_records(std::list<int32_t> component_numbers);
		void check_for_updater_file(int32_t component_number, std::string cont_path);
	private:
		boost::shared_ptr<container_library::MemoryManager> memory_ptr;
		std::set<std::string> updated_cont_list;
		const std::string get_current_time();
		void scheduled_timer();
		void start_timer();
		void run_timer_thread();
		void reset_timer(const boost::system::error_code& error);
		void updater_map_lookup();
		boost::shared_ptr<boost::asio::deadline_timer> timer;
		std::string path;
		int32_t timeout;
		boost::asio::io_service io_service_obj;
		std::map<int32_t,boost::shared_ptr<UpdaterPointerManager> > updater_map;
		boost::thread* monitoring_thread;
		boost::mutex mtx1;
		boost::shared_mutex mtx2;
		bool flag;
		uint32_t max_updated_cont_list_size;
		uint64_t node_id;
};

		

} // namespace updater_maintainer
#endif
	 
