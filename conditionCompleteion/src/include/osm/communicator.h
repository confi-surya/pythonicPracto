#ifndef __communicator_h__
#define __communicator_h__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

class Communicator
{

public:
	Communicator();
	virtual ~Communicator();	
	bool send_notification_to_gl(int fd);
			
};
#endif
