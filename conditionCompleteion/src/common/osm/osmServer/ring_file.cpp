#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <cool/debug.h>
#include "osm/osmServer/ring_file.h"
#include "cool/cool.h"

#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"

using namespace object_storage_log_setup;
using namespace OSDException;

namespace ring_file_manager
{

RingFileReadHandler::RingFileReadHandler(
                std::string path
                )
        : path(path)
{
        this->deserializor.reset(new serializor_manager::Deserializor());
	this->buffer.resize(MAX_READ_CHUNK_SIZE);
        this->cur_buffer_ptr = &this->buffer[0];
}

RingFileReadHandler::~RingFileReadHandler()
{
	if (this->file_handler != NULL)
	{
		this->file_handler->close();
	}
}

boost::shared_ptr<recordStructure::RingRecord> RingFileReadHandler::read_ring_file() 
/*
*@desc: ring file read interface
*/
{
	try
	{
		this->file_handler.reset(new fileHandler::FileHandler(make_ring_file_path(this->path), fileHandler::OPEN_FOR_READ));
	}
	catch(...)
	{
		OSDLOG(ERROR, "Unable to open ring file" << this->path);
		PASSERT(false, "Unable to open ring file for read " << this->path);
	}
	this->buffer.resize(MAX_READ_CHUNK_SIZE);
        this->file_handler->seek(0); //while reading ring file, point to the start of the file, always
        this->cur_buffer_ptr = &this->buffer[0];
	this->file_handler->read(&this->buffer[0], MAX_READ_CHUNK_SIZE);

	recordStructure::Record * rec = NULL;
        int64_t record_size = this->deserializor->get_osm_info_file_record_size(this->cur_buffer_ptr);
        if (record_size <= 0)
        {
		boost::shared_ptr<recordStructure::Record> record(rec);
        	boost::shared_ptr<recordStructure::RingRecord> ring_file_rcrd = 
				boost::dynamic_pointer_cast<recordStructure::RingRecord>(record);
                return ring_file_rcrd; 
        }
        rec = this->deserializor->osm_info_file_deserialize(this->cur_buffer_ptr, record_size);
        if (!rec)
        {
                OSDLOG(ERROR, "Invalid Ring File Record (NULL)");
		boost::shared_ptr<recordStructure::Record> record(rec);
        	boost::shared_ptr<recordStructure::RingRecord> ring_file_rcrd = 
				boost::dynamic_pointer_cast<recordStructure::RingRecord>(record);
                return ring_file_rcrd; //record return will be NULL. Exit condition for while loop
        }
        this->cur_buffer_ptr += record_size;
	boost::shared_ptr<recordStructure::Record> record(rec);
        boost::shared_ptr<recordStructure::RingRecord> ring_file_rcrd = 
				boost::dynamic_pointer_cast<recordStructure::RingRecord>(record);
	return ring_file_rcrd;
	
}

std::vector<std::string> RingFileReadHandler::get_fs_list()
/*
*@desc: interface to obtain list of filesystems from ring file
*
*@return: list of file systems
*/
{
	boost::shared_ptr<recordStructure::RingRecord> ring_file_rcrd = this->read_ring_file();;
	return ring_file_rcrd->get_fs_list();
}

std::vector<std::string> RingFileReadHandler::get_account_level_dir_list()
/*
*@desc: interface to obtain list of account level dir list from ring file
*
*@return: list of file systems
*/
{
        boost::shared_ptr<recordStructure::RingRecord> ring_file_rcrd = this->read_ring_file();;
        return ring_file_rcrd->get_account_level_dir_list();
}

std::vector<std::string> RingFileReadHandler::get_container_level_dir_list()
/*
*@desc: interface to obtain list of container level dir list from ring file
*
*@return: list of file systems
*/
{
        boost::shared_ptr<recordStructure::RingRecord> ring_file_rcrd = this->read_ring_file();
        return ring_file_rcrd->get_container_level_dir_list();
}

std::vector<std::string> RingFileReadHandler::get_object_level_dir_list()
/*
*@desc: interface to obtain list of object level dir list from ring file
*
*@return: list of file systems
*/
{
        boost::shared_ptr<recordStructure::RingRecord> ring_file_rcrd = this->read_ring_file();
        return ring_file_rcrd->get_object_level_dir_list();
}


//---------------------Info File Writer--------------------------//

RingFileWriteHandler::RingFileWriteHandler(std::string path)
	: serializor(new serializor_manager::Serializor()), path(path)
{
	this->max_file_length = 10487560; //tmp done smalhotra
}

RingFileWriteHandler::~RingFileWriteHandler()
{
	if(this->file_handler != NULL)
        {
                this->file_handler->close();
        }
        delete serializor;
}

bool RingFileWriteHandler::remakering()
/*
*@desc: interface for preparing ring file. Writes the filesystem list, account directories, container directories, object directories in ring file
*
*@return: true if ring file is prepared, false otherwise
*/
{
	if (boost::filesystem::exists(make_ring_file_path(this->path)))
	{
		OSDLOG(INFO, "Ring File already exists.");
		return true;
	}

	try
	{
		this->file_handler.reset(new fileHandler::FileHandler(make_temp_ring_file_path(this->path), fileHandler::OPEN_FOR_APPEND));
	}
	catch(...)
	{
		OSDLOG(ERROR, "Unable to open temp ring file for creation");
		return false;
	}
	recordStructure::RingRecord * record = new recordStructure::RingRecord;
	record->set_data(); //Initialize List contents
	int64_t current_offset = this->file_handler->seek(0); 	
	int64_t record_size = this->serializor->get_osm_info_file_serialized_size(record);
	if (current_offset == 0)
	{
		OSDLOG(INFO, "File Empty! Adding record at offset: " << current_offset);
	}
	else if(((this->max_file_length - current_offset) < record_size)) //assuming file size will nvr reach the max_file_length
									  //Considering a max of 165 nodes in the cluster
	{
		OSDLOG(ERROR, "Unexpected Behaviour! No Space Left in file");
		this->file_handler->close();
		return false;
	}
	this->process_write(record, record_size); //serialize and write call for file
	OSDLOG(INFO, "Renaming Temp file to Original Ring File");
	if(::rename(make_temp_ring_file_path(this->path).c_str(), 
					make_ring_file_path(this->path).c_str()) == -1)
        {
                OSDLOG(ERROR, "Unable to rename file "<<make_temp_ring_file_path(this->path)
						     <<" : "<<strerror(errno));
		//throw OSDException::RenameFileException();
		return false;
        }
	return true;
}

void RingFileWriteHandler::process_write(recordStructure::Record * record_ptr, int64_t record_size)
/*
*@desc: actual writing interface 
*
*@param: record to be serialized
*@param: record size to be serialized 
*/
{
        char *buffer = new char[record_size];
        memset(buffer, '\0', record_size);
        this->serializor->osm_info_file_serialize(record_ptr, buffer, record_size);
        this->file_handler->write(buffer, record_size);
        delete[] buffer;
}

std::string const make_ring_file_path(std::string const path)
/*
*@desc: interface to prepare path for ring file
*
*@param: path of info file location
*
*@return: full path of ring file
*/
{
        return path + "/" + RING_FILE_NAME;
}

std::string const make_temp_ring_file_path(std::string const path)
/*
*@desc: interface to prepare remprary path for ring file
*
*@param: path of info file
*
*@return: full path of temprary ring file
*/
{
        std::string temp_string = "temp";
        return path + "/" + RING_FILE_NAME + "_" + boost::lexical_cast<std::string>(temp_string);
}

} // namespace ring_file_manager
