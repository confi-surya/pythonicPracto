"""
Fake local leader
"""
import os
import socket
from time import sleep
from threading import Thread
import thread

class fake_ll(Thread):

    def __init__(self):
        self.__connection = None
	self.__flag = False
	Thread.__init__(self)

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Bind the socket to the port
        sock.bind(("127.0.0.1", 11))

        # Listen for incoming connections
        sock.listen(1)

	self.__flag = True
        while self.__flag:
            # Wait for a connection
            print 'waiting for a connection'
            self.__connection, client_address = sock.accept()
            try:
                print 'connection established from %s' % client_address[0]
                while True:
                    data = self.__connection.recv(1024)
                    print 'received "%s"' % data
                    if data:
                        self.__connection.send(data)
                        print 'Send the data :)'
                    else:
                        print 'no more data from %s' % client_address[0]
                        break
                sleep(5)
            except Exception as err:
                print err
            
    def stop(self):
        print "Fake ll stop called!"
	self.__flag = False
        self.__connection.close()
	os.remove(server_address)
