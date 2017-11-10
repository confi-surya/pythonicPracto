import abc
import struct

from osd.glCommunicator.service_info import ServiceInfo
from osd.glCommunicator.message_binding_pb2 import GetGlobalMap, \
GlobalMapInfo, CompTranferFinalStat, GetServiceComponent, \
GetServiceComponentAck, RecvProcStartMonitoring, RecvProcStartMonitoringAck, \
CompTransferInfo, TransferComp, NodeAdditionCli, NodeAdditionCliAck, \
NodeRetire, NodeRetireAck, LocalNodeStatus, NodeStatusAck, NodeStopCli, \
NodeStopCliAck, NodeSystemStopCli, NodeStatus, heartBeat, errorStatus, \
GetObjectVersion, GetObjectVersionAck, TransferComponentsAck, NodeDeletionCli, \
NodeDeletionCliAck, GetClusterStatus, GetClusterStatusAck, UpdateContainer, \
ReleaseTransactionLock

from osd.glCommunicator.communication.osd_exception import \
InvalidBannerException, InvalidArgumentException



class Message:
    """Base class for all messages"""
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def serialize(self):
        """@desc method to serialize message in string"""
        return

    @abc.abstractmethod
    def de_serialize(self, msg_str):
        """@desc method to de-serialize a string in message"""
        return

class Status:
    """Status class contains status code and status message string."""
    def __init__(self, status_code, status_message):
        self.__status_code = status_code
        self.__status_message = status_message
    def __repr__(self):
        return "(status code: %s, status message: %s)" \
        % (self.__status_code, self.__status_message)

    def __str__(self):
        return "(status code: %s, status message: %s)" \
        % (self.__status_code, self.__status_message)


    def get_status_code(self):
        """@desc getter method for status code"""
        return self.__status_code

    def get_status_message(self):
        """getter method for status message"""
        return self.__status_message

class HeaderMessage(Message):
    """Header message contains banner, message type enum and message size"""
    def __init__(self, banner='OSD01', payload_type=None, size=None):
        self.__banner = banner
        self.__message_type = payload_type
        self.__message_size = size
        self.__encoding_pattern = "<5cii"

    def __str__(self):
        return "Message Banner: %s, Message type: %s, Message size: %s" \
        % (self.__banner, self.__message_type, self.__message_size)

    def __repr__(self):
        return "Message Banner: %s, Message type: %s, Message size: %s" \
        % (self.__banner, self.__message_type, self.__message_size)

    def banner(self):
        """@desc Getter method for banner"""
        return self.__banner

    def message_type(self):
        """@desc Getter method for message type"""
        return self.__message_type

    def message_size(self):
        """@desc Getter method for message size"""
        return self.__message_size

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        if not type(self.__message_type) == int:
            raise InvalidArgumentException("Invalid message type")
        if not type(self.__message_size) == int:
            raise InvalidArgumentException("Invalid message size")
        return struct.pack(self.__encoding_pattern, 'O', 'S', 'D', '0', '1', \
                           self.__message_type, self.__message_size)    

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            unpacked_bytes = struct.unpack(self.__encoding_pattern, msg_str)
            for index, banner_byte in enumerate(self.__banner):
                if not banner_byte == unpacked_bytes[index]:
                    raise InvalidBannerException("Message Banner mismatched")
            self.__message_type = unpacked_bytes[index+1]
            self.__message_size = unpacked_bytes[index+2]


class GetGlobalMapMessage(Message):
    """A message class to request global map."""
    def __init__(self, service_id=None):
        self.__service_id = service_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        assert self.__service_id, "No value given for service id"
        msg_obj = GetGlobalMap()
        msg_obj.service_id = self.__service_id
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = GetGlobalMap()
            msg_obj.ParseFromString(msg_str)
            self.__service_id = msg_obj.service_id

class GlobalMapMessage(Message):
    """Global Map class. It will contains two data memebers.
       1: global_map: it is dict. Its key will be the name of service.
          And value will be a toupple, which will contain two fields.
             A: Service map version.
             B: a list of ServiceInfo's object.
          Ex: {
               "Account": (3.0, [acc1, acc2,..., acc512]),
               "Container": (4.0, [cs1, acs2,..., cs512]),
               "Object": ((3.0, [obj1, obj2,...])
              }
       2: version: Global map version.
    """
    def __init__(self, global_map=None, global_map_version=None, status=True):
        self.__global_map = global_map
        self.__version = global_map_version
        self.__status = status

    def map(self):
        """Get global map"""
        return self.__global_map

    def version(self):
        """Get global map version"""
        return self.__version

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = GlobalMapInfo()
        msg_obj.version = self.__version
        msg_obj.status = self.__status

        for service in self.__global_map["account-server"][0]:
            service_data = msg_obj.account.service_list.add()
            service_data.service_id = service.get_id()
            service_data.ip = service.get_ip()
            service_data.port = service.get_port()
        msg_obj.account.version = self.__global_map["account-server"][1]
        for service in self.__global_map["container-server"][0]:
            service_data = msg_obj.container.service_list.add()
            service_data.service_id = service.get_id()
            service_data.ip = service.get_ip()
            service_data.port = service.get_port()
        msg_obj.container.version = self.__global_map["container-server"][1]
        for service in self.__global_map["accountUpdater-server"][0]:
            service_data = msg_obj.updater.service_list.add()
            service_data.service_id = service.get_id()
            service_data.ip = service.get_ip()
            service_data.port = service.get_port()
        msg_obj.updater.version = self.__global_map["accountUpdater-server"][1]
        for service in self.__global_map["object-server"][0]:
            service_data = msg_obj.object.service_list.add()
            service_data.service_id = service.get_id()
            service_data.ip = service.get_ip()
            service_data.port = service.get_port()
        msg_obj.object.version = self.__global_map["object-server"][1]
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj =  GlobalMapInfo()
            msg_obj.ParseFromString(msg_str)
            self.__global_map = {}
            self.__global_map["account-server"] = ([ServiceInfo(
                                              service.service_id,
                                              service.ip,
                                              service.port) \
                                for service in msg_obj.account.service_list],
                            msg_obj.account.version)
            self.__global_map["container-server"] = ([ServiceInfo(
                                              service.service_id,
                                              service.ip,
                                              service.port) \
                                for service in msg_obj.container.service_list],
                            msg_obj.container.version)
            self.__global_map["accountUpdater-server"] = ([ServiceInfo(
                                              service.service_id,
                                              service.ip,
                                              service.port) \
                                for service in msg_obj.updater.service_list],
                            msg_obj.updater.version)
            self.__global_map["object-server"] = ([ServiceInfo(
                                              service.service_id,
                                              service.ip,
                                              service.port) \
                                for service in msg_obj.object.service_list],
                            msg_obj.object.version)
            self.__version = msg_obj.version
            self.__status = msg_obj.status


class CompToBeRecoverdMessage(Message):
    def __init__(self, componentMap=None):
        self.__map = componentMap

    def map(self):
        return self.__map

class CompTransferFinalStatMessage(Message):
    """
    Message class for component transfer final stat.
    Data members are:
    1: service_id: Id of caller service.
    2: comp_status_list: A list of touples(component number, transfer status flag(True/False)).
        Ex. [(1, True), (2, False), (3, True), (4, True)]
    3: final_status: final status(i.e. True/False)
    """
    def __init__(self, service_id=None, \
        comp_status_list=None, final_status=None):
        self.__service_id = service_id
        self.__comp_status_list = comp_status_list
        self.__final_status = final_status

    def service_id(self):
        """@desc Getter method for service id"""
        return self.__service_id

    def comp_status_list(self):
        """@desc Getter method for component status list"""
        return self.__comp_status_list

    def final_status(self):
        """@desc Getter method for final status"""
        return self.__final_status

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = CompTranferFinalStat()
        msg_obj.service_id = self.__service_id
        for comp_status in self.__comp_status_list:
            status_info = msg_obj.comp_status_list.add()
            status_info.component, status_info.status = comp_status
        msg_obj.final_status = self.__final_status
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            self.__comp_status_list = []
            msg_obj = CompTranferFinalStat()
            msg_obj.ParseFromString(msg_str)
            self.__service_id = msg_obj.service_id
            for status_info in msg_obj.comp_status_list:
                self.__comp_status_list.append((status_info.component, \
                status_info.status))
            self.__final_status = msg_obj.final_status


class GetServiceComponentMessage(Message):
    """Message class to request for ownership component list."""
    def __init__(self, service_id=None):
        self.__service_id = service_id

    def service_id(self):
        """@desc Getter method for caller service id"""
        return self.__service_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = GetServiceComponent()
        msg_obj.service_id = self.service_id()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = GetServiceComponent()
            msg_obj.ParseFromString(msg_str)
            self.__service_id = msg_obj.service_id

class GetServiceComponentAckMessage(Message):
    """Message class for component ownership acknowledge"""
    def __init__(self, comp_list=None, status=None):
        self.__comp_list = comp_list
        self.__status = status

    def comp_list(self):
        """@desc Getter method for ownership list"""
        return self.__comp_list

    def status(self):
        return self.__status

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = GetServiceComponentAck()
        msg_obj.component_list.extend(self.__comp_list)
        msg_obj.err.code = self.__status.get_status_code()
        msg_obj.err.msg = self.__status.get_status_message()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            self.__comp_list = []
            msg_obj = GetServiceComponentAck()
            msg_obj.ParseFromString(msg_str)
            for comp in msg_obj.component_list:
                self.__comp_list.append(comp)
            self.__status = Status(msg_obj.err.code, msg_obj.err.msg)

class RecvProcStartMonitoringMessage(Message):
    """Message class for recovery process start monitoring."""
    def __init__(self, proc_id=None):
        self.__proc_id = proc_id

    def proc_id(self):
        return self.__proc_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = RecvProcStartMonitoring()
        msg_obj.proc_id = self.proc_id()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = RecvProcStartMonitoring()
            msg_obj.ParseFromString(msg_str)
            self.__proc_id = msg_obj.proc_id

class RecvProcStartMonitoringMessageAck(Message):
    """
    Message class for recovery process start monitoring acknowledge.
    Data members are:
    1: dest_comp_map: A map where keys are the serviceObj(dest service),
       and its value is the list of components which needs to be transfered.
    2: source_service: Source service object.
    """
    def __init__(self, dest_comp_map=None, source_service=None):
        self.__dest_comp_map = dest_comp_map
        self.__source_service = source_service

    def dest_comp_map(self):
        return self.__dest_comp_map

    def source_service(self):
        #TODO: it should be the service info object instead of id.
        return self.__source_service

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = RecvProcStartMonitoringAck()
        msg_obj.dest_comp_map = self.dest_comp_map()
        msg_obj.__source_service = self.source_service()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str: 
            self.__dest_comp_map = {}
            msg_obj = RecvProcStartMonitoringAck()
            msg_obj.ParseFromString(msg_str)
            if msg_obj.status == True:
                for entry in msg_obj.service_name:
                    cmp_lst = list()
                    for comp in entry.component_list:
                        cmp_lst.append(comp)
                    self.__dest_comp_map[ServiceInfo(\
                    entry.service.service_id, entry.service.ip, \
                    entry.service.port)] = cmp_lst
                if msg_obj.source_service:
                    self.__source_service = ServiceInfo(\
                    msg_obj.source_service.service_id, \
                    msg_obj.source_service.ip, msg_obj.source_service.port)


class CompTransferInfoMessage(Message):
    """
    Message class for intermediate transfered components.
    """
    def __init__(self, service_id=None, comp_service_list=None):
        self.__service_id = service_id
        self.__comp_service_list = comp_service_list

    def service_id(self):
        return self.__service_id

    def comp_service_list(self):
        return self.__comp_service_list

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = CompTransferInfo()
        msg_obj.service_id = self.__service_id
        for comp, service in self.__comp_service_list:
            comp_service_pair = msg_obj.component_service_pair.add()
            comp_service_pair.component = comp
            comp_service_pair.dest_service.service_id = service.get_id()
            comp_service_pair.dest_service.ip = service.get_ip()
            comp_service_pair.dest_service.port = service.get_port()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = CompTransferInfo()
            msg_obj.ParseFromString(msg_str)
            self.__service_id = msg_obj.service_id
            self.__comp_service_list = []
            for comp_service in msg_obj.component_service_pair:
                self.__comp_service_list.append((comp_service.component, \
                ServiceInfo(comp_service.dest_service.service_id,
                                         comp_service.dest_service.ip,
                                         comp_service.dest_service.port)))

class CompTransferInfoAckMessage(GlobalMapMessage):
    def __init__(self, global_map=None, global_map_version=None, status=True):
        "Same as global map"
        GlobalMapMessage.__init__(self, global_map, global_map_version, status)


class CompTransferFinalStatAckMessage(GlobalMapMessage):
    """
    Message class for component transfer final stat acknowledge.
    It is same as global map message
    """
    def __init__(self, global_map=None, global_map_version=None, status=True):
        "Same as global map"
        GlobalMapMessage.__init__(self, global_map, global_map_version, status)


class TransferCompMessage(Message):

    def __init__(self, service_comp_map=None):
        self.__service_comp_map = service_comp_map

    def service_comp_map(self):
        return self.__service_comp_map

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = TransferComp()
        for service, comp_list in self.__service_comp_map.items():
            service_comp_pair = msg_obj.service_comp_list.add()
            service_comp_pair.dest_service.service_id = service.get_id()
            service_comp_pair.dest_service.ip = service.get_ip()
            service_comp_pair.dest_service.port = service.get_port()
            service_comp_pair.component.extend(comp_list)
            return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = TransferComp()
            msg_obj.ParseFromString(msg_str)
            self.__service_comp_map = {}
            for service_comp_pair in msg_obj.service_comp_list:
                cmp_lst = list()
                for comp in service_comp_pair.component:
                    cmp_lst.append(comp)
                self.__service_comp_map[ServiceInfo(\
                        service_comp_pair.dest_service.service_id,
                        service_comp_pair.dest_service.ip,
                        service_comp_pair.dest_service.port)] = cmp_lst

class TransferCompResponseMessage(Message):

    def __init__(self, comp_status_list, final_status):
        self.__comp_status_list = comp_status_list
        self.__final_status = final_status

    def serialize(self):
        msg_obj = TransferComponentsAck()
        msg_obj.final_status = self.__final_status
        for comp_status in self.__comp_status_list:
            status_info = msg_obj.comp_status_list.add()
            status_info.component, status_info.status = comp_status
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class NodeAdditionCliMessage(Message):
    """Message class for node addition request"""
    def __init__(self, node_list=None):
        self.__node_list = node_list

    def node_list(self):
        return self.__node_list

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = NodeAdditionCli()
        for node in self.__node_list:
            node_info = msg_obj.node_list.add()
            node_info.service_id =  node.get_id()
            node_info.ip = node.get_ip()
            node_info.port = node.get_port()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class NodeAdditionCliAckMessage(Message):
    """Message class for node addition acknowledge"""
    def __init__(self, node_ack_list=None):
        self.__node_ack_list = node_ack_list

    def node_ack_list(self):
        return self.__node_ack_list

    def serialize(self):
        msg_obj = NodeAdditionCliAck()
        for pair in self.__node_ack_list:
            node_ack_pair = msg_obj.node_ack_list.add()
            node_ack_pair.status.code = pair[0].get_status_code()
            node_ack_pair.status.msg = pair[0].get_status_message()
            node_ack_pair.node.service_id = pair[1].get_id()
            node_ack_pair.node.ip = pair[1].get_ip()
            node_ack_pair.node.port = pair[1].get_port()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            self.__node_ack_list = []
            msg_obj = NodeAdditionCliAck()
            msg_obj.ParseFromString(msg_str)
            for node_ack in msg_obj.node_ack_list:
                self.__node_ack_list.append(
                            (Status(node_ack.status.code, node_ack.status.msg),
                            ServiceInfo(node_ack.node.service_id,
                                        node_ack.node.ip,
                                        node_ack.node.port)))

class NodeRetireMessage(Message):
    def __init__(self, node_info=None):
        self.__node = node_info

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = NodeRetire()
        msg_obj.node.service_id = self.__node.get_id()
        msg_obj.node.ip = self.__node.get_ip()
        msg_obj.node.port = self.__node.get_port()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class NodeRetireAckMessage(Message):
    def __init__(self, node_id=None, status=None):
        self.__node_id = node_id
        self.__status = status

    def get_node_id(self):
        return self.__node_id

    def status(self):
        return self.__status

    def serialize(self):
        pass

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = NodeRetireAck()
            msg_obj.ParseFromString(msg_str)
            self.__node_id = msg_obj.node_id
            self.__status = Status(msg_obj.status.code, msg_obj.status.msg)

class NodeDeletionCliMessage(Message):
    def __init__(self, node_info=None):
        self.__node = node_info

    def node_info(self):
        return self.__node

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = NodeDeletionCli()
        msg_obj.node.service_id = self.__node.get_id()
        msg_obj.node.ip = self.__node.get_ip()
        msg_obj.node.port = self.__node.get_port()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = NodeDeletionCli()
            msg_obj.ParseFromString(msg_str)
            self.__node = ServiceInfo(msg_obj.node.service_id,
                                      msg_obj.node.ip,
                                      msg_obj.node.port)

class NodeDeletionCliAckMessage(Message):
    def __init__(self, node_id=None, status=None):
        self.__node_id = node_id
        self.__status = status

    def get_node_id(self):
        return self.__node_id

    def status(self):
        return self.__status

    def serialize(self):
        msg_obj = NodeDeletionCliAck()
        msg_obj.node_id = self.__node_id
        msg_obj.status.code = self.__status.get_status_code()
        msg_obj.status.msg = self.__status.get_status_message()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = NodeDeletionCliAck()
            msg_obj.ParseFromString(msg_str)
            self.__node_id = msg_obj.node_id
            self.__status = Status(msg_obj.status.code, msg_obj.status.msg)


class LocalNodeStatusMessage(Message):
    def __init__(self, node_id=None):
        self.__node_id = node_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = LocalNodeStatus()
        msg_obj.node_id = self.__node_id
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class NodeStatusAckMessage(Message):
    """
    Message Ack class to get the node status.
    Getter method get_node_status() gives an
    enum value.
           ---------------------------
          | RUNNING           --> 10  |
          | STOPPING          --> 20  |
          | STOP              --> 30  |
          | REGISTERED        --> 40  |
          | FAILED            --> 50  |
          | RECOVERED         --> 60  |
          | RETIRING          --> 70  |
          | RETIRED           --> 80  |
          | NODE_STOPPING     --> 90  |
          | INVALID_NODE      --> 100 |
          | NEW               --> 110 |
          | RETIRING_FAILED   --> 120 |
          | RETIRED_RECOVERED --> 130 |
          | IN_LOCAL_NODE_REC --> 140 |
          | NODE_STOPPED      --> 150 |
           ---------------------------

    """
    def __init__(self, node_status=None):
        self.__node_status = node_status

    def get_node_status(self):
        return self.__node_status

    def serialize(self):
        msg_obj = NodeStatusAck()
        msg_obj.status = self.__node_status
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = NodeStatusAck()
            msg_obj.ParseFromString(msg_str)
            self.__node_status = msg_obj.status

class NodeStopCliMessage(Message):
    def __init__(self, node_id=None):
        self.__node_id = node_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = NodeStopCli()
        msg_obj.node_id = self.__node_id
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class NodeStopCliAckMessage(Message):
    def __init__(self, status=None):
        self.__status = status

    def status(self):
        return self.__status

    def serialize(self):
        pass

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = NodeStopCliAck()
            msg_obj.ParseFromString(msg_str)
            self.__status = Status(msg_obj.status.code, msg_obj.status.msg)

class NodeSystemStopCliMessage(Message):
    def __init__(self, node_id):
        self.__node_id = node_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = NodeSystemStopCli()
        msg_obj.node_id = self.__node_id
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class OsdNodeStatusMessage(Message):
    def __init__(self, node_info=None):
        self.__node_info = node_info

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = NodeStatus()
        msg_obj.node.service_id = self.__node_info.get_service_id()
        msg_obj.node.ip = self.__node_info.get_ip()
        msg_obj.node.port = self.__node_info.get_port()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class heartBeatMessage(Message):
    def __init__(self, service_id, sequence):
        self.__service_id = service_id
        self.__sequence = sequence

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = heartBeat()
        msg_obj.msg = "HEARTBEAT"
        msg_obj.service_id = self.__service_id
        msg_obj.sequence = self.__sequence
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class GetClusterStatusMessage(Message):
    def __init__(self, service_id=None):
        self.__service_id = service_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = GetClusterStatus()
        msg_obj.service_id = self.__service_id
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class GetClusterStatusAckMessage(Message):
    def __init__(self, node_status_map=None, cmd_status=None):
        self.__node_status_map = node_status_map
        self.__cmd_status  = cmd_status

    def node_status_map(self):
        return self.__node_status_map

    def cmd_status(self):
        return self.__cmd_status

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = GetClusterStatusAck()
        for node in self.__node_status_map:
            pair_msg = msg_obj.node_status_list.add()
            pair_msg.node_id = node
            pair_msg.status_enum = self.__node_status_map[node]
        msg_obj.status.code = self.cmd_status().get_status_code()
        msg_obj.status.msg = self.cmd_status().get_status_message()
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            self.__node_status_map = {}
            msg_obj = GetClusterStatusAck()
            msg_obj.ParseFromString(msg_str)
            self.__cmd_status = Status(msg_obj.status.code, msg_obj.status.msg)
            for status_pair in msg_obj.node_status_list:
                self.__node_status_map[status_pair.node_id] = \
                status_pair.status_enum

class GetObjectVersionMessage(Message):
    def __init__(self, service_id):
        self.__service_id = service_id

    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = GetObjectVersion()
        msg_obj.service_id = self.__service_id
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        pass

class GetObjectVersionAckMessage(Message):
    def __init__(self, service_id=None, object_version=None):
        self.__service_id = service_id
        self.__object_version = object_version
        self.__status = None

    def get_service_id(self):
        return self.__service_id

    def get_version(self):
        return self.__object_version

    def get_status(self):
        return self.__status

    def serialize(self):
        pass

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = GetObjectVersionAck()
            msg_obj.ParseFromString(msg_str)
            self.__status = msg_obj.status
            if self.__status:
                self.__service_id = msg_obj.service_id
                self.__object_version = msg_obj.object_version


class UpdateContainerMessage(Message):
    def __init__(self, meta_file_path=None, operation=None):
        self.__meta_file_path = meta_file_path
        self.__operation = operation

    def meta_file_path(self):
        return self.__meta_file_path

    def operation(self):
        return self.__operation

    def serialize(self):
        pass

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = UpdateContainer()
            msg_obj.ParseFromString(msg_str)
            self.__meta_file_path = msg_obj.meta_file_path
            self.__operation = msg_obj.operation

class ReleaseTransLockMessage(Message):
    def __init__(self, lock_id=None, operation=None):
        self.__lock_id = lock_id
        self.__operation = operation

    def lock_id(self):
        return self.__lock_id

    def operation(self):
        return self.__operation

    def serialize(self):
        pass

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = ReleaseTransactionLock()
            msg_obj.ParseFromString(msg_str)
            self.__lock_id = msg_obj.lock
            self.__operation = msg_obj.operation

class ErrMessage(Message):
    """Error class"""
    def __init__(self, errorMessage=None, error_code=None):
        self.__message = errorMessage
        self.__error_code = error_code

    def message(self):
        """Get error message"""
        return self.__message

    def error_code(self):
        """Get error code"""
        return self.__error_code


    def serialize(self):
        """
        @desc Serialize message in binary string
        @return Serialize binary string.
        """
        msg_obj = errorStatus()
        msg_obj.msg = self.__message
        msg_obj.code = self.__error_code
        return msg_obj.SerializeToString()

    def de_serialize(self, msg_str):
        """
        @desc Populate message object by de-serializing binary string.
        @param msg_str: A serialize binary string.
        """
        assert type(msg_str) is str, "Input is not a string"
        if msg_str:
            msg_obj = errorStatus()
            msg_obj.ParseFromString(msg_str)
            self.__message = msg_obj.msg
            self.__error_code = msg_obj.code
        



