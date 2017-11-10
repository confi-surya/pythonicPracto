#include <iostream>
#include "communication/communication.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace Communication{

Communication::Communication(): is_alive(true)
{
}

Communication::Communication(int64_t fd
):
	fd(fd),
	is_alive(true),
	communication_helper(CommunicationHelper())
{
}

Communication::Communication(
	std::string hostname,
	uint64_t port
):
	port(port),
	hostname(hostname),
	is_alive(true),
	communication_helper(CommunicationHelper())
{
}

int64_t Communication::get_fd()
{
	return this->fd;
}

void Communication::prepare()
{
	/*
	this->socket.reset(new boost::asio::ip::tcp::socket(*io_service));
	if (!this->socket){
		COMMLOGGER(ERROR, "Error while preparing socket");
		return;
	}
	boost::system::error_code ec_address;
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(this->hostname, ec_address), this->port);
	if (ec_address){
		COMMLOGGER(ERROR, "Error while creating address string : " << ec_address.message());
		this->socket.reset();
		return;
	}
	boost::system::error_code ec;
	socket->connect(endpoint, ec);
	if(ec)
	{
		COMMLOGGER(ERROR, "Error occurred while connecting to socket : " << ec.message());
		this->socket.reset();
		return;
	}*/
	this->is_alive = true;
}

void Communication::prepare(int64_t fd)
{
	/*
	this->socket.reset(new boost::asio::ip::tcp::socket(*io_service));
	if (!this->socket){
		COMMLOGGER(ERROR, "Error occured while preparing socket");
		return;
	}
	boost::system::error_code ec;
	this->socket->assign(boost::asio::ip::tcp::v4(), fd, ec);
	if (ec){
		COMMLOGGER(ERROR, "Error while converting native socket to boost socket : " << ec.message());
		this->socket.reset();
		return;
	}
	*/
	this->is_alive = true;
}

bool Communication::is_connected()
{
	return this->is_alive;
}

void Communication::stop_operations()
{
	try
	{
		this->socket->cancel();
		this->socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		this->socket->close();
	}
	catch(boost::system::system_error)
	{
		OSDLOG(ERROR, "No sockets to close");
	}
}

Communication::~Communication()
{
	/*
	if (this->is_alive){
		this->stop_operations();
	}
	*/
}

boost::shared_ptr<boost::asio::io_service> SynchronousCommunication::get_io_service()
{
	return this->communication_helper.get_async_io_service();
}

SynchronousCommunication::SynchronousCommunication(int64_t fd):
	Communication(fd)
{
	this->prepare(fd);
}

SynchronousCommunication::SynchronousCommunication(
	std::string hostname,
	uint64_t port
):
	Communication(hostname, port)
//	Communication(hostname, port, this->communication_helper.get_sync_io_service())
{
	this->prepare();
}

void SynchronousCommunication::read_socket(char * buffer, handler completion_handler, uint64_t size)
{
	/*
	uint64_t bytes_transfered = 0;
	boost::system::error_code ec;
	try{
		bytes_transfered = boost::asio::read(*(this->socket), boost::asio::buffer(buffer, size));
		COMMLOGGER(DEBUG, "Bytes read: " << bytes_transfered);
	}
	catch (boost::system::system_error & er){
		completion_handler(er.code(), bytes_transfered);
		return;
	}
	completion_handler(ec, bytes_transfered);
	*/
}

uint64_t SynchronousCommunication::read_socket_without_handler(char * buffer, uint64_t size, boost::system::error_code & ec)
{
	uint64_t bytes_transfered = 0;
	/*
	try{
		bytes_transfered = boost::asio::read(*(this->socket), boost::asio::buffer(buffer, size));
	}
	catch (boost::system::system_error & er){
		ec.assign(er.code().value(), er.code().category());
	}
	*/
	return bytes_transfered;
}

uint64_t SynchronousCommunication::read_socket_until(std::string & stream, const std::string & delim, boost::system::error_code & ec)
{
	boost::asio::streambuf buffer(512);
	uint64_t bytes_transfered = 0;

	/*
	bytes_transfered = boost::asio::read_until(*(this->socket), buffer, delim, ec);

	if (ec.value() > 0){
		return bytes_transfered;
	}

	uint32_t total_bytes_read = buffer.size();

	boost::asio::streambuf::const_buffers_type bufs = buffer.data();
	stream.append(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + total_bytes_read);

	*/
	return bytes_transfered;
}

void SynchronousCommunication::write_socket(const char * buffer, handler completion_handler, uint64_t size)
{
	/*
	uint64_t bytes_transfered = 0;
	try{
		bytes_transfered = boost::asio::write(*(this->socket), boost::asio::buffer(buffer, size));
	}
	catch (boost::system::system_error & er){
		completion_handler(er.code(), bytes_transfered);
		return;
	}
	boost::system::error_code ec;
	completion_handler(ec, bytes_transfered);
	*/
}

AsynchronousCommunication::AsynchronousCommunication(int64_t fd):
	Communication(fd)
//	Communication(fd, this->communication_helper.get_async_io_service())
{
	this->prepare(fd);
}

AsynchronousCommunication::AsynchronousCommunication(
	std::string hostname,
	uint64_t port
):
	Communication(hostname, port)
//	Communication(hostname, port, this->communication_helper.get_async_io_service())
{
	this->prepare();
}

boost::shared_ptr<boost::asio::io_service> AsynchronousCommunication::get_io_service()
{
	return this->communication_helper.get_async_io_service();
}

void AsynchronousCommunication::read_socket(char * buffer, handler completion_handler, uint64_t size)
{
	//boost::asio::async_read(*(this->socket), boost::asio::buffer(buffer, size), completion_handler);
}

void AsynchronousCommunication::write_socket(const char * buffer, handler completion_handler, uint64_t size)
{
	//boost::asio::async_write(*(this->socket), boost::asio::buffer(buffer, size), completion_handler);
}

}
