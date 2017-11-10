#ifndef __MESSAGE_FORMAT_HANDLER_H__
#define __MESSAGE_FORMAT_HANDLER_H__

#include <boost/static_assert.hpp>
#include <boost/function.hpp>
#include "libraryUtils/object_storage_log_setup.h"
#include "communication/communication.h"
#include "communication/message.h"
#include "communication/message_result_wrapper.h"

namespace ProtocolStructure
{

#define RESPONSE_HEADER_SIZE 65536      //64KB chunk
#define ACK_HEADER_SIZE 25
#define HEADER_CHUNK_READ_SIZE 13
#define BANNER_CHUNK_LENGTH 05
#define SIZE_CHUNK_LENGTH 04
#define TYPE_CHUNK_LENGTH 04

//BOOST_STATIC_ASSERT(sizeof(PROTOCOL_BANNER) == BANNER_CHUNK_LENGTH);

const std::string banner = "OSD01";

class Protocol{
	public:
		Protocol();
		~Protocol(){};

		bool verify_headers(char *);
		uint32_t get_size(char *);
		uint32_t get_type(char *);

		void packetize(
			char *,
			std::string &,
			uint32_t,
			uint32_t
		);

		uint32_t get_complete_size_of_packet(std::string &);

	private:
		struct __attribute__ ((__packed__)) headerStructure{
			char banner[BANNER_CHUNK_LENGTH];
			int32_t type;
			int32_t size;
		};

		headerStructure header;

		void set_header_size(uint32_t);
		void set_header_type(uint32_t);
};

class MessageFormatHandler{
	public:
		MessageFormatHandler();
		~MessageFormatHandler(){};

		void get_message_object(
			boost::shared_ptr<Communication::Communication>,
//			boost::shared_ptr<comm_messages::Message>,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()>,
			uint32_t = 0
		);
		void send_message(
			boost::shared_ptr<Communication::Communication>,
			comm_messages::Message *,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()>,
			uint32_t = 0
		);
		void send_http_request(
			boost::shared_ptr<Communication::SynchronousCommunication>,
			comm_messages::Message *,
			const std::string &,
			const uint32_t,
			const std::string &,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()> failure_handler,
			uint32_t timeout
		);

		void set_timeout_flag(boost::shared_ptr<Communication::Communication>, const boost::system::error_code &);
		void do_nothing(const boost::system::error_code &);
		boost::shared_ptr<MessageResultWrap> get_msg();


	private:
		Protocol protocol;

		void send_message_handler(
			char *,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()>,
			const boost::system::error_code &,
			uint64_t
		);
		void verify_headers_handler(
			char *,
			boost::shared_ptr<Communication::Communication>,
//			boost::shared_ptr<comm_messages::Message>,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()>,
			const boost::system::error_code &,
			uint64_t bytes
		);
		void read_payload(
			char *,
//			boost::shared_ptr<comm_messages::Message>,
			uint32_t,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()>,
			const boost::system::error_code &,
			uint64_t
		);
		void extract_response_header(
			boost::shared_ptr<Communication::SynchronousCommunication>,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()>,
			const boost::system::error_code &,
			uint64_t
		);
		void send_http_body(
			boost::shared_ptr<Communication::SynchronousCommunication>,
			std::string,
			boost::function<void(boost::shared_ptr<MessageResultWrap>)>,
			boost::function<void()>,
			const boost::system::error_code &,
			uint64_t
		);

		void set_timeout(boost::shared_ptr<Communication::Communication>, uint32_t);

		boost::shared_ptr<MessageResultWrap> result;
		bool is_timeout;
		boost::shared_ptr<boost::asio::deadline_timer> timer;
};

}
#endif
