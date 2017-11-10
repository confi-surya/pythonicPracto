#include <iostream>
#include "libraryUtils/helper.h"
#include "boost/lexical_cast.hpp"

namespace helper_utils
{

Callback::Callback() : offset(0)
{
}

Request::Request( 
	std::string object_name, 
	std::string request_meth,
	int lock_type,
	std::string server_id
) :  
	object_name(object_name), 
	request_method(request_meth),
	operation(lock_type),
	server_id(server_id)
{
	if (lock_type != READ && lock_type != WRITE)
	{
		std::cout << "not a valid operation : " << lock_type << std::endl;
		this->operation = WRITE;
	}
}

Request::Request(const Request & obj)
{
	this->object_name = obj.object_name;
	this->request_method = obj.request_method;
	this->operation = obj.operation;
	this->server_id = obj.server_id;
}

Request::~Request()
{
}

std::string Request::get_object_name()
{
	return this->object_name;
}

std::string Request::get_object_id()
{
	return this->server_id;
}

std::string Request::get_request_method()
{
	return this->request_method;
}

int Request::get_operation_type()
{
	return this->operation;
}

} // namespace helper_utils
