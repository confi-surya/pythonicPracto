import asyncore, socket
import time
import dummy_data
from osd.glCommunicator.messages import *
from osd.glCommunicator.message_type_enum_pb2 import *


BUFF=""

type_map = {
            15: GetServiceComponentMessage,
            10: GetGlobalMapMessage,
            11: CompTransferInfoMessage,
            13: CompTransferFinalStatMessage,
            50: NodeAdditionCliMessage,
            57: LocalNodeStatusMessage,
            71: NodeDeletionCliMessage
    }

class Server(asyncore.dispatcher):
    def __init__(self, host, port):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.bind(('', port))
        self.listen(1)

    def handle_accept(self):
        socket, address = self.accept()
        print 'Connection by', address
        EchoHandler(socket)

class EchoHandler(asyncore.dispatcher):

    def handle_read(self):
        global BUFF
        BUFF = self.recv(8192)
        print "Read bytes:", len(BUFF), " data: ", BUFF

    def handle_write(self):
        global BUFF
        recv_hdr_msg = HeaderMessage()
        recv_hdr_msg.de_serialize(BUFF[0:13])
        if recv_hdr_msg.message_type() in type_map:
            msg_obj = type_map[recv_hdr_msg.message_type()]()
            msg_obj.de_serialize(BUFF[13:])
        print "Requested msg: ", recv_hdr_msg.message_type()
        if recv_hdr_msg.message_type() == typeEnums.GET_SERVICE_COMPONENT:
            print "Request received for GET_SERVICE_COMPONENT"
            msg = dummy_data.get_comp_list()
            data = msg.serialize()
            msg_hdr = HeaderMessage(payload_type=typeEnums.GET_SERVICE_COMPONENT_ACK, size=len(data))
        elif recv_hdr_msg.message_type() == typeEnums.GET_GLOBAL_MAP:
            print "Request received for GET_GLOBAL_MAP"
            msg = dummy_data.get_gl_map_message()
            data = msg.serialize()
            msg_hdr = HeaderMessage(payload_type=typeEnums.GLOBAL_MAP_INFO, size=len(data))
        elif recv_hdr_msg.message_type() == typeEnums.COMP_TRANSFER_INFO:
            print "Request received for COMP_TRANSFER_INFO"
            msg = dummy_data.get_gl_map_message()
            data = msg.serialize()
            msg_hdr = HeaderMessage(payload_type=typeEnums.COMP_TRANSFER_INFO_ACK, size=len(data))
        elif recv_hdr_msg.message_type() == typeEnums.COMP_TRANSFER_FINAL_STAT:
            print "Request received for COMP_TRANSFER_FINAL_STAT"
            msg = dummy_data.get_gl_map_message()
            data = msg.serialize()
            msg_hdr = HeaderMessage(payload_type=typeEnums.COMP_TRANSFER_FINAL_STAT_ACK, size=len(data))
        elif recv_hdr_msg.message_type() == typeEnums.NODE_ADDITION_CLI:
            print "Request received for NODE_ADDITION"
            msg = dummy_data.get_added_node_message()
            data = msg.serialize()
            msg_hdr = HeaderMessage(payload_type=typeEnums.NODE_ADDITION_CLI_ACK, size=len(data))
        elif recv_hdr_msg.message_type() == typeEnums.LOCAL_NODE_STATUS:
            print "Request received for LOCAL_NODE_STATUS"
            msg = dummy_data.get_node_status()
            data = msg.serialize()
            msg_hdr = HeaderMessage(payload_type=typeEnums.NODE_STATUS_ACK, size=len(data))
        elif recv_hdr_msg.message_type() == typeEnums.NODE_DELETION:
             print "Request received for NODE_DELETION"
             msg = dummy_data.get_node_deletion_ack(msg_obj.node_info())
             data = msg.serialize()
             msg_hdr = HeaderMessage(payload_type=typeEnums.NODE_DELETION_ACK, size=len(data))
        elif recv_hdr_msg.message_type() == typeEnums.GET_CLUSTER_STATUS:
             print "Request received for GET_CLUSTER_STATUS"
             msg = dummy_data.get_cluster_status_ack()
             data = msg.serialize()
             msg_hdr = HeaderMessage(payload_type=typeEnums.GET_CLUSTER_STATUS_ACK, size=len(data))
        BUFF = msg_hdr.serialize() + data
        print "sending data with header--> ",  msg_hdr 
        sent = self.send(BUFF)
        BUFF = BUFF[sent:]

    def writable(self):
        is_writable = (len(BUFF) > 0)
        return is_writable

    def readable(self):
        return True

s = Server('', 87654)
asyncore.loop()

