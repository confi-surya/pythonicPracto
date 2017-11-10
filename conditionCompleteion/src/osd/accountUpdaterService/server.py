import os
import sys
import ast
import time
from socket import gethostname
import pickle
import logging
import subprocess
import threading
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SocketServer import ThreadingMixIn
from eventlet.timeout import Timeout

from osd import gettext_ as _
from osd.common.helper_functions import float_comp
from osd.common.bufferedhttp import http_connect
from osd.common.exceptions import ChunkWriteTimeout, \
    ConnectionTimeout
from osd.common.http import is_success, HTTP_CONTINUE, \
    HTTP_INTERNAL_SERVER_ERROR, HTTP_OK, HTTP_INSUFFICIENT_STORAGE
from osd.accountUpdaterService.utils import create_recovery_file, remove_recovery_file
from osd.accountUpdaterService.updater import Updater
from osd.accountUpdaterService.walker import Walker
from osd.accountUpdaterService.reader import Reader
from osd.accountUpdaterService.utils import SimpleLogger
from osd.accountUpdaterService.accountSweeper import AccountSweep
from osd.accountUpdaterService.containerSweeper import ContainerSweeper
from osd.accountUpdaterService.monitor import WalkerMap, ReaderMap, \
SweeperQueueList, GlobalVariables
from osd.accountUpdaterService.daemon import Daemon

from osd.glCommunicator.req import Req
from osd.glCommunicator.response import Resp
from osd.glCommunicator.messages import TransferCompMessage, \
TransferCompResponseMessage
from osd.glCommunicator.service_info import ServiceInfo
from osd.glCommunicator.communication.communicator import IoType
from osd.glCommunicator.message_type_enum_pb2 import *
from libHealthMonitoringLib import healthMonitoring
import libraryUtils

class AccountUpdater(Daemon):
    """
    Update container information in account listings.
    """
    def __init__(self, conf, logger=None):
        """
        constructor for account updater
        :param conf: configuration of account-updater 
        """
        self.logger = logger or SimpleLogger(conf).get_logger_object()
        Daemon.__init__(self, conf, self.logger)
	libraryUtils.OSDLoggerImpl("account-updater-monitoring").initialize_logger()
        create_recovery_file('account-updater-server')
        self.conf = conf
        self.__interval = int(conf.get('interval', 1800))
        self.__ll_port = int(conf.get('llport', 61014))
        self.__account_updater_port = int(\
            conf.get('account_updater_port', 61009))
        self.__service_id = gethostname() + "_" + str(self.__ll_port) + \
            "_account-updater-server"
        self.__param = self.__get_param(conf)
        self.msg = GlobalVariables(self.logger)
        self.msg.set_service_id(self.__service_id)
        self.walker_map = WalkerMap()
        self.reader_map = ReaderMap()

        # Start sending health to local leader
        self.logger.info("Loading health monitoring library")
        self.health_instance = healthMonitoring(self.__get_node_ip\
            (gethostname()), self.__account_updater_port, \
            self.__ll_port, self.__service_id)
        self.logger.info("Loaded health monitoring library") 
        remove_recovery_file('account-updater-server')

	# load global map
        if not self.msg.load_gl_map():
            sys.exit(130)
        self.logger.info("Account updater started")

    def __get_node_ip(self, hostname):
        """
        Get internal node ip on which service is running
        """
        try:
            command = "grep -w " + hostname + " /etc/hosts | awk {'print $1'}"
            child = subprocess.Popen(command, stdout = subprocess.PIPE, \
                stderr = subprocess.PIPE, shell = True)
            std_out, std_err = child.communicate()
            return std_out.strip()
        except Exception as err:
            self.logger.error("Error occurred while getting ip of node:%s" %err)
            return ""

    def __get_param(self, conf):
        """
        Getting parameters through config file
        :param conf: configuration file of account-updater
        """
        return {
            'filesystems': conf.get('filesystems', '/export'),
            'stat_file_location': conf.get('stat_file_location', \
                '/export/OSP_01/updater'),
            'file_expire_time': conf.get('file_expire_time', 900),
            'interval': conf.get('interval', 600),
            'conn_timeout': float(conf.get('conn_timeout', 10)),
            'node_timeout': float(conf.get('node_timeout', 10)),
            'osd_dir': conf.get('osd_dir', '/export/.osd_meta_config'),
            'reader_max_count': int(conf.get('reader_max_count', 10)),
            'max_update_files': int(conf.get('max_update_files', 10)),
            'max_files_stored_in_cache': int(conf.get('max_files_stored_in_cache', 100)),
            'updaterfile_expire_delta_time': int(conf.get('updaterfile_expire_delta_time', 1020))
            }

    def run_forever(self, *args, **kwargs):
        """
        Run the updator continuously.
        """
        try:
            self.logger.debug('Begin account update')

            # get account-updater server ownership
            self.get_ownership_obj = threading.Thread(target = self.msg.get_my_ownership)
            self.get_ownership_obj.setDaemon(True)
            self.get_ownership_obj.start()

            self.walker_obj = Walker(self.walker_map, self.__param, self.logger)
            self.walker_obj.setDaemon(True)
            self.walker_obj.start()
            self.logger.info("Walker Started")
            self.reader_obj = Reader(self.walker_map, self.reader_map, \
                                self.__param, self.logger)
            self.reader_obj.setDaemon(True)
            self.reader_obj.start() 
            self.logger.info("Reader Started")
            self.account_sweeper = AccountSweep(self.__param, self.logger)
            self.account_sweeper.setDaemon(True)
            self.account_sweeper.start()
            self.logger.info("Account Sweeper Started")     
            self.updater_obj = Updater(self.walker_map, self.reader_map, \
                           self.__param, self.logger)
            self.updater_obj.setDaemon(True)
            self.updater_obj.start()   
            self.logger.info("Updater Started") 
            self.container_sweeper = ContainerSweeper(self.walker_map, \
                           self.reader_map, self.__param, self.logger)
            self.container_sweeper.setDaemon(True)
            self.container_sweeper.start()
            self.logger.info("Container Sweeper Started") 

            account_updater_server = ThreadedAccountUpdaterServer(\
                (self.__get_node_ip(gethostname()), \
                self.__account_updater_port), HttpListener)
            account_updater_server.serve_forever()
        except Exception as ex:
            self.logger.error("Exception occured: %s" % ex)


class ThreadedAccountUpdaterServer(ThreadingMixIn, HTTPServer):
    pass

class HttpListener(BaseHTTPRequestHandler):

    def do_STOP_SERVICE(self):
        """
        Handle COMPLETE ALL REQUEST HTTP request
        """
        self.logger = SimpleLogger(conf=None).get_logger_object()
        self.conf = SimpleLogger(conf=None).get_conf()
        self.msg = GlobalVariables(self.logger)
        all_transfer_event_received = []
        self.logger.info("Account-updater received STOP_SERVICE request")

        complete_all_request_event = self.msg.get_complete_all_event()
        complete_all_request_event.set()
        try:
            while len(all_transfer_event_received) != 4:
                all_transfer_event_received.append(self.msg.get_from_Queue())
            
            self.logger.info("Completed STOP_SERVICE request")
            self.send_header('Message-Type', typeEnums.BLOCK_NEW_REQUESTS_ACK)
            self.send_header('Ownership-List', self.msg.get_ownershipList())
            self.send_response(HTTP_OK)
            self.end_headers()
            self.wfile.write(self.msg.get_ownershipList())
            return
        except Exception as err:
            self.logger.exception('Exception raised in' \
                ' STOP_SERVICE error :%s' % err)
            self.send_response(HTTP_INTERNAL_SERVER_ERROR)
            self.end_headers() 

    def do_ACCEPT_COMPONENT_TRANSFER(self):
        """
        Handle ACCEPT COMPONENT TRANSFER HTTP request
        """
        try:
            self.logger = SimpleLogger(conf=None).get_logger_object()
            self.conf = SimpleLogger(conf=None).get_conf()
            self.msg = GlobalVariables(self.logger)
            self.ll_port = int(self.conf.get('llport', 61014))
            self.logger.info("Account-updater received ACCEPT_COMPONENT_" \
                "TRANSFER request")
            length = int(self.headers['Content-Length'])
            self.logger.debug("Headers:%s" %self.headers)
            #sending intemediate (connection created) acknowledgement 
            #to source node
            self.send_response(HTTP_CONTINUE)
            self.end_headers()
            
            #receiving new ownership list
            pickled_string = self.rfile.read(length)
            add_comp_list = ast.literal_eval(pickled_string)
            self.logger.info("Accepting new component ownership: %s"% add_comp_list)


            #updating global map for new onwership
            thread = threading.Thread(target = self.update_my_ownership, \
                                    args=(add_comp_list,))
            thread.start()

            self.logger.info("Completed ACCEPT_COMPONENTS_TRANSFER request")
            self.send_response(HTTP_OK)
            self.end_headers()
            return
        except Exception as err:
            self.logger.exception('Exception raised in' \
                'ACCEPT_COMPONENTS_TRANSFER error :%s' % err)
            self.send_response(HTTP_INTERNAL_SERVER_ERROR)
            self.end_headers()

    def do_TRANSFER_COMPONENTS(self):
        """
        Handle TRANSFER COMPONENTS HTTP request
        """
        self.logger = SimpleLogger(conf=None).get_logger_object()
        self.conf = SimpleLogger(conf=None).get_conf()
        self.msg = GlobalVariables(self.logger)
        self._request_handler = Req()
        transfer_component_timeout = int(self.conf.get('\
            transfer_component_timeout', 600))
        self.ll_port = int(self.conf.get('llport', 61014))
        self.service_id = self.msg.get_service_id()
        self.deleted_comp_list = [] 
        transfer_component_map = {}             #dictionary containing{'(dest_node_obj)':'[comp_list]'}
                                                #eg: {('169.254.1.12', '61009', 'HN0101_61014_account-updater'):'['1', '2']'}
        self.final_transfer_status_list = []    #final component status list which will be send to GL
                                                #[(1, True),(2, False),(3, True)] 
        self.final_status = False               #final response to GL
        self.check_transfer_component_map = {}  # to check if component transfer completed or failed: 
                                                #{dest_node_obj1:True, dest_node_obj2:"Failed", dest_node_obj3:False}
        all_transfer_event_received = []
        self.protocol_version = "HTTP/1.1"
        self.logger.info("Account-updater received Transfer component request")
        try:
            content_length = int(self.headers['Content-Length'])
            #sending acknowledgement of TRANSFER_COMPONENT to GL 
            self.send_response(HTTP_CONTINUE, "Continue\r\n\r\n")

            data = self.rfile.read(content_length)
            message = TransferCompMessage()
            message.de_serialize(data)
            transfer_component_map = message.service_comp_map()
            self.logger.info("Deserialized transfer component map:%s" \
                %transfer_component_map)
        except Exception, ex:
            self.logger.error("Exception: %s" % ex)
            self.send_header('Message-Type', \
                typeEnums.TRANSFER_COMPONENT_RESPONSE)
            self.send_response(HTTP_INTERNAL_SERVER_ERROR)
            self.end_headers()
            return

        #if transfer_component_map is empty then send HTTP_OK to GL
        if not transfer_component_map:
            self.logger.info("Deserialized transfer component map is empty, return HTTP OK!")
            comp_transfer_response = TransferCompResponseMessage(\
                self.final_transfer_status_list, True)
            serialized_body = comp_transfer_response.serialize()

            self.send_header('Message-Type', \
                typeEnums.TRANSFER_COMPONENT_RESPONSE)
            self.send_header('Content-Length', len(serialized_body))
            self.send_response(HTTP_OK)
            self.end_headers()
            self.wfile.write(serialized_body)
            return
 
        for dest_node_obj, comp_list in transfer_component_map.items():
            for comp in comp_list:
                self.deleted_comp_list.append(comp)
                self.final_transfer_status_list.append((comp, False))
            self.check_transfer_component_map[dest_node_obj] = False
        self.logger.debug("Ownership for components:%s will be removed" %self.deleted_comp_list)
        self.delete_self_ownership()
        transfer_comp_event = self.msg.get_transfer_cmp_event()
        transfer_comp_event.set()

        try:
            #sending accept component request to other nodes
            self.logger.info("Sending accept component request to " \
                "destination nodes")
            for target_service_obj, comp_list in transfer_component_map.items():
                thread_connecting_node = threading.Thread(target = \
                    self.send_accept_component_request, args = \
                    ("ACCEPT_COMPONENT_TRANSFER", target_service_obj, \
                    comp_list, ))
                thread_connecting_node.setDaemon(True)
                thread_connecting_node.start()

            #Checking if transfer component completed and intermediate 
            #response sent or not
            self.logger.info("Checking if transfer component completed")
            thread_check_transfer_status = threading.Thread(target = \
                self.check_component_transfer_completion)
            thread_check_transfer_status.setDaemon(True)
            thread_check_transfer_status.start()
            thread_check_transfer_status.join(transfer_component_timeout)

            # sending final response to GL
            self.logger.info("Sending final response to GL :%s" \
                % self.final_transfer_status_list)
            if self.final_transfer_status_list:
                for comp_status_tuple in self.final_transfer_status_list:
                    if not comp_status_tuple[1]:
                        self.logger.warning("Final transfer component list having failed" \
                            "component:%s, %s" %(comp_status_tuple[0], comp_status_tuple[1]))
                        self.msg.set_ownershipList(self.old_ownership_list)
                        break
                else:
                    self.final_status = True
            comp_transfer_response = TransferCompResponseMessage(\
                self.final_transfer_status_list, self.final_status)
            serialized_body = comp_transfer_response.serialize()
             
            while len(all_transfer_event_received) != 4:
                all_transfer_event_received.append(self.msg.get_from_Queue())
            
            self.logger.info("Completed TRANSFER_COMPONENTS request")
            self.send_header('Message-Type', \
                typeEnums.TRANSFER_COMPONENT_RESPONSE)
            self.send_header('Content-Length', len(serialized_body))
            self.send_response(HTTP_OK)
            self.end_headers()
            self.wfile.write(serialized_body)
            transfer_comp_event.clear()
            return
        except Exception as ex:
            self.logger.error("Exception raised: %s" % ex)
            transfer_comp_event.clear()
            self.msg.set_ownershipList(self.old_ownership_list)
            self.logger.error("old_ownership_list:%s" %self.old_ownership_list)
            self.send_header('Message-Type', \
                typeEnums.TRANSFER_COMPONENT_RESPONSE)
            self.send_response(HTTP_INTERNAL_SERVER_ERROR)
            self.end_headers()

    def update_final_transfer_status(self, comp_list):
        """
        Updating final transfer component map with True value for components
        in component list which will be send to GL

        :param comp_list: component list 
        """
        for comp in comp_list:
            for index in range(len(self.final_transfer_status_list)):
                if self.final_transfer_status_list[index][0] == comp:
                    self.final_transfer_status_list[index] = (comp, True)
        self.logger.info("Updated final transfer status list : %s" \
            % self.final_transfer_status_list)

    def check_component_transfer_completion(self):
        """
        Checking if transfer component completed and intermediate 
        response sent or not
        """
        while self.check_transfer_component_map:
            for dest_node_obj, value in \
                self.check_transfer_component_map.items():
                if value == True or value == "Failed":
                    del self.check_transfer_component_map[dest_node_obj]
                    self.logger.info("Processed one node transfer : %s, id : \
                        %s, status : %s" %(dest_node_obj, \
                        dest_node_obj.get_id(), value))
                else:
                    continue
            time.sleep(5)

    def send_accept_component_request(self, method, target_service_obj, \
                                                            comp_list):
        """
        Creating http connection, sending ACCEPT_COMPONENT_TRANSFER
        http request with component transfer list to target node
        and sending intermediate response to GL.

        :param method: ACCEPT_COMPONENT_TRANSFER 
        :param target_service_obj: target node object containing (ip, port, id)
        :param comp_list: transfer component list, need to send to target node 
    
        """
        headers = {}
        create_connection = False
        conn_timeout = float(self.conf.get('conn_timeout', 10))
        node_timeout = int(self.conf.get('node_timeout', 10))
        self.logger.debug("Entering connect_target_node method")
        filesystem = 'export'
        directory = 'OSP_01'
        path = '/recovery_process/'
        #Parameters are fixed, could be changes according to requirement.
        headers['Expect'] = '100-continue'
        headers['X-Timestamp'] = time.time()
        headers['Content-Type'] = 'text/plain'
        try:
            #header_content_key = (self.node['ip'], self.node['port'])
            Content_length = len(str(comp_list))
            headers['Content-Length'] = Content_length 
            self.logger.info("Header Sent:%s, ip:%s, port:%s, filesystem:%s,"
                " directory:%s, path:%s method:%s" %(headers, \
                target_service_obj.get_ip(), target_service_obj.get_port(), \
                filesystem, directory, path, method))

            #Creating http connection to target node
            with ConnectionTimeout(conn_timeout):
                conn = http_connect(
                    target_service_obj.get_ip(), target_service_obj.get_port(),\
                    filesystem, directory, method, path, headers)
            with Timeout(node_timeout):
                resp = conn.getexpect()

            if resp.status == HTTP_CONTINUE:
                conn.resp = None
                self.logger.info("HTTP continue %s" % resp.status)
                create_connection = True
            elif is_success(resp.status):
                conn.resp = resp
                self.logger.info("Successfull status:%s" % resp.status)
                create_connection = True
            elif resp.status == HTTP_INSUFFICIENT_STORAGE:
                self.logger.error('ERROR Insufficient Storage' \
                      'ip:%s, port:%s' %(target_service_obj.get_ip(), \
                      target_service_obj.get_port()))
                create_connection = False
                self.check_transfer_component_map[target_service_obj] = "Failed"
        except (Exception, Timeout) as err:
            self.logger.exception(
                "Exception occured: %s during http connect id :%s, \
                Expected: 100-continue" % (err, target_service_obj.get_id()))
            create_connection = False
            self.check_transfer_component_map[target_service_obj] = "Failed"

        # sending component list to target node over http connection
        if create_connection:
            conn.reader = str(comp_list)
            self.logger.info("Sending component List: %s" % \
                conn.reader)
            try:
                with ChunkWriteTimeout(node_timeout):
                    conn.send(conn.reader)
                    conn.send_data = True
            except (Exception, ChunkWriteTimeout) as err:
                self.logger.error('Exception occured : %s at id : %s \
                    info: send file failed' %(target_service_obj.get_id(), err))
                conn.send_data = False
                self.check_transfer_component_map[target_service_obj] = \
                    "Failed"

            self.logger.info("Sending component list:%s completed ip:%s port:%s"
                %(comp_list, target_service_obj.get_ip(), \
                target_service_obj.get_port()))

            def get_conn_response(conn, comp_list, node_timeout):
                """
                Getting connection response
                """
                try:
                    with Timeout(node_timeout):
                        if conn.resp:
                            self.logger.debug("conn.resp returned")
                            return conn.resp
                        else:
                            self.logger.debug("conn.getexpect()")
                            return conn.getexpect()
                except (Exception, Timeout):
                    self.check_transfer_component_map[target_service_obj] = \
                        "Failed"
                    self.logger.exception('get_conn_response: Trying to get \
                        final status for id:%s' %(target_service_obj.get_id()))

            retry_count = 3
            counter = 0
            if conn.send_data:
                response = get_conn_response(conn, comp_list, node_timeout)
                if response and is_success(response.status):
                    # send intermediate status to GL
                    self.logger.info("Sending intermediate state to GL for \
                        target service : %s" % target_service_obj.get_id())
                    transferred_comp_list = []
                    for comp in comp_list:
                        transferred_comp_list.append((comp, target_service_obj))
                    while counter < retry_count:
                        counter += 1
                        gl_info_obj = self._request_handler.get_gl_info()
                        conn_obj = self._request_handler.connector(\
                            IoType.EVENTIO, gl_info_obj)
                        if conn_obj != None:
                            ret = self._request_handler.comp_transfer_info(\
                                self.service_id, transferred_comp_list, \
                                conn_obj)
                            if ret.status.get_status_code() == Resp.SUCCESS: 
                                self.logger.info("Sent intermediate response " \
                                    ":%s to GL" %transferred_comp_list)
                                self.update_final_transfer_status(comp_list)
                                self.check_transfer_component_map[\
                                    target_service_obj] = True           
                                break
                            else:
                                self.logger.warning("Sending intermediate" \
                                    "response to GL failed, Retrying")
                        conn_obj.close()
                    if ret.status.get_status_code() != Resp.SUCCESS:
                        self.check_transfer_component_map[target_service_obj] =\
                            "Failed"
                else:
                    self.logger.error('get_response failed id:%s' \
                        %(target_service_obj.get_id()))
                    self.check_transfer_component_map[target_service_obj] = \
                        "Failed"

    def delete_self_ownership(self):
        """
        Deleting transferred component ownership
        """
        current_ownership_list = self.msg.get_ownershipList()
        self.old_ownership_list = current_ownership_list
        for comp in self.deleted_comp_list:
            if comp in current_ownership_list:
                current_ownership_list.remove(comp)
        self.logger.debug("After removing transfer component ownership, \
            new ownership: %s" % current_ownership_list)
        self.msg.set_ownershipList(current_ownership_list)

    def update_my_ownership(self, add_comp_list):
        """
        Updating accept component ownership
        """
        all_transfer_event_received = []
        transfer_comp_event = self.msg.get_transfer_cmp_event()
        acc_updater_map_version = self.msg.get_acc_updater_map_version()
        self.msg.load_gl_map()
        while float_comp(acc_updater_map_version, \
            self.msg.get_acc_updater_map_version()) >= 0:
            time.sleep(20)
            self.logger.info("Account updater map version is not updated old"\
                "map version:%s, new map version:%s" %(acc_updater_map_version,\
                 self.msg.get_acc_updater_map_version()))
            self.msg.load_gl_map()

        transfer_comp_event.set()
        self.msg.load_ownership()
        self.updated_comp_list = self.msg.get_ownershipList()
        while not self.check_ownership_updated(add_comp_list):
            time.sleep(20)
            self.msg.load_ownership()
            self.updated_comp_list = self.msg.get_ownershipList()

        self.logger.info("Updating ownership :%s" %self.updated_comp_list)
        self.msg.set_ownershipList(self.updated_comp_list)

        while len(all_transfer_event_received) != 4:
            all_transfer_event_received.append(self.msg.get_from_Queue())
        transfer_comp_event.clear()
        self.logger.info("transfer/accept component event is cleared")
        add_comp_list = []
        

    def check_ownership_updated(self, add_comp_list):
        updated = True
        for val in add_comp_list:
            if val not in self.updated_comp_list:
                updated = False
                break
        return updated

