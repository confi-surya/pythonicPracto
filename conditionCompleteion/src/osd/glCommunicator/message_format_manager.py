import errno
import socket
import hashlib
from osd.glCommunicator.response import Resp, Status
from osd.glCommunicator.messages import HeaderMessage, ErrMessage
from osd.glCommunicator.message_type_enum_pb2 import *
from osd.glCommunicator.communication.osd_exception import \
    InvalidSocketException, SocketSentException, SerializationException, \
    SocketReadException, InvalidArgumentException

BANNER_SIZE = 13


class MessageFormatManager:
    """
       Message handler class. serialize/de-serialize messages \
       and send/recv over communication socket
    """
    def __init__(self, logger):
        self.__logger = logger


    def get_message_object(self, conn, message_obj, timeout=300):
        """
        @desc Read data form communication object, de-serialize to data in message and respond to user.
        @param conn: A communication object to read data.
        @param message: An empty message object, which will be populated with de-serialized data.
        @param timeout: timeout value in seconds.
        @return An object of Status class which contains error code and error string. 
        @see Status
        """
        #Convert timeouts in milliseconds.
        #This timeout only work for socket_io. 
        timeout = timeout * 1000
        if conn.recv_wait(timeout):
            try:
                self.__logger.info("Going to read for message: %s" \
                % message_obj.__class__.__name__)
                self.__logger.debug("Read banner: %s bytes" %BANNER_SIZE)
                data_string = conn.read(BANNER_SIZE)
            except (InvalidSocketException, socket.error), ex:
                self.__logger.error("Socket error while reading banner: %s" %ex)
                return Status(Resp.INVALID_SOCKET, "Error while Socke read")
            except SocketReadException, ex:
                return Status(Resp.SOCKET_READ_ERROR, \
                "Could not read from socket")
            except IOError, ex:
                self.__logger.error("Io error while reading banner: %s" %ex)
                if ex.errno == errno.EPIPE:
                    status_code = Resp.BROKEN_PIPE
                elif ex.errno == errno.ECONNRESET:
                    status_code = Resp.CONNECTION_RESET_BY_PEER
                else:
                    status_code = Resp.IO_ERROR
                return Status(status_code, ex.message)
            try:
                msg_hdr = HeaderMessage()
                msg_hdr.de_serialize(data_string)
            except Exception, ex:
                self.__logger.error("Error while de_serialize header: %s" %ex)
                return Status(Resp.CORRUPTED_HEADER, ex.message)
            try:
                self.__logger.debug("Reading message: %s bytes" 
                %msg_hdr.message_size())
                data_string = conn.read(msg_hdr.message_size())
                self.__logger.debug("md5sum of %s read bytes: %s" \
                %(len(data_string), hashlib.md5(data_string).hexdigest()))
            except SocketReadException, ex:
                self.__logger.error("Socket Error while reading actual message")
                return Status(Resp.SOCKET_READ_ERROR, \
                "Could not read from socket")
            except (InvalidSocketException, socket.error), ex:
                self.__logger.error("Error while reading message: %s" %ex)
                return Status(Resp.INVALID_SOCKET, "Error while Socke read")
            except IOError, ex:
                self.__logger.error("Io error while reading message: %s" %ex)
                if ex.errno == errno.EPIPE:
                    status_code = Resp.BROKEN_PIPE
                elif ex.errno == errno.ECONNRESET:
                    status_code = Resp.CONNECTION_RESET_BY_PEER
                else:
                    status_code = Resp.IO_ERROR
                return Status(status_code, ex.message)
            if not data_string:
                return Status(Resp.NONE_BYTES, "Nothing to de-serialize")
            if msg_hdr.message_type() == typeEnums.ERROR_MSG:
                self.__logger.error("Server responded error message")
                message_obj = ErrMessage()
                try:
                    message_obj.de_serialize(data_string)
                    self.__logger.error(\
                    "Server error message: error_code: %s, message: %s" \
                    %(message_obj.error_code(), message_obj.message()))
                    return Status(Resp.SERVER_ERROR, message_obj.message())
                except Exception, ex:
                    self.__logger.error("Error while de_serialize message: %s" \
                    %ex)
                    return Status(Resp.CORRUPTED_MESSAGE, ex.message)
            else:
                try:
                    message_obj.de_serialize(data_string)
                    return Status(Resp.SUCCESS, "")
                except Exception, ex:
                    self.__logger.error("Error while de_serialize message: %s" \
                    %ex)
                    return Status(Resp.CORRUPTED_MESSAGE, ex)
        else:
            self.__logger.error("Timeout occured while receving message")
            return Status(Resp.NETWORK_TIMEOUT, \
            "Timeout occured while receving message")

    def send_message_object(self, conn, message_type, message_object):
        """
        @desc Method to serialize a message object into binary string and send it over communication object.
        @param conn: A communication object to send data.
        @param message_type: message enum.
        @param message_object: A message object which needs to be sent over communication object.
        @return An object of Status class which contains error code and error string
        @see
        """
        self.__logger.debug("serialize message object for type: %s" \
        % message_type)
        try:
            message_string = message_object.serialize()
        except Exception, ex:
            self.__logger.error("Message type: %s, Failed to serialize: %s" \
            % (message_type, ex))
            raise SerializationException(ex.message)
        self.__logger.debug("Size of serialize actual message: %s, md5sum: %s" \
        %(len(message_string), hashlib.md5(message_string).hexdigest()))
        self.__logger.debug("prepare and serialize banner")
        msg_hdr = HeaderMessage(payload_type=message_type, size=len(message_string))
        try:
            msg_hdr_serialized = msg_hdr.serialize()
        except InvalidArgumentException, ex:
            self.__logger.error("Header message failed to serialized: %s" %ex)
            raise SerializationException("Invalid Argument")
        data_string = msg_hdr_serialized + message_string
        self.__logger.info("Sending message with header: %s, size: %s" \
        % (msg_hdr, len(data_string)))
        try:
            conn.write(data_string)
            conn.send_wait()
            self.__logger.debug("Message sent Successfully")
            return Status(Resp.SUCCESS, "Message sent Successfully")
        except (InvalidSocketException, SocketSentException, socket.error), ex:
            self.__logger.error("Socket Error while writing: %s" %ex)
            return Status(Resp.INVALID_SOCKET, ex.message)
        except IOError, ex:
            self.__logger.error("IO Error while writing: %s" %ex)
            if ex.errno == errno.EPIPE:
                status_code = Resp.BROKEN_PIPE
            elif ex.errno == errno.ECONNRESET:
                status_code = Resp.CONNECTION_RESET_BY_PEER
            else:
                status_code = Resp.IO_ERROR
            return Status(status_code, ex.message)

