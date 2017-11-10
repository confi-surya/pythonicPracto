#include <fcntl.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <sys/socket.h>
#include <boost/thread.hpp>
#include <cstdio>
#include <boost/asio/io_service.hpp>
#include <boost/foreach.hpp>
#include <errno.h>
#include <stdlib.h>

#include "communication/message_interface_handlers.h"
#include "osm/osmServer/listener.h"
#include "communication/msg_external_interface.h"

Listener::Listener(
	boost::shared_ptr<common_parser::MessageParser> parser_obj,
	cool::SharedPtr<config_file::OSMConfigFileParser> config_parser_ptr
)
{
	//cool::SharedPtr<config_file::ConfigFileParser> parser_ptr = get_config_parser();
	this->domain_port = config_parser_ptr->unix_domain_portValue();
	this->tcp_port = config_parser_ptr->osm_portValue();
	this->parser_obj = parser_obj;
	//this->domain_port = "/remote/hydra042/gmishra/socket_donot_touch";
	//this->tcp_port = 61014;
	this->fifo_name = "/tmp/listener_fifo";
	if (boost::filesystem::exists(this->fifo_name))
	{
		OSDLOG(INFO, "fifo file exists, removing it");
		boost::filesystem::remove(this->fifo_name);
		
	}

	int error = mkfifo(fifo_name.c_str(), 0777);
	int err = errno;
	if (error == -1)
	{
		OSDLOG(INFO, "Could not create fifo, errno: " << err);
	}
	else
	{
		this->fifo_fd = open(fifo_name.c_str(), O_RDWR);
		this->sockets.push_back(this->fifo_fd);
	}
	this->work.reset(new boost::asio::io_service::work(this->io_service_obj));
	int thread_count = config_parser_ptr->listener_thread_countValue();
	//int thread_count = 10;
	for (int i = 1; i <= thread_count; i++)
	{
		this->thread_pool.create_thread(boost::bind(&boost::asio::io_service::run, &this->io_service_obj));
	}

}

Listener::~Listener()
{
	close(this->fifo_fd);
	boost::filesystem::remove(this->fifo_name);
	this->work.reset();
	this->thread_pool.join_all();
	this->io_service_obj.reset();
}

void Listener::create_sockets()
{
	this->create_domain_socket();
	this->create_tcp_socket();
}

void Listener::close_all_sockets()
{
	int num = write(this->fifo_fd, "KILL", 4);
}

void Listener::create_domain_socket()
{
	int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd < 0)
	{
		//return error
		
	}
	::unlink(this->domain_port.c_str());
	struct sockaddr_un serv_addr;
	serv_addr.sun_family = AF_LOCAL;
//	strcpy(serv_addr.sun_path, "/remote/hydra042/gmishra/socket_donot_touch"); //TODO this will be configurable
	strcpy(serv_addr.sun_path, this->domain_port.c_str());
	int32_t const opt = 1;
	::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	::bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	::listen(fd, 5); //TODO this will be configurable
	this->sockets.push_back(fd);
}

void Listener::create_tcp_socket()
{
	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
	{
		//return error
	}
	//sock options
	int32_t const opt = 1;
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(this->tcp_port);
	::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (::fcntl(fd, F_SETFD, FD_CLOEXEC) == -1){
		OSDLOG(ERROR, "unable to set close exec flag on fd : " << errno);
		exit(1);
	}
	if(::bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		OSDLOG(ERROR, "Port is busy: " << errno);
		exit(1);
		OSDLOG(INFO, "Could not exit");
	}
	::listen(fd, 5);
	this->sockets.push_back(fd);
}

void Listener::accept()
{
	try
	{
		while(1)
		{
			std::vector<int> active_fds = this->select_on_sockets();
			if ( !active_fds.empty() )
			{
				this->accept_on_socket(active_fds);
			}
			boost::this_thread::interruption_point();
		}
	}
	catch(const boost::thread_interrupted&)
	{
		OSDLOG(INFO, "Listener Thread interrupted. Exiting thread.");
	}
}
		

std::vector<int> Listener::select_on_sockets()
{
//	fd_set socket_set;
	FD_ZERO(&this->socket_set);
	int maxfd = 0;
	BOOST_FOREACH(int fd, this->sockets)
	{
		FD_SET(fd, &this->socket_set);
		maxfd = std::max(maxfd, fd);
	}
	int active_fd_count = ::select(maxfd + 1, &this->socket_set, NULL, NULL, NULL);
	std::vector<int> active_fds;
	if(active_fd_count < 0)
	{
		return active_fds;
	}

	BOOST_FOREACH(int fd, this->sockets)
	{
		if(FD_ISSET(fd, &socket_set))
		{
			if(fd != this->fifo_fd)
			{
				active_fds.push_back(fd);
			}
		}
	}
	return active_fds;
}

void Listener::accept_on_socket(std::vector<int> active_fds)
{
	uint64_t new_conn_fd;
	BOOST_FOREACH(int fd, active_fds)
	{
		struct sockaddr_storage client_request;
		socklen_t req_len = sizeof(client_request);
		new_conn_fd = ::accept(fd, reinterpret_cast<struct sockaddr*>(&client_request), &req_len);
		if(new_conn_fd < 0)
			continue;
		this->io_service_obj.post(boost::bind(&Listener::read, this, new_conn_fd));
	}
}

void Listener::read(uint64_t new_fd)
{
	boost::shared_ptr<Communication::SynchronousCommunication> comm(new Communication::SynchronousCommunication(new_fd));
	if(comm->is_connected())
	{
		boost::shared_ptr<MessageResultWrap> msg_result_wrap_ptr = MessageExternalInterface::sync_read_any_message(comm);
		boost::shared_ptr<MessageCommunicationWrapper> msg_comm_obj(new MessageCommunicationWrapper(msg_result_wrap_ptr, comm));
		this->parser_obj->decode_message(msg_comm_obj);
	}
}
