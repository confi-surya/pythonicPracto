#include "communication/message_result_wrapper.h"

MessageResultWrap::MessageResultWrap(const uint32_t type):
	payload(NULL),
	type(type),
	error_message(""),
	error_code(0),
	size(0)
{
}

MessageResultWrap::MessageResultWrap(
	std::string error_message,
	uint32_t error_code
):
	payload(NULL),
	type(255),
	error_message(error_message),
	error_code(error_code),
	size(0)
{
}


MessageResultWrap::MessageResultWrap(
	char * payload,
	const uint32_t type,
	std::string error_message,
	uint32_t error_code
):
	payload(payload),
	type(type),
	error_message(error_message),
	error_code(error_code),
	size(0)
{
}

MessageResultWrap::~MessageResultWrap()
{
/*	if (this->payload){
		delete [] this->payload;
	}*/
}

const uint32_t MessageResultWrap::get_type()
{
	return this->type;
}

std::string MessageResultWrap::get_error_message()
{
	return this->error_message;
}

void MessageResultWrap::set_error_message(std::string error_message)
{
	this->error_message = error_message;
}

void MessageResultWrap::set_payload(char * payload, uint32_t size)
{
	this->payload = payload;
	this->size = size;
}

char * MessageResultWrap::get_payload()
{
	return this->payload;
}

uint32_t MessageResultWrap::get_size()
{
	return this->size;
}

uint32_t MessageResultWrap::get_error_code()
{
	return this->error_code;
}

void MessageResultWrap::set_error_code(uint32_t error_code)
{
	this->error_code = error_code;
}
