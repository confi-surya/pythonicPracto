#define GTEST_HAS_TR1_TUPLE 0
#include "gtest/gtest.h"
#include <iostream>
#include <cerrno>
#include <boost/asio.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "communication/message_type_enum.pb.h"
#include "communication/message_format_handler.h"
#include "communication/stubs.h"
#include "communication/message_result_wrapper.h"
#include "communication/message_interface_handlers.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <list>


class Chibaku
{
	public:
		Chibaku(){ this->status = false;};
		~Chibaku(){};

		void success(boost::shared_ptr<MessageResultWrap>);
		void failure();
		bool get_status();

	private:
		bool status;
};

void Chibaku::success(boost::shared_ptr<MessageResultWrap> result){
	this->status = true;
}

void Chibaku::failure(){
	this->status = false;
}

bool Chibaku::get_status(){
	return this->status;
}

void connect_han(const boost::system::error_code& error){
}

const std::string user_name = ::getenv("USER");
const std::string path = "/tmp/" + user_name;

//void perform()
//
class server
{
	public:
		server(){this->server_fd = -1;};
		~server(){};

		int get_serverfd();
		int get_clientfd();
		void get_unix_socket();
		int get_unix_client();
		void close_fd();

	private:
		int client_fd;
		int server_fd;
};

int server::get_unix_client()
{
	int sockfd;
	sockaddr_un addr;
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path.c_str());
	if(connect(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0)
	{
		this->client_fd = -1;
	}
	else{
		this->client_fd = sockfd;
		return sockfd;
	}
	return -1;
}

void server::get_unix_socket(){
	struct sockaddr_un addr, cli_addr;
	int fd, newsockfd;
	socklen_t clilen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);
	unlink(path.c_str());
	bind(fd, (struct sockaddr*)&addr, sizeof(addr));

	listen(fd, 10);
	clilen = sizeof(cli_addr);

        newsockfd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0){
                this->server_fd = -1;
        }
        else{
		this->server_fd = newsockfd;
        }
}

int server::get_serverfd(){
	return this->server_fd;
}

int server::get_clientfd(){
	return this->client_fd;
}

void server::close_fd()
{
	close(this->server_fd);
}

class HandlerHelper
{
	public:
		HandlerHelper(){this->status = false;};
		~HandlerHelper(){};

		void register_handler(boost::function<bool()>);
		bool get_status();
		void run();

	private:
		bool status;
		boost::function<bool()> handler;
};

void HandlerHelper::register_handler(boost::function<bool()> func){
	this->handler = func;
}

bool HandlerHelper::get_status(){
	return this->status;
}

void HandlerHelper::run()
{
	int val = this->handler();
	if (val == -1){
		return;
	}
	this->status = true;
}

void connect_to_server(int fd)
{
	boost::shared_ptr<Communication::SynchronousCommunication> com(new Communication::SynchronousCommunication(fd));
	ProtocolStructure::MessageFormatHandler msg_handler;
	Chibaku * handler = new Chibaku();
	comm_messages::Message * msg = new comm_messages::HeartBeat("HN0101", 132);
	int ll;
	std::cout << "\n enter\n";
	//std::cin >> ll;
	//msg_handler.send_message(com, msg, boost::bind(&Chibaku::success, handler, result1), boost::bind(&Chibaku::failure, handler));

}

TEST(SendMessageTest, sendMessageSuccessfullyTest)
{
/*	HandlerHelper hand_of_god;
	hand_of_god.register_handler(boost::bind(get_unix_socket));

	boost::thread server(boost::bind(HandlerHelper::run, hand_of_god));*/
	server * srv = new server();
	boost::thread serv(boost::bind(&server::get_unix_socket, srv));
	//srv->get_unix_socket();
//	ASSERT_NE(fd, -1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
//	int fd = srv->get_unix_client();
//	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

//	serv.join();
//	client.join();
	int fd_2 = srv->get_serverfd();

/*	boost::shared_ptr<Communication::SynchronousCommunication> com(new Communication::SynchronousCommunication(fd));
	ProtocolStructure::MessageFormatHandler msg_handler;
	Chibaku * handler = new Chibaku();
	comm_messages::Message * msg = new comm_messages::HeartBeat("HN0101", 132);
	msg_handler.send_message(com, msg, boost::bind(&Chibaku::success, handler, result1), boost::bind(&Chibaku::failure, handler));

	ASSERT_TRUE(handler->get_status());*/

//	boost::thread hel(boost::bind(connect_to_server, fd));

//	int l;
//	std::cin >> l;
	boost::shared_ptr<Communication::SynchronousCommunication> serv_com(new Communication::SynchronousCommunication(fd_2));
	boost::shared_ptr<MessageResultWrap> result1(new MessageResultWrap(messageTypeEnum::typeEnums::HEART_BEAT));
	ProtocolStructure::MessageFormatHandler msg_handler_2;
	Chibaku * handler_2 = new Chibaku();
	msg_handler_2.get_message_object(
						serv_com,
						boost::bind(&Chibaku::success, handler_2, result1),
						boost::bind(&Chibaku::failure, handler_2)
					);
	//std::cout << "\n abc : " << result1->get_error_code();
	boost::shared_ptr<MessageResultWrap> result2 = msg_handler_2.get_msg();
	std::cout << "aaa : " << result2->get_error_code();
	ASSERT_TRUE(handler_2->get_status());
}
