import asyncore, socket
import time
import threading

from osd.glCommunicator.communication.base_io import BaseIo
from osd.glCommunicator.communication.osd_exception import InvalidSocketException

class Client(asyncore.dispatcher):
    counter = 0
    LOCK = threading.Lock()
    def __init__(self, host, port):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((host, port))
        self.out_buffer = ""
        self.in_buffer = ""
        Client.invoke()
        self.i = 1


    def __del__(self):
        print "__del__"
        with cls.LOCK:
            Cls.counter -= 1

#    @staticmethod
    @classmethod
    def invoke(cls):
        with cls.LOCK:
            if cls.counter == 0:
                try:
                    #t = threading.Thread(target=cls.runLoop)
                    t = threading.Thread(target=cls.runLoop, name="Asyncore Loop")
                    t.setDaemon(True)
                    t.start()
                except Exception as ex:
                    print ex
        cls.counter += 1

#    @staticmethod
    @classmethod
    def runLoop(cls):
        asyncore.loop()

    def handle_close(self):
        print "closing.............."
        self.close()

    def handle_read(self):
        self.in_buffer =  self.in_buffer + self.recv(4096)

    def writable(self):
        print "checking writability: ", len(self.out_buffer)
        is_writable = (len(self.out_buffer) > 0)
        return is_writable

    def handle_write(self):
        print "Sending: ", self.out_buffer
        sent = self.send(self.out_buffer)
        self.out_buffer = self.out_buffer[sent:]

    def put_data_in_buff(self, data):
        self.out_buffer = data
        return 

    def get_data_from_buff(self, size):
        if len(self.in_buffer) > size:
                data = self.in_buffer[0:size]
                self.in_buffer = self.in_buffer[size:]
                return data
        return self.in_buffer

    def is_out_buffer_empty(self):
        if len(self.out_buffer) == 0:
            return True
        return False

    def is_data_recv(self):
        if len(self.in_buffer) > 0:
            return True
        return False

class AsyncIo(BaseIo):

    def __init__(self, host_info, retry_connection, timeout, logger):
        super(AsyncIo, self).__init__(host_info, retry_connection, timeout)
        self.__host_info = host_info
        self.__timeout = timeout
        self.__connection_retries = retry_connection
        self.__logger = logger
        self.__sock = None
        self.__is_connected = False
        self.__lock = threading.Lock()
        self.open()


    def __create_connection(self):
        msg = "Failed to connect"
        try:
            self.__sock = Client(self.__host_info.get_ip(), \
                                 self.__host_info.get_port())
        except Exception, ex:
            if self.__sock is not None:
                self.__sock.handle_close()
            raise Exception(msg)


    def open(self):
        """Open a conection."""
        waitEvent = threading.Event()
        for retry in range(1, self.__connection_retries + 1):
            try:
                if not waitEvent.isSet():
                    self.__create_connection()
                    break
            except Exception, ex:
                self.__sock = None
                waitEvent.set()
                waitEvent.clear()
                waitEvent.wait(self.__timeout)

    def close(self):
        """Close connection"""
        return

    def write(self, data):
        """Write data on connection"""
        if not self.__sock:
            InvalidSocketException("Socket is not created")
        self.__sock.put_data_in_buff(data)

    def read(self, size):
        """Read data from connection"""
        if not self.__sock:
            InvalidSocketException("Socket is not created")
        self.__sock.get_data_from_buff(size)

    def re_open(self):
        self.close()
        self.open()

    def send_wait(self):
        while True:
            if self.__sock.is_out_buffer_empty():
                return 
            time.sleep(0.5)

    def recv_wait(self):
        while True:
            if self.__sock.is_data_recv():
                return
            time.sleep(0.5)

