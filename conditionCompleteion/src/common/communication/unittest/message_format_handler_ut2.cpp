#include <iostream>
#include <cerrno>
#include <boost/asio.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "communication/message_type_enum.pb.h"
#include "communication/message_format_handler.h"
#include "communication/stubs.h"
#include "communication/message_result_wrapper.h"
#include "communication/message_interface_handlers.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <list>

void success(boost::shared_ptr<MessageResultWrap> result){
	std::cout << "success" << std::endl;
}

void failure(){
	std::cout << "failed" << std::endl;
}

void connect_han(const boost::system::error_code& error){
	std::cout << "connection complete" << std::endl;
}

int get_tcp_fd(){
	struct sockaddr_in serv_addr, cli_addr;
	int newsockfd, sockfd;
	socklen_t clilen;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		std::cout << "error while conencting socket";
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	int portno = 123;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		std::cout << "error when binding socket";
		return -1;
	}

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0){
		std::cout << "error while accepting connection";
		return -1;
	}
	else{
		std::cout << "got connection";
	}
	return newsockfd;
}

//void perform()
int get_unix_socket(){
	struct sockaddr_un addr, cli_addr;
	int fd, newsockfd;
	socklen_t clilen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "/root/hidden", sizeof(addr.sun_path)-1);
	unlink("/root/hidden");
	bind(fd, (struct sockaddr*)&addr, sizeof(addr));

	listen(fd, 10);
	clilen = sizeof(cli_addr);
	int i;
	std::cin >> i;

        newsockfd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0){
                std::cout << "error while accepting connection" << std::endl;
		std::cout << " error : " << errno << std::endl;
		std::cout << " error : " << strerror(errno) << std::endl;
                return -1;
        }
        else{
                std::cout << "got connection";
		return newsockfd;
        }
}

int main(){
//	comm_messages::Message * hb = new comm_messages::GetGlobalMap("HN0101_1234_container-server");
	boost::shared_ptr<MessageResultWrap> hb;
	ProtocolStructure::MessageFormatHandler msg_handler;
	int fd = get_unix_socket();
	int i ;
	std::cin >> i;
//	Communication::SynchronousCommunication * com = new Communication::SynchronousCommunication("192.168.123.13", 123);
	Communication::SynchronousCommunication * com = new Communication::SynchronousCommunication(fd);
//	Communication::AsynchronousCommunication * com = new Communication::AsynchronousCommunication(fd);
	msg_handler.get_message_object(com, success, failure);
//	msg_handler.send_message(com, hb, success, failure);
//	delete com;
//	}

	hb = msg_handler.get_msg();
	std::cout << "type : " << hb->get_type() << " \n ";
	ProtocolStructure::MessageFormatHandler m;
	boost::shared_ptr<ErrorStatus> err(new ErrorStatus(0, ""));
	comm_messages::Message * ack = new comm_messages::OsdStartMonitoringAck("HN0101", err);
	m.send_message(com, ack, success, failure);
	std::cout << "ack message sent to library \n";
	delete ack;

	hb.reset();
	while(true){
		ProtocolStructure::MessageFormatHandler n;
		std::cout << "waiting for heartbeat\n";
		n.get_message_object(com, success, failure);
		hb = n.get_msg();
		std::cout << "\n got heartbeat type : " << hb->get_type() << std::endl;
	}
//	comm_messages::GetServiceComponent * jn = dynamic_cast<comm_messages::GetServiceComponent *>(hb);
/*	if (hb->get_type() == 9){
		ProtocolStructure::MessageFormatHandler m;
		comm_messages::Message * ll = new comm_messages::GlobalMapInfo(container_pair, account_pair, updater_pair, object_pair);
		m.send_message(com, ll, success, failure);
		delete ll;
	}
	else{
		std::cout << "ajdioasjidoad";
	}*/
	delete com;
	hb.reset();
//	delete jn;
	
//	boost::system::error_code ec;
//	acceptor.accept(sock);
//	sock.async_connect(endpoint, connect_han);

//	boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
//	get_heart_beat_msg_test();
}
