#ifndef __UTIL_HELPER_H__
#define __UTIL_HELPER_H__

#include <string>

namespace helper_utils{

enum lockType{
	WRITE = 1,
	READ = 2,
	READ_WRITE = 3
};

struct transactionIdElement{
	int64_t transaction_id;
	int operation;
};

class Callback
{
    public:
//        Callback(boost::condition_variable condition, boost::mutex::scoped_lock lock);
        Callback();
        ~Callback();
        void execute_callback(std::size_t offset);
//        boost::condition_variable condition;
//        boost::mutex::scoped_lock lock;

    private:
        const std::size_t offset;
};

class Request
{
/* Dummy implementation of Request class
 * Container service will convert the python request class to C++ class
 * which will be used by the Transaction Library
 */
    public:
        Request(std::string object_name, std::string request_method, int lock_type, std::string server_id);
	Request(const Request&);
        ~Request();
        std::string  get_object_name();
        std::string  get_object_id();
        std::string  get_request_method();
        char * get_node_name();
	int get_operation_type();
        int get_port();
        char * get_service();
        int get_permission();
        std::size_t get_length();
        char get_sub_state();
        int set_reserved(int bytes);

    private:
        std::string object_name;
        std::string request_method;
	int operation;
        char node_name[255];
        int port;
        char service[255];
        int permissions;
        std::size_t length;
        char substate[255];
        int reserved;
	std::string server_id;
};



}//helper_utils namespace ends

#endif
