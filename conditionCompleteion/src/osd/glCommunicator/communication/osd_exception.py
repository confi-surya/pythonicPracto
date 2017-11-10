class OsdException(Exception):
    def __init__(self, msg, value):
        Exception.__init__(self, msg, value)
        self.error = msg
        self.value = value

    def __str__(self):
        return "%s: %s" % (self.error, self.value)

    def __repr__(self):
        return self.__str__()

    def get_error_massage(self):
        return self.error

    def get_error_code(self):
        return self.value

class InvalidTypeException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class InvalidArgumentException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)


class InvalidSocketException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)


class InvalidBannerException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class ServerRespodException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class SocketReadException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class SocketSentException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class FileNotFoundException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class FileCorruptedException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class SerializationException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

class SocketTimeOutException(OsdException):
    def __init__(self, msg=None, Value=None):
        OsdException.__init__(self, msg, Value)

