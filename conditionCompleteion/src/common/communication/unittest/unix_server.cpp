#include <cerrno>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>



int get_unix_socket(){
	struct sockaddr_un addr, cli_addr;
	int fd, newsockfd;
	socklen_t clilen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "./hidden", sizeof(addr.sun_path)-1);
	unlink("./hidden");
	bind(fd, (struct sockaddr*)&addr, sizeof(addr));

	listen(fd, 5);
	clilen = sizeof(cli_addr);

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
	
}
