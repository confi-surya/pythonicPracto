#ifndef __lock_h__
#define __lock_h__

#include <iostream>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>

using namespace std;

enum lock_status
{
	LOCK_ACQUIRED = 1,
	LOCK_REJECTED = 2,
	LOCK_NOT_NEEDED = 3
};

/*Lock file*/
class LockFile
{
public:
	LockFile(std::string path, boost::function<int(void)>);
	~LockFile();
	int get_lock(std::string & election_version);
	int release_lock(std::string & election_version);

private:
	std::string lock_file_base;
	std::string gfs_path;
	std::string get_lock_file();
	boost::function<int(void)> check_hfs_stat;
};


/*Election File*/
class ElectionFile
{
public:
	ElectionFile(std::string path);
	int create_file(std::string);
	int rename(std::string, std::string);
	int read(int const fd, char * buffer, uint64_t const bufferLength);
	int write(int const fd, char const * buffer, uint64_t const bufferLength);
	int remove_file(std::string);
	int open(std::string path_name, int const mode);
	int close(int const fd, bool const sync);
private:
	std::string gfs_path;
	std::string elect_file_name;
};

class ElectionFileHandler
{
public:
	ElectionFileHandler(std::string path);
	int create_election_file(int file_size);
	int write_on_offset(const char *, int, int);
	int sweep_election_file();
	std::string getGL(int record_size, int);
	bool election_file_exist();
	bool post_election_file_exist();
	int temp_file_cleaner(int);
private:
	std::string gfs_path;
	boost::shared_ptr<ElectionFile> elect_file;
	std::string temp_path;
	std::string elect_file_name;
	int buffer_size;

};

#endif
