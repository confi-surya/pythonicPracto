#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "osm/osmServer/msg_handler.h"

class Listener
{
	public:
		Listener(
			boost::shared_ptr<common_parser::MessageParser> parser_obj,
			cool::SharedPtr<config_file::OSMConfigFileParser> config_parser_ptr
		);
		void create_sockets();
		void accept();
		void close_all_sockets();
		~Listener();
	private:
		boost::shared_ptr<common_parser::MessageParser> parser_obj;
		std::string domain_port;
		int32_t tcp_port;
		fd_set socket_set;
		int fifo_fd;
		std::string fifo_name;
		boost::shared_ptr<boost::asio::io_service::work> work;
		boost::asio::io_service io_service_obj;
		boost::thread_group thread_pool;
		std::vector<int> sockets;
		void accept_on_socket(std::vector<int> active_fds);
		std::vector<int> select_on_sockets();
		void create_domain_socket();
		void create_tcp_socket();
		void read(uint64_t fd);
};
