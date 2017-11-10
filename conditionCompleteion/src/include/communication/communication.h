#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition_variable.hpp>
#include "communication/communication_helper.h"

namespace Communication{

#define COMMLOGGER(logLevel, msg)\
{\
	OSDLOG(logLevel, " [communication-library] : " << msg);\
}\


typedef boost::function<void(const boost::system::error_code &, uint64_t)> handler;

enum CommunicationErrorMsg
{
	SUCCESS = 0,
	FAILED_TO_READ_PAYLOAD = 1,
	FAILED_TO_READ_HEADER = 2,
	INVALID_HEADER = 3,
	TIMEOUT_OCCURED = 4,
	FAILED_TO_SEND_MESSAGE = 5,
	NO_OPERATION = 6,
	FAILED_TO_READ_BODY = 7,
	ERROR_BODY_READING = 8,
	CONTENT_LENGTH_NOT_FOUND = 9,
	TYPE_NOT_FOUND = 10,
	INCORRECT_RESPONSE_HEADER = 11,
	ERROR_OCCURED = 12,
	NO_CONTENT = 13,
	SERIALIZATION_FAILED = 14,
	SOCKET_NOT_CONNECTED = 15,
	CONNECTION_ISSUE = 16
};

class Communication{
	public:
		Communication();
		Communication(int64_t fd);
		Communication(std::string, uint64_t);
		virtual ~Communication();

		virtual void read_socket(char *, handler, uint64_t) = 0;
		virtual void write_socket(const char *, handler, uint64_t) = 0;
		virtual boost::shared_ptr<boost::asio::io_service> get_io_service() = 0;
		int64_t get_fd();
		void stop_operations();
		bool is_connected();

	protected:
		boost::shared_ptr<boost::asio::ip::tcp::socket> socket;
		uint64_t port;
		std::string hostname;
		//void prepare(boost::shared_ptr<boost::asio::io_service>);
		void prepare();
		//void prepare(int64_t, boost::shared_ptr<boost::asio::io_service>);
		void prepare(int64_t);
		CommunicationHelper communication_helper;

	private:
		void timeout_handler(
			boost::shared_ptr<boost::asio::ip::tcp::socket>,
			const boost::system::error_code &
		);
		void connect_handler(
			boost::shared_ptr<boost::asio::deadline_timer>,
			const boost::system::error_code &
		);

		int64_t fd;
		bool is_alive;
		bool is_timed_out;
		boost::mutex mutex;
		boost::condition_variable condition;
};

class SynchronousCommunication: public Communication
{
	public:
		SynchronousCommunication(int64_t);
		SynchronousCommunication(std::string, uint64_t);

		void read_socket(char *, handler, uint64_t);
		void write_socket(const char *, handler, uint64_t);
		boost::shared_ptr<boost::asio::io_service> get_io_service();
		uint64_t read_socket_without_handler(char *, uint64_t, boost::system::error_code &);
		uint64_t read_socket_until(std::string &, const std::string &, boost::system::error_code &);

//	private:
//		CommunicationHelper communication_helper;
};

class AsynchronousCommunication: public Communication
{
	public:
		AsynchronousCommunication(int64_t);
		AsynchronousCommunication(std::string, uint64_t);

		void read_socket(char *, handler, uint64_t);
		void write_socket(const char *, handler, uint64_t);
		boost::shared_ptr<boost::asio::io_service> get_io_service();

//	private:
		//CommunicationHelper communication_helper;

};

}

#endif
