#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "communication/communication.h"


int main(){
	FILE *fp;
	int s, len;
	struct sockaddr_un saun;
	
	//Open socket
	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
			//OSDLOG(FATAL, "Unable to create socket");
//			PASSERT(false, "Socket creation failed for health monitoring!");
		std::cout << "failed to open socket" << std::endl;
	}

	saun.sun_family = AF_UNIX;
	strcpy(saun.sun_path, "/root/hidden");

		//Connect to server
	len = sizeof(saun.sun_family) + strlen(saun.sun_path);
	if (connect(s, (struct sockaddr *)&saun, len) < 0) {
			//OSDLOG(FATAL, "Connection to server failed");
//                        PASSERT(false, "Connection to server failed for health monitoring!");
		std::cout << "failed to connect to the server" << std::endl;
	}
	fp = fdopen(s, "r");

		//Populate fd in object
	uint64_t fd = fileno(fp);

	Communication::SynchronousCommunication * com = new Communication::SynchronousCommunication(fd);
	delete com;
	std::cout << "deleted \n";
	//Communication::SynchronousCommunication * com1 = new Communication::SynchronousCommunication(fd);

}
