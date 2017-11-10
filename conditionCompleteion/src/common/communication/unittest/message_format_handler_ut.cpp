#include <iostream>
#include <boost/asio.hpp>
#include "communication/message_type_enum.pb.h"
#include "communication/message_format_handler.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

void success(){
	std::cout << "success" << std::endl;
}

void failure(){
	std::cout << "failed" << std::endl;
}

void connect_han(const boost::system::error_code& error){
	std::cout << "connection complete" << std::endl;
}

void get_heart_beat_msg_test(){
	messages::HeartBeat * hb = new messages::HeartBeat("HN0101");
	messages::Message * msg = NULL;
	ProtocolStructure::MessageFormatHandler msg_handler;

	Communication::SynchronousCommunication * com = new Communication::SynchronousCommunication("192.168.123.13", 123);
	msg_handler.get_message_object(com, msg, success, failure);
}

void perform()

int main(){
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 123);
	boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint);
//	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 123);
//	boost::asio::ip::tcp::acceptor acceptor(io_service, ep);
	boost::asio::ip::tcp::socket sock(io_service);

	acceptor.accept(sock);

	messages::Message * hb = new messages::HeartBeat("HN0101");
	
	
	boost::system::error_code ec;
//	acceptor.accept(sock);
//	sock.async_connect(endpoint, connect_han);

//	boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
//	get_heart_beat_msg_test();
}
