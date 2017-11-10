import os
import fnmatch
from eventlet import Timeout
from osd.glCommunicator.response import *
from osd.glCommunicator.messages import heartBeatMessage, \
GetGlobalMapMessage, GlobalMapMessage, RecvProcStartMonitoringMessage, \
RecvProcStartMonitoringMessageAck, GetServiceComponentMessage, \
GetServiceComponentAckMessage, CompTransferInfoMessage, Status, \
CompTransferInfoAckMessage, CompTransferFinalStatMessage, \
CompTransferFinalStatAckMessage, NodeAdditionCliMessage, \
NodeAdditionCliAckMessage, NodeRetireMessage, NodeRetireAckMessage, \
LocalNodeStatusMessage, NodeStatusAckMessage, NodeStopCliMessage, \
NodeStopCliAckMessage, NodeSystemStopCliMessage, GetObjectVersionMessage, \
GetObjectVersionAckMessage, NodeDeletionCliMessage, NodeDeletionCliAckMessage, \
GetClusterStatusMessage, GetClusterStatusAckMessage

from osd.glCommunicator.message_type_enum_pb2 import *
from osd.glCommunicator.service_info import ServiceInfo
from osd.glCommunicator.message_format_manager import MessageFormatManager
from osd.glCommunicator.communication.communicator \
import ConnectionFactory
from osd.glCommunicator.config import *
from osd.glCommunicator.communication.osd_exception import \
FileNotFoundException, FileCorruptedException, \
InvalidTypeException
from osd.glCommunicator.config import IoType

class FakeLogger:
    """@desc Fake logger class"""
    def __init__(self):
        pass
    def info(self, msg):
        pass
    def debug(self, msg):
        pass
    def warn(self, msg):
        pass
    def error(self, msg):
        pass

class TimeoutError:
    def __init__(self, msg):
        self.msg = msg
    def __repr__(self):
        return self.msg
    def __str__(self):
        return self.msg

class Req:
    """
    @desc Request handler class
    @param logger object for logging.
    """
    def __init__(self, logger=None, timeout=300):
        self.__timeout = timeout
        self.__logger = logger or FakeLogger()
        self.__msg_manager = MessageFormatManager(self.__logger)

    def reset_timeout(self, value=60):
        self.__timeout = value

    def get_timeout(self):
        return self.__timeout

    def get_gl_info(self):
        """
        @desc get global leader info object
        @return An object of ServiceInfo class, which contains service id, ip and port.
        @see ServiceInfo
        """
        self.__logger.debug("Search gl info file: %s" % GL_FILE_BASE)
        try:
            file_list = [f for f in os.listdir(GL_FS_PATH) \
                if fnmatch.fnmatch(f, GL_FILE_BASE+'*')]
        except OSError as ex:
            self.__logger.warn("While listing %s: %s" %(GL_FS_PATH, ex))
            raise FileNotFoundException( \
            "global leader fs path not found", 2)
        if not file_list:
            self.__logger.error("gl info file not found named with: %s" \
            % GL_FILE_BASE)
            raise FileNotFoundException( \
            "osd_global_leader_information file not found", 2)
        self.__logger.debug("gl info file list: %s" %file_list)
        gl_file = file_list[0]
        gl_file_path = os.path.join(GL_FS_PATH, gl_file)
        with open(gl_file_path, 'r') as f:
            gl_data = f.read(MAX_FILE_READ_SIZE)
        try:
            self.__logger.debug("read bytes from gl info file: %s" % gl_data)
            host_id, ip, port = gl_data.split()
        except ValueError as ex:
            self.__logger.error("While geting gl info from data: %s : %s" \
                                % (gl_data, ex))
            raise FileCorruptedException( \
            "Could not get gl info from osd_global_leader_information file")
        gl_info_obj = ServiceInfo(host_id.strip(),
                                  ip.strip(),
                                  int(port.strip()))
        return gl_info_obj

    def connector(self, conn_type, host_info):
        """
        @desc Create connection with given node
        @param conn_type: Connection type can be Io.SOCKETIO, Io.EVENTIO.
        @param host_info: host_info will be an object of ServiceInfo, with whome we want to connect.
        @return In case of success, returns communication object, None otherwise.
        """
        self.__logger.debug("Connector called: %s" % host_info)
        if conn_type == IoType.EVENTIO and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    return ConnectionFactory().get_connection(conn_type, \
                    host_info, CONNECTION_RETRY, TIMEOUT, self.__logger)
            except Timeout:
                self.__logger.error("Timeout while connecting with: %s" \
                                    % host_info)
                return None
        else:
            return ConnectionFactory().get_connection(conn_type, host_info, \
            CONNECTION_RETRY, TIMEOUT, self.__logger)

    def send_heartbeat(self, service_id, sequence, communicator_obj):
        """
        @desc Method to send heartbeat to leader.
        @param service_id: Id of sender service.
        @param sequence: A sequence number. It can odd numbers i.e. 1, 3, 5...
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains heartbeat sent status.
        @see Result
        """
        self.__logger.debug("Prepare heartBeatMessage")
        message_object = heartBeatMessage(service_id, sequence)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.HEART_BEAT, message_object)
            except Timeout:
                self.__logger.error("Timeout in send_heartbeat")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.HEART_BEAT, message_object)
        return Result(status, None)

    def get_global_map(self, service_id, communicator_obj):
        """
        @desc Request to global leader for global map.
        @param service_id: Id of sender service.
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of request and message(i.e. object of GlobalMapMessage)
        @see Result, GlobalMapMessage
        """
        self.__logger.debug("Prepare Messages for global map")
        global_map = GlobalMapMessage()
        message_object = GetGlobalMapMessage(service_id)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.GET_GLOBAL_MAP, message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, global_map)
            except Timeout:
                self.__logger.error("Timeout in get_global_map")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(\
            communicator_obj, typeEnums.GET_GLOBAL_MAP, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, global_map, self.__timeout)
        return Result(status, global_map)
        

    def recv_pro_start_monitoring(self, service_id, communicator_obj):
        """
        @desc Start monitoring of recovery process
        @param service_id: Id of sender service.
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of \
        request and message(i.e. object of RecvProStartMonitoringMessageAck)
        @see Result, RecvProStartMonitoringMessageAck
        """
        self.__logger.debug("Prepare Messages for recovery process monitoring")
        rcv_pro_strt_monitoring_ack = RecvProcStartMonitoringMessageAck()
        message_object = RecvProcStartMonitoringMessage(service_id)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.RECV_PROC_START_MONITORING, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, rcv_pro_strt_monitoring_ack)
            except Timeout:
                self.__logger.error("Timeout in recv_pro_start_monitoring")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.RECV_PROC_START_MONITORING, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, rcv_pro_strt_monitoring_ack, self.__timeout)
        return Result(status, rcv_pro_strt_monitoring_ack)

    def get_comp_list(self, service_id, communicator_obj):
        """
        @desc Request to global leader for component ownership list. \
        Global leader responds with a list of components to which caller service own.
        @param service_id: Id of sender service.
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of \
        request and message(i.e. object of GetServiceComponentAckMessage)
        @see result, GetServiceComponentAckMessage
        """
        self.__logger.debug("Prepare Messages for Service Component list")
        service_comp_ack = GetServiceComponentAckMessage()
        message_object = GetServiceComponentMessage(service_id)
        ownership_list = None
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.GET_SERVICE_COMPONENT, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, service_comp_ack)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = service_comp_ack.status()
                        ownership_list = service_comp_ack.comp_list()
            except Timeout:
                self.__logger.error("Timeout in get_comp_list")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.GET_SERVICE_COMPONENT, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, service_comp_ack, self.__timeout)
            if status.get_status_code() == Resp.SUCCESS:
                status = service_comp_ack.status()
                ownership_list = service_comp_ack.comp_list()
        return Result(status, ownership_list)

    def comp_transfer_info(self, service_id, comp_service_list, \
                                communicator_obj, local_timeout=0):
        """
        @desc A method to notify glabal leader about intermediate component transfer info.
            When one or more components transfered successfully,
            service/recovery procees will notify to glabal leader about transfered component.
        @param service_id: Id of sender service.
        @param comp_service_list: A list of touples(component number, object of ServiceInfo).
              Ex. [(1, contObj4), (2, contObj4),(3, contObj3),(4, contObj3)]
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, it contains processing status of request and message(i.e. object of CompTransferInfoAckMessage)
        @see ServiceInfo, Result, CompTransferInfoAckMessage
        """
        if not local_timeout:
            local_timeout = self.__timeout
        self.__logger.debug(\
        "Prepare Messages for partialy component transfered info")
        comp_transfer_info_ack = CompTransferInfoAckMessage()
        message_object = CompTransferInfoMessage(service_id, comp_service_list)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and local_timeout not in [None, 0]:
            try:
                with Timeout(local_timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.COMP_TRANSFER_INFO, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, comp_transfer_info_ack)
            except Timeout:
                self.__logger.error("Timeout in comp_transfer_info")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.COMP_TRANSFER_INFO, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, comp_transfer_info_ack, local_timeout)
        return Result(status, comp_transfer_info_ack)

    def comp_transfer_final_stat(self, service_id, comp_status_list, \
                             final_status, communicator_obj):
        """
        @desc When service/recovery procees finished components transfer process, notify to glabal leader about transfer status.
        @param service_id: Id of sender service.
        @param comp_status_list: A list of touples(component number, transfer status flag(True/False)).
            Ex. [(1, True), (2, False), (3, True), (4, True)]
        @param final_status: final status(i.e. True/False)
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of 
        request and message(i.e. object of CompTransferFinalStatAckMessage)
        @see ServiceInfo, Result, CompTransferFinalStatAckMessage
        """
        self.__logger.debug("Prepare Message for component transfer final stat")
        comp_transfer_final_stat_ack = CompTransferFinalStatAckMessage()
        message_object = CompTransferFinalStatMessage(service_id, \
                         comp_status_list, final_status)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try: 
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.COMP_TRANSFER_FINAL_STAT, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, comp_transfer_final_stat_ack)
            except Timeout:
                self.__logger.error("Timeout in comp_transfer_final_stat")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.COMP_TRANSFER_FINAL_STAT, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, comp_transfer_final_stat_ack, self.__timeout)
        return Result(status, comp_transfer_final_stat_ack)

    def node_addition(self, node_list, communicator_obj):
        """
        @desc Method for Node addition will be called by cli
        @param node_list: A list of nodes(i.e. objects of ServiceInfo)
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of 
        request and message(i.e. object of NodeAdditionCliAckMessage)
        @see Result, NodeAdditionCliAckMessage
        """
        self.__logger.debug("Prepare Messages for node addition")
        node_addition_ack = NodeAdditionCliAckMessage()
        message_object = NodeAdditionCliMessage(node_list)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try: 
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.NODE_ADDITION_CLI, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, node_addition_ack)
            except Timeout:
                self.__logger.error("Timeout in node_addition")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.NODE_ADDITION_CLI, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, node_addition_ack, self.__timeout)
        return Result(status, node_addition_ack)

    def node_retire(self, node_info, communicator_obj):
        """
        @desc Method for node retire.
        @param node_info: node to whom we needs to retire(i.e. an object of ServiceInfo)
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of 
        request and message(i.e. object of NodeRetireAckMessage)
        @see Result, NodeRetireAckMessage
        """
        self.__logger.debug("Prepare Messages for node retire")
        node_retire_ack = NodeRetireAckMessage()
        message_object = NodeRetireMessage(node_info)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.NODE_RETIRE, message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, node_retire_ack)
            except Timeout:
                self.__logger.error("Timeout in node_retire")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.NODE_RETIRE, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, node_retire_ack, self.__timeout)
        return Result(status, node_retire_ack)

    def node_deletion(self, node_info, communicator_obj):
        """
        @desc Method for node deletion.
        @param node_info: node to whom we needs to delete(i.e. an object of ServiceInfo)
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of 
        request and message(i.e. object of NodeDeletionCliAck)
        @see Result, NodeDeletionCliAck
        """
        self.__logger.debug("Prepare Messages for node deletion")
        deletion_ack = NodeDeletionCliAckMessage()
        message_object = NodeDeletionCliMessage(node_info)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try: 
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.NODE_DELETION, message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, deletion_ack)
            except Timeout:
                self.__logger.error("Timeout in node_deletion")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(\
            communicator_obj, typeEnums.NODE_DELETION, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, deletion_ack, self.__timeout)
        return Result(status, deletion_ack)

    def local_node_status(self, node_id, communicator_obj):
        """
        @desc Method to request for local node status.
        @param node_id: Id of dest local leader.
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of 
        request and message(i.e. object of NodeStatusAckMessage)
        @see Result, NodeStatusAckMessage
        """
        self.__logger.debug("Prepare Messages for local node status")
        node_status_enum = NodeStatusAckMessage()
        message_object = LocalNodeStatusMessage(node_id)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.LOCAL_NODE_STATUS, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        try:
                            status = self.__msg_manager.get_message_object(\
                            communicator_obj, node_status_enum)
                            self.__logger.debug("local node status: %s" \
                            % node_status_enum)
                        except InvalidTypeException, ex:
                            self.__logger.error(\
                            "node_status_enum missmatch: %s" %ex)
                            return Result(Status(Resp.INVALID_KEY, \
                            "node_status_enum missmatch"), None)
            except Timeout:
                self.__logger.error("Timeout in local_node_status")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.LOCAL_NODE_STATUS, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                try:
                    status = self.__msg_manager.get_message_object(\
                    communicator_obj, node_status_enum, self.__timeout)
                    self.__logger.debug("local node status: %s" \
                    % node_status_enum)
                except InvalidTypeException, ex:
                    self.__logger.error("node_status_enum missmatch: %s" %ex)
                    return Result(Status(Resp.INVALID_KEY, \
                    "node_status_enum missmatch"), None)
        return Result(status, node_status_enum)

    def node_stop(self, node_id, communicator_obj):
        """
        @desc Ask local leader to stop a node(used by cli).
        @param node_id: Id of dest local leader.
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of 
        request and message(i.e. object of NodeStopCliAckMessage)
        @see Result, NodeStopCliAckMessage
        """
        self.__logger.debug("Prepare Messages for node stop")
        node_stop_cli_ack = NodeStopCliAckMessage()
        message_object = NodeStopCliMessage(node_id)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.NODE_STOP_CLI, message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, node_stop_cli_ack)
            except Timeout:
                self.__logger.error("Timeout in node_stop")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.NODE_STOP_CLI, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, node_stop_cli_ack, self.__timeout)
        return Result(status, node_stop_cli_ack.status())

    def system_stop(self, node_id, communicator_obj):
        """
        @desc Ask local leader to stop a node(used by cli). This case sifnify that complete system is shuting down.
        @param node_id: Id of dest local leader.
        @param communicator_obj: A communication object of leader.
        @return An object of Result class, which contains processing status of 
        request and message(i.e. None)
        @see Result
        """
        #CLI to LL: Notify to local leader to stop.
        self.__logger.debug("Prepare Messages for system stop")
        message_object = NodeSystemStopCliMessage(node_id)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.NODE_SYSTEM_STOP_CLI, \
                    message_object)
            except Timeout:
                self.__logger.error("Timeout in system_stop")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.NODE_SYSTEM_STOP_CLI, message_object)
        return Result(status, None)

    def get_object_version(self, node_info, communicator_obj):
        self.__logger.debug("Prepare Messages for get object version")
        get_object_version_ack = GetObjectVersionAckMessage()
        message_object = GetObjectVersionMessage(node_info)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.GET_OBJECT_VERSION, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, get_object_version_ack)
            except Timeout:
                self.__logger.error("Timeout in get_object_version")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.GET_OBJECT_VERSION,  message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, get_object_version_ack, self.__timeout)
        return Result(status, get_object_version_ack)

    def get_osd_status(self, service_id, communicator_obj):
        self.__logger.debug("Prepare Messages to get osd cluster status")
        get_cluster_status_ack = GetClusterStatusAckMessage()
        message_object = GetClusterStatusMessage(service_id)
        if communicator_obj.sock_type == IoType.EVENTIO \
        and self.__timeout not in [None, 0]:
            try:
                with Timeout(self.__timeout):
                    status = self.__msg_manager.send_message_object(\
                    communicator_obj, typeEnums.GET_CLUSTER_STATUS, \
                    message_object)
                    if status.get_status_code() == Resp.SUCCESS:
                        status = self.__msg_manager.get_message_object(\
                        communicator_obj, get_cluster_status_ack)
            except Timeout:
                self.__logger.error("Timeout in get_object_version")
                return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)
        else:
            status = self.__msg_manager.send_message_object(communicator_obj, \
            typeEnums.GET_CLUSTER_STATUS, message_object)
            if status.get_status_code() == Resp.SUCCESS:
                status = self.__msg_manager.get_message_object(\
                communicator_obj, get_cluster_status_ack, self.__timeout)
                if status.get_status_code() == Resp.SUCCESS:
                    status = get_cluster_status_ack.cmd_status()
        return Result(status, get_cluster_status_ack.node_status_map())
