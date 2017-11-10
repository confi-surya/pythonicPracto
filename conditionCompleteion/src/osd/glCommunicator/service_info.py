"""Service info class."""

class ServiceInfo:
    """Class for the service info"""
    def __init__(self, host_id=None, host_ip=None, host_port=None):
        self.__host_id = host_id
        self.__host_ip = host_ip
        self.__host_port = host_port

    def __repr__(self):
        return "(Id: %s, Ip: %s, Port: %s)" \
        % (self.__host_id, self.__host_ip, self.__host_port)

    def __str__(self):
        return "(Id: %s, Ip: %s, Port: %s)" \
        % (self.__host_id, self.__host_ip, self.__host_port)

    def get_id(self):
        """@desc Get Service Id"""
        return self.__host_id

    def get_ip(self):
        """@desc Get Ip"""
        return self.__host_ip

    def get_port(self):
        """@desc Get Port"""
        return self.__host_port

