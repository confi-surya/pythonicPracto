import abc

class BaseIo(object):
    __metaclass__ = abc.ABCMeta
    
    @abc.abstractmethod
    def __init__(self, host_info, retry_connection, timeout, logger):
        return

    @abc.abstractmethod
    def open(self):
        """Open a conection."""
        return

    @abc.abstractmethod
    def re_open(self):
        """Re-open a conection."""
        self.close()
        self.open()

    @abc.abstractmethod
    def close(self):
        """Close connection"""
        return

    @abc.abstractmethod
    def write(self, data):
        """Write data on connection"""
        return

    @abc.abstractmethod
    def read(self, size):
        """Read data from connection"""
        return

    @abc.abstractmethod
    def send_wait(self):
        """Wait for data"""
        return

    @abc.abstractmethod
    def recv_wait(self, timeout):
        """Wait for data"""
        return

