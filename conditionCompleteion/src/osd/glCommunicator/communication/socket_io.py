import socket
import select

from osd.glCommunicator.config import IoType
from osd.glCommunicator.communication.base_io import BaseIo
from osd.glCommunicator.communication.osd_exception import \
InvalidSocketException, SocketSentException, SocketReadException

MAX_READ_SIZE = 65535

class SocketIo(BaseIo):
    """
    Class for async socket based connection.
    Data members are:
    1: host_info: An object of ServiceInfo.
    2: retry_connection: Connection retry count.
    3: timeout: timeouts
    4: logger: A logger object for logging
    """

    def __init__(self, host_info, retry_connection, timeout, logger):
        BaseIo.__init__(self, host_info, retry_connection, timeout, logger)
        self.__host_info = host_info
        self.__timeout = timeout
        self.__connection_retries = retry_connection
        self.__logger = logger
        self.__sock = None
        self.__data_bytes = ""
        self.__is_connected = False
        self.sock_type = IoType.SOCKETIO
        self.open()

    def __del__(self):
        if self.__sock:
            self.__sock.close()

    def __create_connection(self):
        """@desc Create a socket connection."""
        msg = "getaddrinfo returns an empty list"
        for res in socket.getaddrinfo(self.__host_info.get_ip(), \
                self.__host_info.get_port(), 0, socket.SOCK_STREAM):
            af, socktype, proto, _, sa = res
            try:
                self.__logger.debug("Createing socket")
                self.__sock = socket.socket(af, socktype, proto)
                self.__sock.connect(sa)
                self.__logger.info("Connected with: %s: %s" \
                % (self.__host_info.get_ip(), self.__host_info.get_port()))
                break
            except socket.error, msg:
                self.__logger.error("Error while connecting to %s:%s :  %s" \
                % (self.__host_info.get_ip(), self.__host_info.get_port(), msg))
                if self.__sock is not None:
                    self.__sock.close()
                    self.__sock = None
                raise socket.error, msg

    def open(self):
        """@desc Open socket connection."""
        for retry in range(1, self.__connection_retries + 1):
            self.__logger.debug("Open connection, retry count: %s" % retry)
            try:
                self.__create_connection()
                break
            except socket.error, ex:
                self.__logger.error("Error while creating connection: %s" %ex)
                self.__logger.info("connection retry count: %s" % retry)
                self.__sock = None

    def get_sock(self):
        """@desc Getter method of socket"""
        return self.__sock

    def re_open(self):
        """@desc Close and then open a new connection"""
        self.__logger.debug("Re create connection")
        self.close()
        self.open()

    def close(self):
        """@desc Close connection"""
        self.__logger.debug("Closing socket connection")
        try:
            self.__sock.shutdown(socket.SHUT_RDWR)
            self.__sock.close()
        except Exception, ex:
            self.__logger.error("Error while closing socket: %s" %ex)
        finally:
            self.__sock = None

    def write(self, data):
        """
        @desc Write data on socket.
        @param data: Data string which needs to be written.
        """
        if not self.__sock:
            self.__logger.error("Invalid socket in write")
            raise InvalidSocketException("Socket is not created")
        self.__logger.debug("Sending data on socket...")
        sent = total_sent = 0
        while total_sent < len(data):
            try:
                sent = self.__sock.send(data[total_sent:])
                self.__logger.debug("Sent chunk size: %s" % sent)
            except socket.timeout, ex:
                self.__logger.error("Socket timeout while writing : %s" % ex)
            except socket.error, ex:
                self.__logger.error("While writing on Socket: %s" %ex)
            if sent == 0:
                raise SocketSentException("Could not send data")
            total_sent = total_sent + sent
        self.__logger.info("Total sent bytes: %s" % total_sent)

    def read(self, size):
        """
        @desc Read data from socket.
        @param size: number of bytes going to read.
        @return Read data string
        """
        if not self.__sock:
            self.__logger.error("Invalid socket in read")
            raise InvalidSocketException("Socket is not created")
        self.__logger.debug("reading on socket with buffer size: %s" % size)
        bytes_read = len(self.__data_bytes)
        while(bytes_read < size):
            try:
                chunk = self.__sock.recv(MAX_READ_SIZE)
            except socket.error, ex:
                self.__logger.error("While reading: %s" %ex)
                raise SocketReadException("Socket read failed")
            self.__logger.debug("Read chunk size: %s" % len(chunk))
            if chunk == '':
                self.__logger.error("error while reading from socket")
                raise SocketReadException("Socket read failed")
            self.__data_bytes = self.__data_bytes + chunk
            bytes_read = bytes_read + len(chunk)
        self.__logger.info("Total read data byte size: %s" \
        % len(self.__data_bytes))
        read_bytes = self.__data_bytes[:size]
        self.__data_bytes = self.__data_bytes[size:]
        return read_bytes


    def send_wait(self):
        """@desc Wait for data"""
        return True

    def recv_wait(self, timeout=60):
        """@desc Wait for data"""
        self.__logger.debug("Waiting for data to read on socket")
        if not self.__sock:
            self.__logger.error("Invalid socket in recv_wait")
            raise InvalidSocketException("Socket is not created")
        events = []
        try:
            READ_ONLY = select.POLLIN | select.POLLPRI | select.POLLHUP | \
            select.POLLERR
            poller = select.poll()
            poller.register(self.__sock, READ_ONLY)
            fd_to_socket = {self.__sock.fileno(): self.__sock}
            events = poller.poll(timeout)
        except Exception, ex:
            self.__logger.error("While waiting for data: %s" % ex)
        if not events:
            self.__logger.error(\
            "Timeout expired: No data available for reading")
            return False
        for fd, flag in events:
            s = fd_to_socket[fd]
            if flag & (select.POLLIN | select.POLLPRI):
                if s is self.__sock:
                    return True
                else:
                    self.__logger.error(\
                    "Timeout expired: No data available for reading")
                    return False

