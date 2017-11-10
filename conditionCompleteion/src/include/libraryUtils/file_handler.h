#ifndef __FILEHANDLER_H__
#define __FILEHANDLER_H__

#include <fstream>
#include <vector>
#include "boost/shared_ptr.hpp"

namespace fileHandler
{
	static const char PADDING = '$';

enum openFileMode
{
	OPEN_FOR_READ = 1,
	OPEN_FOR_APPEND, 
	OPEN_FOR_WRITE,
	OPEN_FOR_READ_WRITE,
	OPEN_FOR_CREATE,	// O_TRUNC
};

class FileHandler
{
public:
	FileHandler(std::string const path, openFileMode const mode);
	~FileHandler();
	FileHandler();
	void open(std::string const path, openFileMode const mode);
	void reopen();
	void open_tmp_file(std::string const suffix);
	void rename_tmp_file(std::string const suffix);
	int64_t seek(int64_t offset);
	int64_t seek_end();
	int64_t seek_offset(int64_t fd, int64_t offset, int64_t whence);
	int64_t truncate(int64_t offset);
	int64_t tell();
	int64_t get_end_offset();
	bool sync();
	int64_t write(char * buff, int64_t bytes);
	int64_t write(char * buff, int64_t bytes, int64_t offset);
	int64_t read(char * buff, int64_t bytes);
	int64_t read(char * buff, int64_t bytes, int64_t offset);
	int64_t read_end(char * buff, int64_t bytes, int64_t offset);
	void close();
	void insert_padding(int64_t padding_bytes);

private:
	std::string const path;
	openFileMode const mode;
	int fd;
};

class AppendWriter
{
public:
	AppendWriter(boost::shared_ptr<FileHandler> file, int64_t offset,
			uint32_t buffer_size);
	void append(void const * data, uint32_t size);
	void flush();
private:
	boost::shared_ptr<FileHandler> file;
	int64_t next_offset;
	size_t curr_size;
	std::vector<char> buffer;
};

}// fileHandler namespace ends

#endif
