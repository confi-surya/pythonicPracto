#ifndef __STATUS_H__
#define __STATUS_H__

#include <string>
#include <boost/unordered_map.hpp>
#include "libraryUtils/object_storage_exception.h"

using namespace OSDException;

namespace StatusAndResult
{

template <typename S>

class Status
{
private:
	S data;
	bool return_status;
	uint16_t error_code;
	std::string message;	
public:
	Status();
	Status(S status);
	Status(S value, std::string, bool, uint16_t);
	void set_return_status(bool status);
	void set_failure_reason(std::string msg);
	bool get_return_status(void);
	uint16_t get_error_code(void);
	std::string get_failure_reason(void);
	S get_value(void);
	~Status();
};

template <typename S>
Status<S>::Status()
{

}

template <typename S>
Status<S>::Status(S status)
{
        this->status = status;
}

template <typename S>
Status<S>::Status(S value, std::string msg = "", bool status = false, uint16_t code = 0)
{
        this->return_status = status;
        this->data = value;
	this->message = msg;
	this->error_code = code;
}

template <typename S>
void Status<S>::set_return_status(bool status)
{
        this->return_status = status;
}

template <typename S>
void Status<S>::set_failure_reason(std::string msg)
{
        this->message = msg;
}

template <typename S>
bool Status<S>::get_return_status(void)
{
        return this->return_status;
}

//UNCOVERED_CODE_BEGIN: 
template <typename S>
std::string Status<S>::get_failure_reason(void)
{
        return this->message;
                                                                                                                   
}
//UNCOVERED_CODE_END
template <typename S>
S Status<S>::get_value(void)
{
        return this->data;
}

//UNCOVERED_CODE_BEGIN: 
template <typename S>
uint16_t Status<S>::get_error_code(void)
{
	return this->error_code;
}
//UNCOVERED_CODE_END
template <typename S>
Status<S>::~Status()
{

}
}
#endif 

