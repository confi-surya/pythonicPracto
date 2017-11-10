#include "libraryUtils/file_handler.h"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/file.h>
#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"
#include <errno.h>
#include <string.h>

using namespace object_storage_log_setup;

namespace fileHandler
{

FileHandler::FileHandler(
	std::string const path,
	openFileMode const mode
)
	: path(path), mode(mode), fd(-1)
{
	this->open(path, mode);		//TODO: Remove open from constructor
}

FileHandler::~FileHandler()
{
	this->close();
}

void FileHandler::open(std::string const path, openFileMode const mode)
{
	switch (mode)
	{
	case OPEN_FOR_READ:
		this->fd = ::open(path.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		break;
	case OPEN_FOR_APPEND:
		this->fd = ::open(path.c_str(), O_CREAT | O_APPEND | O_WRONLY,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		break;
	case OPEN_FOR_WRITE:
		this->fd = ::open(path.c_str(), O_CREAT | O_WRONLY,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		break;
	case OPEN_FOR_READ_WRITE:
		this->fd = ::open(path.c_str(), O_RDWR,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		break;
	case OPEN_FOR_CREATE:
		this->fd = ::open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		break;
	}
	if(this->fd == -1)
	{
		OSDLOG(INFO, "Unable to open file "<<this->path<<" : "<<strerror(errno));
		if (errno == 2)
		{
			throw OSDException::FileNotFoundException();
		}
		/*else if(errno == 17)
		{
			OSD_EXCEPTION_THROW(OSDException::FileExistException, "Unable to open file "<<this->path<<" : "<<strerror(errno));
		}*/
		throw OSDException::OpenFileException();
	}
	if (::fcntl(this->fd, F_SETFD, FD_CLOEXEC) == -1)
	{
		OSDLOG(ERROR, "File Handler:unable to set close exec flag on fd : " << errno);
		exit(1);
	}
	//This is done for bug HYD-7377, so that fd created in parent process address space are not shared with child process address space
}

void FileHandler::reopen()
{
	this->close();
	this->open(this->path, this->mode);
}

/*void FileHandler::rename(std::string old_name, std::string new_name)
{
	::rename(old_name.c_str(), new_name.c_str());
}*/

void FileHandler::open_tmp_file(std::string const suffix)
{
	this->close();
	this->open(this->path + suffix, OPEN_FOR_CREATE);
}

void FileHandler::rename_tmp_file(std::string const suffix)
{
	std::string tmpfile = this->path + suffix;
	if(::rename(tmpfile.c_str(), this->path.c_str()) == -1)
	{
		OSDLOG(INFO, "Unable to rename file "<<this->path<<" : "<<strerror(errno));
		throw OSDException::RenameFileException();
		//OSD_EXCEPTION_THROW(OSDException::RenameFileException, "Unable to rename file "<<this->path<<" : "<<strerror(errno));
	}
}

void FileHandler::close()
{
	if (this->fd != -1)
	{
		if (::close(this->fd) == -1)
		{	
			OSDLOG(INFO, "Unable to close file "<<this->path<<" : "<<strerror(errno));
			throw OSDException::CloseFileException();
			//OSD_EXCEPTION_THROW(OSDException::CloseFileException, "Unable to close file "<<this->path<<" : "<<strerror(errno));
		}
		else
		{
			this->fd = -1;
		}
	}
	else
	{
		OSDLOG(DEBUG, "No open file handle to close file "<<this->path);
	}
}

int64_t FileHandler::write(char * buff, int64_t bytes)
{
	int64_t ret_value = ::write(this->fd, buff, bytes);
	if ( ret_value == -1 )
	{
		OSDLOG(INFO, "Unable to write file "<<this->path<<" : "<<strerror(errno));
		throw OSDException::WriteFileException();
		//OSD_EXCEPTION_THROW(OSDException::WriteFileException, "Unable to write file "<<this->path<<" : "<<strerror(errno));
	}
	else
	{
		return ret_value;
	}
}


int64_t FileHandler::write(char * buff, int64_t bytes, int64_t offset)
{
	int64_t ret_value = ::pwrite(this->fd, buff, bytes, offset);
	if (ret_value  == -1)
	{
		OSDLOG(INFO, "Unable to write file "<<this->path<<" : "<<strerror(errno));
		throw OSDException::WriteFileException();
		//OSD_EXCEPTION_THROW(OSDException::WriteFileException, "Unable to write file "<<this->path<<" : "<<strerror(errno));

	}
	{
		return ret_value;
	}
}

int64_t FileHandler::read(char * buff, int64_t bytes)
{
	int64_t ret_value = ::read(this->fd, buff, bytes);
	if(ret_value == -1) 
	{
		OSDLOG(INFO, "Unable to read file "<<this->path<<" : "<<strerror(errno));
		throw OSDException::ReadFileException();
		//OSD_EXCEPTION_THROW(OSDException::ReadFileException, "Unable to read file "<<this->path<<" : "<<strerror(errno));
	}
	else
	{
		return ret_value;
	}
}

int64_t FileHandler::read(char * buff, int64_t bytes, int64_t offset)
{

	int64_t ret_value = ::pread(this->fd, buff, bytes, offset);
	if(ret_value == -1) 
	{
		OSDLOG(INFO, "Unable to read file "<<this->path<<" : "<<strerror(errno));
		throw OSDException::ReadFileException();
		//OSD_EXCEPTION_THROW(OSDException::ReadFileException, "Unable to read file "<<this->path<<" : "<<strerror(errno));
	}
	{
		return ret_value;
	}
}

int64_t FileHandler::seek_offset( int64_t fd, int64_t offset, int64_t whence)
{
	int ret_value = ::lseek(fd, offset, whence);
	if (ret_value == -1)
	{
		OSDLOG(INFO, "Unable to seek file "<<this->path<<" : "<<strerror(errno));
		throw OSDException::SeekFileException();
		//OSD_EXCEPTION_THROW(OSDException::SeekFileException, "Unable to seek file "<<this->path<<" : "<<strerror(errno));
	}
	else
	{
		return ret_value;
	}
}


int64_t FileHandler::read_end(char *buff, int64_t bytes, int64_t offset)
{
	seek_offset(this->fd, -offset, SEEK_END);
	int ret_value = ::read(this->fd, buff, bytes);
	if (ret_value == -1)
	{
		OSDLOG(INFO, "Unable to read file "<<this->path<<" : "<<strerror(errno));
		throw OSDException::ReadFileException();
		//OSD_EXCEPTION_THROW(OSDException::ReadFileException, "Unable to read file "<<this->path<<" : "<<strerror(errno));
	}
	else
	{
		return ret_value;
	}

}

int64_t FileHandler::seek(int64_t offset)
{
	return this->seek_offset(this->fd, offset, SEEK_SET);
}

int64_t FileHandler::tell()
{
	return this->seek_offset(this->fd, 0, SEEK_CUR);
}

int64_t FileHandler::seek_end()
{
	return this->seek_offset(this->fd, 0, SEEK_END);
}

int64_t FileHandler::truncate(int64_t offset)
{
        OSDLOG(INFO,"Truncating at offset : " << offset);
        if ( ftruncate(this->fd, offset) != 0 )
        {
                OSDLOG(INFO, "Unbale to Truncate the file: " <<this->path <<" : "<<strerror(errno));
                return -1;
        }
        else
        {
                return 0;
        }
}


int64_t FileHandler::get_end_offset()
{
	int64_t cur_offset = this->tell();
	int64_t file_size = this->seek_offset(this->fd, 0L, SEEK_END);
	this->seek(cur_offset);
	return file_size;
}

void FileHandler::insert_padding(int64_t padding_bytes)
{
	if(padding_bytes < 0)
	{
		OSDLOG(WARNING, "No padding bytes to add....!! ");
		return;
	}
	char *buff = new char[padding_bytes];
	memset(buff, '$', padding_bytes);

	this->write(buff, padding_bytes);
	delete [] buff; //asrivastava
}

bool FileHandler::sync()
{
	if (::fsync(this->fd) == 0) // return -1 on failure
		return true;
	OSDLOG(INFO, "Unable to sync file "<<this->path<<" : "<<strerror(errno));
	throw OSDException::SyncFileException();
	//OSD_EXCEPTION_THROW(OSDException::SyncFileException, "Unable to sync file "<<this->path<<" : "<<strerror(errno));
}

AppendWriter::AppendWriter(boost::shared_ptr<FileHandler> file, int64_t offset,
		uint32_t buffer_size)
	: file(file), next_offset(offset), curr_size(0)
{
	this->buffer.resize(buffer_size);
}

void AppendWriter::append(void const * data, uint32_t size)
{
	DASSERT(this->buffer.size() >= size, "data size exceeds buffer size; size=" << size);
	if (this->curr_size + size > this->buffer.size())
	{
		this->flush();
	}
	::memcpy(&this->buffer[this->curr_size], data, size);
	this->curr_size += size;
}

void AppendWriter::flush()
{
	this->file->write(&this->buffer[0], this->curr_size, this->next_offset);
	this->next_offset += this->curr_size;
	this->curr_size = 0;
}

}
