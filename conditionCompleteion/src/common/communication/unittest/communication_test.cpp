#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include "communication/communication.h"
#include "boost/shared_ptr.hpp"

namespace communication_test
{
/*
void connect_handler(boost::shared_ptr<boost::asio::deadline_timer> t, const boost::system::error_code & code)
{
        if (!code){
        //        t->cancel();
                std::cout << "\nconnection success: "<< code.value() << std::endl;
        }
        else{
                std::cout << "\n connection failed : " << code.message() << " \n";
        }
}

void timeout_handler(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code & code){
        if (!code){
		//if (sock->is_open()){
		//sock->cancel();
                //sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                sock->close();
		//}
                //sock->close();
                std::cout <<"1 Timeout hit :" << code.message() << std::endl;
        }
        else{
                std::cout << "2 Timeout hit : " << code.message();
        }
}*/

void run(boost::shared_ptr<boost::asio::io_service>io)
{
	io->run();
}

TEST(CommunicationTest, default_constructor_msg_string_check)
{
        boost::shared_ptr<boost::asio::io_service> io(new boost::asio::io_service);
	boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(*io.get()));
	boost::thread th(boost::bind(run, io));
//        boost::shared_ptr<boost::asio::deadline_timer> t(new boost::asio::deadline_timer(*io, boost::posix_time::seconds(1)));
        boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(*io));
//        t->async_wait(boost::bind(timeout_handler, sock, _1));
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("192.168.123.13"), 1234);
//        sock->async_connect(endpoint, boost::bind(connect_handler, t, _1));
	boost::system::error_code ec;
	sock->connect(endpoint, ec);
	if (!ec){
		std::cout <<"1 socket : " << ec.value() << "." << ec.message() << std::endl;
		//t->cancel();
	}
	else{
		std::cout << "2 socket :" << ec.message() << std::endl;
		//t->cancel();
	}
	char * buffer = new char[4];
	try{
		int bytes_transfered = 0;
		bytes_transfered = boost::asio::read(*sock, boost::asio::buffer(buffer, 4));
	}
	catch(boost::system::system_error & er)
	{
		std::cout << "error : " << er.code().message();
	}
	//io->stop();
	//int i;
	//std::cin >> i;
	//sock.reset();
	//io->run();
}

}
