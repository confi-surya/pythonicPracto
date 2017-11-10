#ifndef _PID_FILE_H_123614
#define _PID_FILE_H_123614

namespace LockFileBlocking
{
enum Enum{
        BLOCKING = 0,
        NOT_BLOCKING = 1
};
}
// creates a file of the given name and locks it
// returns the fd of the pid file, and -1 on error
// the file remains open after the function successfully returns
int create_lock_file(char const * const file_name, LockFileBlocking::Enum should_block);

//simply close the file
void unlock_file(int file_fd);

// creates a pid file of the given name and locks it
int create_pid_file(char const * const pid_file_name);

// writes the pid of the current process to the given fd.
// returns -1 on failure and 0 on success
// it does not close the fd in any case
int write_pid_to_pid_file(int pid_file_fd);



#endif // #ifndef _PID_FILE_H_123614

