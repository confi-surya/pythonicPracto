#include "communication/http_handler.h"
#include "boost/lexical_cast.hpp"

namespace HttpSupport{

HttpMessage::HttpMessage()
{
}

HttpMessage::~HttpMessage()
{
}

void HttpMessage::prepare_header_string(
	std::string & header_string,
	std::map<std::string, std::string> & header
)
{
	for (std::map<std::string, std::string>::iterator it = header.begin(); it != header.end(); ++it){
		header_string += it->first;
		header_string += HttpSupport::http_header_seperator;
		header_string += it->second;
		header_string += HttpSupport::http_delimitter;
	}
}

void HttpMessage::attach_generic_headers(
	const uint32_t & content_length,
	std::map<std::string, std::string> & header,
	std::string & header_string,
	const std::string & ip,
	const uint32_t port
){
	header_string += HttpSupport::http_protocol;
	header_string += HttpSupport::http_delimitter;
	header_string += "User-Agent: OSD communication agent";
	header_string += HttpSupport::http_delimitter;
	header_string += HttpSupport::host_header;
	header_string += HttpSupport::http_header_seperator;
	header_string += ip;
	header_string += ":";
	header_string += boost::lexical_cast<std::string>(port);
	header_string += HttpSupport::http_delimitter;
	header_string += HttpSupport::accept_header;
	header_string += HttpSupport::http_delimitter;

	this->prepare_header_string(header_string, header);

	if (content_length < 1){
		header_string += HttpSupport::http_delimitter;
		header_string += HttpSupport::http_delimitter;
		return;
	}

	header_string += HttpSupport::body_length_header;
	header_string += HttpSupport::http_header_seperator;
	header_string += boost::lexical_cast<std::string>(content_length);
	header_string += HttpSupport::http_delimitter;
	header_string += HttpSupport::continue_header;
	header_string += HttpSupport::http_header_seperator;
	header_string += HttpSupport::continue_header_value;
	header_string += HttpSupport::http_delimitter;
	header_string += HttpSupport::http_delimitter;
}

}
