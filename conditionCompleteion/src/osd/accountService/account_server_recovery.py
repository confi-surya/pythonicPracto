import os
import sys
import procname
import time
import argparse
import pickle
import ConfigParser
from osd import gettext_ as _
from osd.common.bufferedhttp import http_connect
from eventlet import Timeout, sleep, event
import eventlet
from osd.glCommunicator.communication.communicator import IoType
from osd.common.utils import ContextPool , GreenthreadSafeIterator, \
    GreenAsyncPile
from osd.common.http import HTTP_CONTINUE
from osd.common.exceptions import ChunkWriteTimeout, ConnectionTimeout
from osd.glCommunicator.req import Req
from osd.glCommunicator.response import Resp
from osd.glCommunicator.message_type_enum_pb2 import *
from osd.accountService.utils import temp_clean_up_in_comp_transfer
from osd.common.utils import get_logger

MAX_RETRY_COUNT = 3

class RecoveryData():
    '''Recovery resources data class '''
    def __init__(self):
        # map that keeps dest_service_obj and comp_list
        self._recovery_map = dict()
        #transfered component list
        self._transfered_comp_list = list()

    def get_recovery_map(self):
        return self._recovery_map

    def set_recovery_map(self, recovery_dict):
        self._recovery_map.update(recovery_dict)

    def get_transfered_comp_list(self):
        return self._transfered_comp_list

    def extend_transfered_comp_list(self, comp_list):
        self._transfered_comp_list.extend(comp_list)

    def append_transfered_comp_list(self, elem):
        self._transfered_comp_list.append(elem)
        
class Recovery:
    '''Account server Recovery'''
    def __init__(self, conf, logger, source_service_id, communicator, recovery_data_obj):
        self._root = conf.get('filesystems', '/export')
        self._base_fs = conf.get('base-fs', 'OSP_01')
        self._communicator_obj = communicator
        self._source_service_id = source_service_id
        self._server = 'account'
        self._recovery_data_obj = recovery_data_obj
        self.logger = logger
        self.conf = conf
        self._con_obj = None

        self._final_recovery_status_list = list()
        self._final_status = True
        self._network_fail_status_list = [Resp.BROKEN_PIPE, Resp.CONNECTION_RESET_BY_PEER, Resp.INVALID_SOCKET]
        self._success_status_list = [Resp.SUCCESS]

        #flags
        self._intermediate_flag = False
        self._finish_recovery_flag = False
        self._latest_gl_version = None
        if not self.__load_gl_map_info():
            exit(1)

        # condition variables
        self._strm_ack_evt = event.Event()

    def connect_gl(self):
        gl_info_obj = self._communicator_obj.get_gl_info()
        self._con_obj = self._communicator_obj.connector(IoType.EVENTIO, gl_info_obj)
        if self._con_obj:
            return True

    def __load_gl_map_info(self):
        retry = 3
        counter = 0
        while counter < retry:
            gl_info_obj = self._communicator_obj.get_gl_info()
            con_obj = self._communicator_obj.connector(IoType.EVENTIO, gl_info_obj)
            if con_obj:
                ret = self._communicator_obj.get_global_map(\
                "account-recovery", con_obj)
                if ret.status.get_status_code() \
                    in self._success_status_list:
                    self._latest_gl_version = ret.message.version()
                    self.logger.debug("latest gl version: %s" % self._latest_gl_version)
                    return True
                self.logger.warn("get_global_map error code: %s msg: %s" \
                % (ret.status.get_status_code(), \
                ret.status.get_status_message()))
                con_obj.close()
            self.logger.warn("Failed to get global map, Retry: %s" % counter)
            counter += 1
        self.logger.error("Failed to laod global map")
        return False



    def __reset_con_obj(self):
        if self._con_obj:
            self._con_obj.close()
        self.connect_gl()

    def start_recovery(self):
        self.logger.info("start_recovery start")
        heart_beat_gthread = eventlet.spawn(self.start_heart_beat) 

        #temp_base_path = os.path.join(self._root, self._base_fs, 'tmp')
       
        # wait for event until recovery_map not populated by heartbeat thread
        self._strm_ack_evt.wait()
        self.logger.info("strm_ack event received")
        # temp cleanup
        pile_size = len(self._recovery_data_obj.get_recovery_map())
        pile =  GreenAsyncPile(pile_size)
        target_node_service_iter = GreenthreadSafeIterator(self._recovery_data_obj.get_recovery_map()) #can be used with .values
        for target_service_obj in target_node_service_iter:
            self.logger.info("target service: %s, comp list: %s" \
            %(target_service_obj, self._recovery_data_obj.get_recovery_map()[target_service_obj]))
            pile.spawn(self.__transfer_ownership,
                       target_service_obj, 
                       self._recovery_data_obj.get_recovery_map()[target_service_obj],
                       self.logger.thread_locals)
        # send intermediate response to GL
        for dest_serv_obj, component_list in pile:
            if len(component_list) == len(self._recovery_data_obj.get_recovery_map()[dest_serv_obj]):
                #All of components temp cleanup is successful
                self._recovery_data_obj.extend_transfered_comp_list([(comid, dest_serv_obj) for comid in component_list])
                self._final_recovery_status_list.extend([(compid, True) for compid in component_list])
                
            else:
                #Some of compoents temp cleanup failed
                self._final_status = False
                if component_list:
                    # Success case for particular component
                    self._recovery_data_obj.extend_transfered_comp_list(
                        [(comid, dest_serv_obj) for comid in component_list])
                    self._final_recovery_status_list.extend([(compid, True) \
                        for compid in component_list])
                    # set difference is fail case for particular component
                    self._final_recovery_status_list.extend([(compid, False) \
                    for compid in set(\
                    self._recovery_data_obj.get_recovery_map()[dest_serv_obj]\
                                    ) - set(component_list)])
             
        self._intermediate_flag = True
        pile.waitall(30)
        
        #wait for intermmediate response
        while self._intermediate_flag:
            sleep(1)

        if self._latest_gl_version == -1:
            self.logger.error("Intermediate response sending to GL failed even after retry")
            sys.exit(1)

        # set the finish transfer flag
        self._finish_recovery_flag = True

        #join heartbeat thread for its completion
        heart_beat_gthread.wait()
        self.logger.info("Account server id : %s Recovery exiting" % self._source_service_id)

    def __clean_temp_data(self, target_service_obj, comp_list):
        #self.logger.thread_locals = logger_thread_locals
        transfered_comp_list = list()
        for comp_id in comp_list:
            # temp cleanup
            temp_dir = os.path.join(self._root, self._base_fs, 'tmp')
            status = temp_clean_up_in_comp_transfer(temp_dir, self._server, comp_id, self.logger)
            if status:
                transfered_comp_list.append(comp_id)
        return (target_service_obj, transfered_comp_list)

    def __send_http_to_target(self, target_service, transfered_comp_list, \
                           logger_thread_locals):
        self.logger.thread_locals = logger_thread_locals
        method = "ACCEPT_COMPONENTS"
        http_con = self.__connect_target_node(target_service, transfered_comp_list, logger_thread_locals, method)
        if http_con:
            self.logger.info("http connection for ACCEPT_COMPONENTS is established from source %s to target %s" \
                            % (self._source_service_id, target_service.get_id()))
            #serialize data
            # note : take care of transfered_comp_list in case of some components cleanup
            http_con.reader = pickle.dumps(transfered_comp_list)
            self.logger.debug("component list going to be transfer: %s" % transfered_comp_list)
            self.__send_data(http_con, target_service)
            resp = http_con.getresponse()
            if resp.status == 200: #HTTPOk 
                self.logger.info("ACCEPT_COMPONENTS request for target %s completed successfully" % target_service.get_id())
                return True
            else:
                self.logger.error("ACCEPT_COMPONENTS request failed for target %s , return status %s" \
                                                            % (target_service.get_id(), resp.status))
        else:
            self.logger.error("http connection for ACCEPT_COMPONENTS is failed from source %s to target %s" \
                                                            % (self._source_service_id, target_service.get_id()))
        return False

    def __transfer_ownership(self, target_service_obj, comp_list, logger_thread_locals):
        target_service_obj, cleaned_comp_list = \
        self.__clean_temp_data(target_service_obj, comp_list)
        if self.__send_http_to_target(target_service_obj, \
            cleaned_comp_list, logger_thread_locals):
            return (target_service_obj, cleaned_comp_list)
        return (target_service_obj, [])

        

    def __connect_target_node(self, target_service, comp_list, \
                            logger_thread_locals, method):
        self.logger.thread_locals = logger_thread_locals
        #client_timeout = int(self.conf.get('CLIENT_TIMEOUT', 500))
        conn_timeout = float(self.conf.get('CONNECTION_TIMEOUT', 30))
        node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))

        #Some values are needed for connections.
        filesystem = 'export'
        directory = 'OSP_01'
        path = '/recovery_process/'

        try:
            # create headers 
            headers = dict()
            headers['Expect'] = '100-continue'
            headers['X-Timestamp'] = time.time()
            headers['Content-Type'] = 'text/plain'
            headers['X-GLOBAL-MAP-VERSION'] = self._latest_gl_version
            headers['Content-Length'] = len(pickle.dumps(comp_list))
            with ConnectionTimeout(conn_timeout):
                conn = http_connect(
                         target_service.get_ip(), target_service.get_port(), filesystem, directory,  method, path, headers)
            with Timeout(node_timeout):
                resp = conn.getexpect()
            if resp.status == HTTP_CONTINUE:
                conn.resp = None
                conn.target_service = target_service
                self.logger.info("HTTP continue %s" % resp.status)
                return conn
            else:
                self.logger.error("Http connection status: %s" % resp.status)
                return None
        except (Exception, Timeout) as err:
            self.logger.exception(
                _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                '%(info)s'),
                {'type': "Account", 'ip': target_service.get_ip(), 'port': target_service.get_port(), #check
                'info': "Expect: 100-continue on "})


    def __send_data(self, conn, service_obj):
        """Method for sending data"""
        self.logger.info("send_data started")
        node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
        bytes_transferred = 0
        chunk = ""
        while conn.reader:
            chunk = conn.reader[:65536]
            bytes_transferred += len(chunk)
            try:
                with ChunkWriteTimeout(node_timeout):
                    self.logger.debug('Sending chunk%s' % chunk)
                    conn.send(chunk)
                    conn.reader = conn.reader[65536:]
            except (Exception, ChunkWriteTimeout):
                self.logger.exception(
                    _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                    '%(info)s'),
                    {'type': "Account", 'ip': service_obj.get_ip(), 'port': service_obj.get_port(),
                    'info': "send data failed "})
        self.logger.info("Bytes transferred: %s" % bytes_transferred)


    def __send_transfer_status(self):
        counter = 0
        while counter < MAX_RETRY_COUNT:
            self.logger.info("Sending intermediate response to GL")
            ret = self._communicator_obj.comp_transfer_info(self._source_service_id, \
                                        self._recovery_data_obj.get_transfered_comp_list(), \
                                        self._con_obj)
            if ret.status.get_status_code() in self._success_status_list:
                self.logger.info("Account server recovery server_id : %s Component transfer Completed" % self._source_service_id)
                self._latest_gl_version = ret.message.version()
                break
            elif ret.status.get_status_code() in self._network_fail_status_list:
                self.logger.error("comp_transfer_info failed with: %s" % ret.status)
                self._con_obj.close()
                sys.exit(1)
            else:
                self.logger.info("Account server recovery server_id : %s, Retrying... " % self._source_service_id)
            counter += 1
        if ret.status.get_status_code() not in self._success_status_list:
            #set invalid gl versin value in case of failure
            self.logger.error("Account server recovery server_id : %s Component transfer Failed" % self._source_service_id) 
            self._latest_gl_version = -1


    def start_heart_beat(self):
        #Other Way: create health monitoring obj and start monitoring
        self.logger.info("start_heart_beat start")
        #send strm
        ret = self._communicator_obj.recv_pro_start_monitoring( \
                            self._source_service_id, self._con_obj)
        if ret.status.get_status_code() in self._network_fail_status_list:
            self.logger.error("recv_pro_start_monitoring failed with: %s" % ret.status)
            self._con_obj.close()
            sys.exit(1)
        if ret.status.get_status_code() in self._success_status_list:
            self._recovery_data_obj.set_recovery_map(ret.message.dest_comp_map())
            # send event to recovery green thread
            self._strm_ack_evt.send()
            self.logger.info("strm_ack event is sent to recovery gthread")
       
        sequence_counter = 1
        #start heart beat
        while not self._finish_recovery_flag:
            if self._intermediate_flag:
                # main thread set intermediate flag after temp cleanup
                self.logger.info("intermediate recovery flag is set, Sending status to GL")
                self.__send_transfer_status()
                #To continue heartbeats, unset flag once transfer completed
                self._intermediate_flag = False
            else:
                self.logger.debug("sending heartbeat to GL")
                ret = self._communicator_obj.send_heartbeat(\
                self._source_service_id, sequence_counter, self._con_obj)
            	sleep(10)
            sequence_counter += 1 

        #send end monitoring
        self.__end_monitoring()

    def __end_monitoring(self):
        self.logger.info("sending end monitoring to GL")
        ret = self._communicator_obj.comp_transfer_final_stat(\
        self._source_service_id, self._final_recovery_status_list, \
        self._final_status, self._con_obj)
        if ret.status.get_status_code() in self._network_fail_status_list:
            self.logger.error("comp_transfer_final_stat failed with: %s" % ret.status)
            self._con_obj.close()
            sys.exit(1)

              
             
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Recovery process option parser")
    parser.add_argument("server_id", help="Service id from Global leader.")
    args = parser.parse_args()
    acc_service_id = args.server_id

    #Change Process name of Script.
    script_prefix = "RECOVERY_"
    proc_name = script_prefix + acc_service_id
    procname.setprocname(proc_name)

    #create config object
    CONFIG_PATH = "/opt/HYDRAstor/objectStorage/configFiles/account-server.conf"
    config = ConfigParser.ConfigParser()
    config.read(CONFIG_PATH)
    conf_obj = dict(config.items('DEFAULT'))

    #create logger obj
    logger_obj = get_logger(conf_obj, log_route='recovery')

    logger_obj.info("Parsed aguments:%s, configfile: %s, proc %s" % \
                    (acc_service_id, conf_obj, proc_name))

    #create communication object
    communicator_obj = Req(logger_obj)
    #create recovery obj and start recovery
    rec_data_obj = RecoveryData()
    retry = 0
    rec_obj = Recovery(conf_obj, logger_obj, acc_service_id, \
    communicator_obj, rec_data_obj)
    while retry < MAX_RETRY_COUNT:
        if rec_obj.connect_gl():
            logger_obj.info("Starting Recovery Process...")
            # create green thread for recovery instance
            recovery_gthread = eventlet.spawn(rec_obj.start_recovery)
            recovery_gthread.wait()
            #rec_obj.start_recovery()
            logger_obj.info("Recovery Process Finished")
            break
        retry += 1
        logger_obj.error("Failed to connect gl, Retry: %s" % retry)

