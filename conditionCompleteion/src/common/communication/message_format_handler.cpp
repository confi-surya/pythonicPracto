#include <iostream>
//#include <cmath>
//#include <netinet/in.h>
#include "libraryUtils/md5.h"
#include "communication/message_type_enum.pb.h"
#include "communication/message_format_handler.h"
#include "communication/http_handler.h"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>

namespace ProtocolStructure
{

MessageFormatHandler::MessageFormatHandler(
):
	is_timeout(false)
{
}

void MessageFormatHandler::set_timeout(boost::shared_ptr<Communication::Communication> sock, uint32_t const timeout)
{

	this->timer.reset(new boost::asio::deadline_timer(*(sock->get_io_service()), boost::posix_time::seconds(timeout)));
	if (timeout > 0){
		COMMLOGGER(DEBUG, "Timeout is : " << timeout);
		this->timer->async_wait(boost::bind(
				&ProtocolStructure::MessageFormatHandler::set_timeout_flag,
				this,
				sock,
				boost::asio::placeholders::error
			)
		);
	}
	else{
		this->timer->async_wait(boost::bind(
				&ProtocolStructure::MessageFormatHandler::do_nothing,
				this, boost::asio::placeholders::error
			)
		);
	}
	
}

boost::shared_ptr<MessageResultWrap> MessageFormatHandler::get_msg(){
	return this->result;
}

void MessageFormatHandler::set_timeout_flag(
	boost::shared_ptr<Communication::Communication> sock,
	const boost::system::error_code& ec
)
{
	if (ec.value() == 0){
		COMMLOGGER(ERROR, "Timeout handler is invoked");
		this->is_timeout = true;
		sock->stop_operations();
	}
}

void MessageFormatHandler::do_nothing(const boost::system::error_code& ec)
{
}

void MessageFormatHandler::read_payload(
	char * payload,
	uint32_t size,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	const boost::system::error_code &er,
	uint64_t bytes
)
{
	if (this->is_timeout){
		this->result->set_error_code(Communication::TIMEOUT_OCCURED);
		this->result->set_error_message("timeout occured while reading payload");
		delete [] payload;
		COMMLOGGER(ERROR, "Timeout occured while reading payload");
		failure_handler();
	}
	else if (er.value() == 2 || er.value() == 9){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
		COMMLOGGER(ERROR, "got connection issue : " << er.value() << " , message : " << er.message());
		delete [] payload;
		failure_handler();
	}
	else if (er.value() > 0){
		this->timer->cancel();
		this->result->set_error_code(Communication::FAILED_TO_READ_PAYLOAD);
		this->result->set_error_message("error while reading payload");
		delete [] payload;
		COMMLOGGER(ERROR, "Error while reading payload : " << er.message());
		failure_handler();
	}
	else if (bytes != size){
		this->timer->cancel();
		this->result->set_error_code(Communication::FAILED_TO_READ_PAYLOAD);
		this->result->set_error_message("incomplete packet recieved while reading payload");
		delete [] payload;
		COMMLOGGER(ERROR, "Incomplete payload recieved");
		failure_handler();
	}
	else{
		this->timer->cancel();
		this->result->set_payload(payload, bytes);
		success_handler(this->result);
	}
}

void MessageFormatHandler::verify_headers_handler(
	char * message_header,
	boost::shared_ptr<Communication::Communication> sock,
//	boost::shared_ptr<comm_messages::Message> msg,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	const boost::system::error_code &ec,
	uint64_t bytes
)
{
	if (this->is_timeout){
		this->result.reset(new MessageResultWrap("timeout occured while reading headers", Communication::TIMEOUT_OCCURED));
		delete [] message_header;
		COMMLOGGER(ERROR, "Timeout occured while reading protocol headers");
		failure_handler();
	}
	else if (ec.value() == 2 || ec.value() == 9){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
		delete [] message_header;
		COMMLOGGER(ERROR, "got connection issue : " << ec.value() << " , message : " << ec.message());
		failure_handler();
	}
	else if (ec.value() > 0){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap(ec.message(), Communication::FAILED_TO_READ_HEADER));
		delete [] message_header;
		COMMLOGGER(ERROR, "Error occured while reading protocl headers : " << ec.message());
		failure_handler();
	}
	else if (bytes != HEADER_CHUNK_READ_SIZE){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Incomplete pakcet recieved", Communication::FAILED_TO_READ_HEADER));
		delete [] message_header;
		COMMLOGGER(ERROR, "Incomplete packet recieved : " << bytes << " .Actual expected : " << HEADER_CHUNK_READ_SIZE);
		failure_handler();
	}
	else if (!this->protocol.verify_headers(message_header)){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Invalid Protocol", Communication::INVALID_HEADER));
		delete [] message_header;
		COMMLOGGER(ERROR, "Protocol header mismatched");
		failure_handler();
	}
	else{
		uint32_t type = this->protocol.get_type(message_header);
		uint32_t size = this->protocol.get_size(message_header);
		this->result.reset(new MessageResultWrap(type));

		COMMLOGGER(INFO, "recieved message type : " << type << ", payload size : " << size);

		char * payload = new char[size];
		memset(payload, '\0', size);

		sock->read_socket(
			payload,
			boost::bind(
				&ProtocolStructure::MessageFormatHandler::read_payload,
				this,
				payload,
				size,
				success_handler,
				failure_handler,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			),
			size
		);
		delete [] message_header;
	}
}

void MessageFormatHandler::get_message_object(
	boost::shared_ptr<Communication::Communication> sock,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	uint32_t timeout
)
{
	if (sock->is_connected()){
		this->result.reset(new MessageResultWrap("Empty Result", Communication::NO_OPERATION));
	}
	else{
		this->result.reset(new MessageResultWrap("Communication object is not prepared", Communication::SOCKET_NOT_CONNECTED));
		failure_handler();
		return;
	}

	this->set_timeout(sock, timeout);

	char * message_header = new char[HEADER_CHUNK_READ_SIZE];

	sock->read_socket(
		message_header,
		boost::bind(
			&ProtocolStructure::MessageFormatHandler::verify_headers_handler,
			this,
			message_header,
			sock,
			success_handler,
			failure_handler,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		),
		HEADER_CHUNK_READ_SIZE
	);
}

void MessageFormatHandler::send_message_handler(
	char * message,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	const boost::system::error_code &ec,
	uint64_t bytes
)
{
	if (this->is_timeout){
		this->result.reset(new MessageResultWrap("timeout occured", Communication::TIMEOUT_OCCURED));
		COMMLOGGER(ERROR, "Timeout error occured while sending message");
		failure_handler();
	}
	else if (ec.value() == 2 || ec.value() == 9){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
		COMMLOGGER(ERROR, "got connection issue : " << ec.value() << " , message : " << ec.message());
		failure_handler();
	}
	else if (ec.value() > 0){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap(ec.message(), Communication::FAILED_TO_SEND_MESSAGE));
		COMMLOGGER(ERROR, "Error occured while sending message : " << ec.message());
		failure_handler();
	}
	else{
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("", 0));
		success_handler(this->result);
		COMMLOGGER(DEBUG, "Message sent successfully on network");
	}

	delete [] message;
}

void MessageFormatHandler::send_message(
	boost::shared_ptr<Communication::Communication> sock,
	comm_messages::Message * msg,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	uint32_t timeout
)
{
	if (sock->is_connected()){
		this->result.reset(new MessageResultWrap("Empty Result", Communication::NO_OPERATION));
	}
	else{
		this->result.reset(new MessageResultWrap("Communication object is not prepared", Communication::SOCKET_NOT_CONNECTED));
		failure_handler();
		return;
	}

	this->set_timeout(sock, timeout);

	uint32_t type = msg->get_type();
	COMMLOGGER(INFO, "Message type : " << type << " to be sent");
	std::string serialized_string;
	if (!msg->serialize(serialized_string)){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("unable to serialize message", Communication::SERIALIZATION_FAILED));
		COMMLOGGER(ERROR, "Unable to serialize the message");
		failure_handler();
		return;
	}

	COMMLOGGER(DEBUG, "message serialized. size : " << serialized_string.size() << " . md5sum : " << md5(serialized_string.c_str(), serialized_string.size()));
	uint32_t size = this->protocol.get_complete_size_of_packet(serialized_string);

	char * packet_string = new char[size];
	this->protocol.packetize(packet_string, serialized_string, serialized_string.size(), type);

	sock->write_socket(
			packet_string,
			boost::bind(
				&ProtocolStructure::MessageFormatHandler::send_message_handler,
				this,
				packet_string,
				success_handler,
				failure_handler,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			),
			size

		);
}

void MessageFormatHandler::extract_response_header(
	boost::shared_ptr<Communication::SynchronousCommunication> sock,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	const boost::system::error_code &ec,
	uint64_t bytes

){
	if (this->is_timeout){
		this->result.reset(new MessageResultWrap("timeout occured http request", Communication::TIMEOUT_OCCURED));
		COMMLOGGER(ERROR, "timeout occured in http request");
		return;
	}
	if (ec.value() == 2 || ec.value() == 9){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
		COMMLOGGER(ERROR, "got connection issue : " << ec.value() << " , message : " << ec.message());
		return;
	}
	else if (ec.value() > 0){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap(ec.message(), Communication::FAILED_TO_READ_BODY));
		COMMLOGGER(ERROR, "Error in http request : " << ec.message());
		return;
	}

	boost::asio::streambuf buff(512);
	boost::system::error_code er;
	std::string response_header_string;
	uint64_t bytes_transfered = 0;
	uint64_t total_bytes_read = 0;

	bytes_transfered = sock->read_socket_until(response_header_string, HttpSupport::http_end_marker, er);
	total_bytes_read = response_header_string.size();

	if (this->is_timeout){
		this->result.reset(new MessageResultWrap("timeout occured while reading body", Communication::TIMEOUT_OCCURED));
		COMMLOGGER(ERROR, "timeout occured while reading http request response headers");
		return;
	}
	if (er.value() == 2 || er.value() == 9){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
		COMMLOGGER(ERROR, "got connection issue : " << er.value() << " , message : " << ec.message());
		return;
	}
	else if (er.value() > 0){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap(er.message(), Communication::FAILED_TO_READ_BODY));
		COMMLOGGER(ERROR, "Error occured while reading http request response headers : " << er.message());
		return;
	}

	uint32_t content_length = 0;
	boost::cmatch what_content_length;
	boost::regex expression("Content-Length: ([0-9]*)");
	if (!boost::regex_search(response_header_string.c_str(), what_content_length, expression)){
	}
	else{
		std::string cl = what_content_length[1];
		content_length = atoi(cl.c_str());
	}

	COMMLOGGER(DEBUG, "Content-length : " << content_length);

	uint32_t type = 0;
	boost::cmatch what_type;
	boost::regex expression_type("Message-Type: ([0-9]*)");
	if (!boost::regex_search(response_header_string.c_str(), what_type, expression_type)){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Message type header not found", Communication::TYPE_NOT_FOUND));
		COMMLOGGER(ERROR, "Message type header not found");
		return;
	}
	else{
		std::string type_str = what_type[1];
		type = atoi(type_str.c_str());
		COMMLOGGER(DEBUG, "type found in http header: " << type);
	}

	if (content_length == 0){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap(type));
		COMMLOGGER(DEBUG, "content length is 0 so nothing to read in body");
		return;
	}
	else{
		char * body = new char[content_length];
		memset(body, '\0', content_length);

		uint64_t diff = content_length - (total_bytes_read - bytes_transfered);
		memcpy(body, &response_header_string[bytes_transfered], total_bytes_read - bytes_transfered);

		COMMLOGGER(DEBUG, "size : " << bytes_transfered << ", total bytes : " << total_bytes_read);

		if (diff > 0){
			boost::system::error_code es;
			uint64_t t_bytes = sock->read_socket_without_handler(&body[total_bytes_read - bytes_transfered], diff, es);

			if (this->is_timeout){
				this->result.reset(new MessageResultWrap("timeout occured while reading body", Communication::TIMEOUT_OCCURED));
				COMMLOGGER(ERROR, "Timeout occured while reading http response body");
				return;
			}
			if (es.value() == 2 || es.value() == 9){
				this->timer->cancel();
				this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
				COMMLOGGER(ERROR, "got connection issue : " << es.value() << " , message : " << es.message());
				return;	
			}
			if (es.value() > 0){
				this->timer->cancel();
				this->result.reset(new MessageResultWrap(es.message(), Communication::FAILED_TO_READ_BODY));
				COMMLOGGER(ERROR, "Error while reading http response body : " << es.message());
				return;
			}
			if (t_bytes != diff){
				this->timer->cancel();
				this->result.reset(new MessageResultWrap("Incomplete body recieved", Communication::FAILED_TO_READ_BODY));
				COMMLOGGER(ERROR, "Incomplete body recived : " << t_bytes);
				return;
			}
		}
		this->timer->cancel();
		this->result.reset(new MessageResultWrap(type));
		this->result->set_payload(body, content_length);
	}
}

void MessageFormatHandler::send_http_body(
	boost::shared_ptr<Communication::SynchronousCommunication> sock,
	std::string serialized_string,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	const boost::system::error_code &ec,
	uint64_t bytes
){
	if (this->is_timeout){
		this->result.reset(new MessageResultWrap("timeout occured while reading headers", Communication::TIMEOUT_OCCURED));
		COMMLOGGER(ERROR, "timeout occured sending http request headers");
		return;
	}
	if (ec.value() == 2 || ec.value() == 9){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
		COMMLOGGER(ERROR, "got connection issue : " << ec.value() << " , message : " << ec.message());
		return;
	}
	if (ec.value() > 0){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap(ec.message(), Communication::FAILED_TO_SEND_MESSAGE));
		COMMLOGGER(ERROR, "Error while sending http request headers : " << ec.value());
		return;
	}

	char * ack_header_string = new char[ACK_HEADER_SIZE];
	boost::system::error_code er;

	int ret = sock->read_socket_without_handler(ack_header_string, ACK_HEADER_SIZE, er);

	if (this->is_timeout){
		this->result.reset(new MessageResultWrap("timeout occured while reading headers", Communication::TIMEOUT_OCCURED));
		COMMLOGGER(ERROR, "timeout occured while reading Expect continue header");
		delete [] ack_header_string;
		return;
	}
	if (er.value() == 2 || er.value() == 9){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Connection issue", Communication::CONNECTION_ISSUE));
		COMMLOGGER(ERROR, "got connection issue : " << er.value() << " , message : " << er.message());
		delete [] ack_header_string;
		return;
	}
	if (er.value() > 0){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Error Occured", Communication::ERROR_OCCURED));
		COMMLOGGER(ERROR, "Error while reading Expect continue headers : " << er.message());
		delete [] ack_header_string;
		return;
	}

	boost::cmatch what_type;
	boost::regex expression_type("HTTP/1.1 100 Continue\r\n\r\n");
	if (!boost::regex_search(ack_header_string, what_type, expression_type)){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Expect continue response header not found", Communication::TYPE_NOT_FOUND));
		COMMLOGGER(ERROR, "100 Continue header not recieved or is corrupt or is incomplete.");
		delete [] ack_header_string;
		return;
	}

	delete [] ack_header_string;

	sock->write_socket(
		serialized_string.c_str(),
		boost::bind(
			&ProtocolStructure::MessageFormatHandler::extract_response_header,
			this,
			sock,
			success_handler,
			failure_handler,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		),
		serialized_string.size()
	);
}

void MessageFormatHandler::send_http_request(
	boost::shared_ptr<Communication::SynchronousCommunication> sock,
	comm_messages::Message * sending_message,
	const std::string & ip,
	const uint32_t port,
	const std::string & http_request_method,
	boost::function<void(boost::shared_ptr<MessageResultWrap>)> success_handler,
	boost::function<void()> failure_handler,
	uint32_t timeout
)
{
	if (sock->is_connected()){
		this->result.reset(new MessageResultWrap("Empty Result", Communication::NO_OPERATION));
	}
	else{
		this->result.reset(new MessageResultWrap("Communication object is not prepared", Communication::SOCKET_NOT_CONNECTED));
		failure_handler();
		return;
	}

	this->set_timeout(sock, timeout);

	uint32_t type = sending_message->get_type();
	COMMLOGGER(DEBUG, "Message type : " << type << " to be sent via http");
	std::string serialized_string;

	std::map<std::string, std::string> header_map = sending_message->get_header_map();
	if (!sending_message->serialize(serialized_string)){
		this->timer->cancel();
		this->result.reset(new MessageResultWrap("Unable to serialize the message for http request", Communication::SERIALIZATION_FAILED));
		COMMLOGGER(ERROR, "Unable to serialize the message when sending http request");
		return;
	}

	COMMLOGGER(DEBUG, "Message serialized. size : " << serialized_string.size());

	std::string sending_string;
	sending_string += http_request_method;
	sending_string += " ";
	sending_string += HttpSupport::http_dummy_path;
	sending_string += " ";

	HttpSupport::HttpMessage http_obj;
	http_obj.attach_generic_headers(serialized_string.size(), header_map, sending_string, ip, port);

	//NOTE: sending initial sending header to the server
	
	if (serialized_string.size() > 0){
		sock->write_socket(
			sending_string.c_str(),
			boost::bind(
				&ProtocolStructure::MessageFormatHandler::send_http_body,
				this,
				sock,
				serialized_string,
				success_handler,
				failure_handler,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			),
			sending_string.size()
		);
	}
	else{
		sock->write_socket(
			sending_string.c_str(),
			boost::bind(
				&ProtocolStructure::MessageFormatHandler::extract_response_header,
				this,
				sock,
				success_handler,
				failure_handler,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			),
			sending_string.size()
		);
		
	}
}

Protocol::Protocol(
)
{
	strncpy(this->header.banner, &ProtocolStructure::banner[0], BANNER_CHUNK_LENGTH);
}

bool Protocol::verify_headers(char * header_string){
	if (std::strncmp(header_string, &ProtocolStructure::banner[0], BANNER_CHUNK_LENGTH) != 0){
		return false;
	}
	headerStructure * headerStruct = reinterpret_cast<headerStructure *>(header_string);
	
	this->set_header_size(headerStruct->size);
	this->set_header_type(headerStruct->type);
	headerStruct = NULL;
	delete headerStruct;
	return true;
}

uint32_t Protocol::get_size(char * message_header){
	return this->header.size;
}

uint32_t Protocol::get_type(char * message_header){
	return this->header.type;
}

void Protocol::set_header_size(uint32_t size){
	this->header.size = size;
}

void Protocol::set_header_type(uint32_t type){
	this->header.type = type;
}

uint32_t Protocol::get_complete_size_of_packet(std::string & message){
	uint32_t size = message.size();
	size += HEADER_CHUNK_READ_SIZE;
	return size;
}

void Protocol::packetize(char * packet_string, std::string & message, uint32_t size, uint32_t type){

	this->set_header_size(size);
	this->set_header_type(type);

	memset(packet_string, '\0', HEADER_CHUNK_READ_SIZE + size);
	memcpy(packet_string, reinterpret_cast<char *>(&this->header), HEADER_CHUNK_READ_SIZE);
	memcpy(&packet_string[HEADER_CHUNK_READ_SIZE], message.c_str(), size);
}

}
