#ifndef __RINGFILE_H_2424__
#define __RINGFILE_H_2424__

#include <boost/thread/mutex.hpp>

#include "libraryUtils/file_handler.h"
#include "libraryUtils/serializor.h"
#include "libraryUtils/record_structure.h"
#include "libraryUtils/config_file_parser.h"

#define MAX_READ_CHUNK_SIZE 65536

namespace ring_file_manager
{

const static std::string RING_FILE_NAME = "ring_file.ring";

class RingFileReadHandler
{
	public:
		RingFileReadHandler(std::string path);
		virtual ~RingFileReadHandler();
		std::vector<std::string> get_fs_list();
		std::vector<std::string> get_account_level_dir_list();
		std::vector<std::string> get_container_level_dir_list();
		std::vector<std::string> get_object_level_dir_list(); 
		
	protected:
		std::string path;
		std::vector<char> buffer;
                char * cur_buffer_ptr;
		boost::shared_ptr<serializor_manager::Deserializor> deserializor;
                boost::shared_ptr<fileHandler::FileHandler> file_handler;
		boost::shared_ptr<recordStructure::RingRecord> read_ring_file();
};

class RingFileWriteHandler
{
	public:
		RingFileWriteHandler(std::string path);
		virtual ~RingFileWriteHandler();
		bool remakering();
	protected:
		boost::shared_ptr<fileHandler::FileHandler> file_handler;
		serializor_manager::Serializor * serializor;

	private:
		std::string path;
		boost::mutex mtx;
		std::vector<char> buffer;
		int64_t max_file_length;
		void process_write(recordStructure::Record * record, int64_t record_size);
};

std::string const make_ring_file_path(std::string const path);
std::string const make_temp_ring_file_path(std::string const path);

}// ring_file_manager namespace ends
#endif
