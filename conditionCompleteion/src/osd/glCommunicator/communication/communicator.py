from osd.glCommunicator.communication.event_io import EventIo
from osd.glCommunicator.communication.socket_io import SocketIo
from osd.glCommunicator.communication.async_io import AsyncIo
from osd.glCommunicator.config import IoType
from osd.glCommunicator.communication.osd_exception import InvalidTypeException

class Communicator:
    """Base communicator class"""
    def __init__(self):
        pass

    def dispatcher(self, conn, message):
        pass

    def receiver(self, conn):
        pass

class ConnectionFactory:
    """Connection Factory class"""
    def __init__(self):
        pass

    def get_connection(self, conn_type, host_info, retry, timeout, logger):
        """Factroy method for connection"""
        if conn_type == IoType.EVENTIO:
            conn = EventIo(host_info, retry, timeout, logger)
        elif conn_type == IoType.SOCKETIO:
            conn = SocketIo(host_info, retry, timeout, logger)
        else:
            logger.error("Invalid conn_type: %s" % conn_type)
            raise InvalidTypeException("%s is not a valid Io class type" \
            % conn_type)
        if not conn.get_sock():
            return None
        return conn




