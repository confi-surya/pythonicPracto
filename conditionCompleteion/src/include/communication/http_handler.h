#ifndef __HTTP_HANDLER_H__
#define __HTTP_HANDLER_H_

#include <iostream>
#include <map>
#include "communication/communication.h"

namespace HttpSupport{


const std::string send_http_method = "PUT";
const std::string http_delimitter = "\r\n";
const std::string http_header_seperator = ": ";
const std::string http_protocol = "HTTP/1.1";
const std::string host_header = "Host";
const std::string body_length_header = "Content-Length";
const std::string continue_header = "Expect";
const std::string continue_header_value = "100-continue";
const std::string http_dummy_path = "/v1/AUTH_D/ac/a";
const std::string http_end_marker = "\r\n\r\n";
const std::string http_response_ack_string = "HTTP/1.1 100 Continue\r\n\r\n";
const std::string accept_header = "Accept: */*";

class HttpMessage
{
	public:
		HttpMessage();
		~HttpMessage();

		void attach_generic_headers(
			const uint32_t &,
			std::map<std::string, std::string> &,
			std::string &,
			const std::string &,
			const uint32_t
		);

	private:
		void prepare_header_string(std::string &, std::map<std::string, std::string> &);
};

}

#endif
