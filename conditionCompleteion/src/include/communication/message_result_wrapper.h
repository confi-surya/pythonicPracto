#ifndef __MESSAGE_RESULT_WRAPPER__
#define __MESSAGE_RESULT_WRAPPER__

#include "communication/message.h"

class MessageResultWrap
{
	public:
		MessageResultWrap(const uint32_t type);
		MessageResultWrap(
			std::string,
			uint32_t
		);
		MessageResultWrap(
			char *,
			const uint32_t type,
			std::string,
			uint32_t
		);
		~MessageResultWrap();

		const uint32_t get_type();
		std::string get_error_message();
		void set_error_message(std::string);
		void set_payload(char *, uint32_t);
		char * get_payload();
		uint32_t get_size();
		uint32_t get_error_code();
		void set_error_code(uint32_t);

	private:
		char * payload;
		const uint32_t type;
		std::string error_message;
		uint32_t error_code;
		uint32_t size;
};

#endif
