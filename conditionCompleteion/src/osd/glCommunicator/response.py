
class Resp:
    """
    @desc Response class for Result. status code should be mapped with this class.
    """
    SUCCESS = 0

    #Network Related Error
    BROKEN_PIPE = -1
    CONNECTION_RESET_BY_PEER = -2
    NETWORK_TIMEOUT = -3
    INVALID_SOCKET = -4
    IO_ERROR = -5
    SOCKET_READ_ERROR = -6

    #Data related
    NONE_BYTES = -10
    CORRUPTED_HEADER = -11
    CORRUPTED_MESSAGE = -12
    CORRUPTED_DATA = -13
    INVALID_KEY = -14

    #Server side Exception
    SERVER_ERROR = -20

    #Generic Exception
    TIMEOUT = -25


    


class Status:
    """Status class for the result."""
    def __init__(self, code, msg=None):
        self.__code = code
        self.__msg = msg
    def __repr__(self):
        return "(status code: %s, status message: %s)" \
        % (self.__code, self.__msg)

    def __str__(self):
        return "(status code: %s, status message: %s)" \
        % (self.__code, self.__msg)


    def get_status_code(self):
        """@desc status code getter method"""
        return self.__code

    def get_status_message(self):
        """desc status message getter method"""
        return self.__msg

class Result:
    """Result class"""
    def __init__(self, status, message=None):
        self.status = status
        self.message = message

