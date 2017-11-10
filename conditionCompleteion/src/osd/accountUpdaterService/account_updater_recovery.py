import argparse
import time
import httplib
import procname
import signal
import os
import sys
from Queue import Queue
from threading import Thread, Lock, Condition
from osd.accountUpdaterService.utils import SimpleLogger
from osd.glCommunicator.req import Req
from osd.glCommunicator.response import Resp
from osd.glCommunicator.communication.communicator import IoType
from osd.glCommunicator.communication.osd_exception \
                        import SocketTimeOutException
from osd.accountUpdaterService.config_variable \
                        import MAX_THREAD_COUNT, \
                        accountUpdater_script_prefix

class RecoveryData:
    '''Recovery resources data class '''
    def __init__(self):
        # map that keeps dest_service_obj and comp_list
        self._recovery_map = dict()
        self._map_lock = Lock()

    def get_recovery_map(self):
        self._map_lock.acquire()
        try:
            return self._recovery_map
        finally:
            self._map_lock.release()

    def set_recovery_map(self, recovery_dict):
        self._map_lock.acquire()
        try:
            self._recovery_map.update(recovery_dict)
        finally:
            self._map_lock.release()

class Response:

    def __init__(self, resp, dest_id):
        self.__resp = resp
        self.__dest_id = dest_id

    def is_success(self):
        if self.__resp.status == 200 or self.__resp.status == 100:
            return True
        else:
            return False


class Recovery:

    ''' AccountUpdater Updater server Recovery '''

    def __init__(self, conf, logger, source_service_id):
        self.__communicator_obj = Req(logger)
        self.__source_service_id = source_service_id
        self.__recovery_data_obj = RecoveryData()
        logger.info(" Req and Recovery Data object created ")
        self.__conf = conf
        self.__logger = logger
        self.__status_queue = Queue()
        self.__final_transferred_component_list = list()
        self.__final_status = True
        self.__network_fail_status_list = [Resp.BROKEN_PIPE, \
               Resp.CONNECTION_RESET_BY_PEER, \
               Resp.NETWORK_TIMEOUT, Resp.INVALID_SOCKET, \
               Resp.IO_ERROR, Resp.SOCKET_READ_ERROR, \
               Resp.SERVER_ERROR, Resp.CORRUPTED_HEADER, \
               Resp.CORRUPTED_MESSAGE, Resp.CORRUPTED_DATA, \
               Resp.INVALID_KEY, Resp.TIMEOUT]
        self.__success_status_list = [Resp.SUCCESS]
        #flags
        self.__strm_ack_flag = False
        self.__finish_recovery_flag = False
        # condition variables
        self.__strm_ack_cond = Condition()
        self.__finish_recovery_cond = Condition()
        #create con obi
        self.__logger.info(" Condition and Connection object created ")
        self.__con_obj = self.__create_con_obj()


    def __create_con_obj(self):
        self.__logger.info(" Into the create_con_obj ")
        gl_info_obj = self.__communicator_obj.get_gl_info() 
        retry_count = 0
        self.__con_obj = None
        while retry_count < 3:
            if not self.__con_obj:
                self.__con_obj = self.__communicator_obj.connector( \
                IoType.EVENTIO, gl_info_obj)
            if self.__con_obj:
                return self.__con_obj
            retry_count += 1
        self.__logger.error(" Failed to connect with Global Leader\
            after three retry... ")
        sys.exit(-1)

    def __reset_con_obj(self):
        self.__create_con_obj()

    def __getconnObject(self, dest_node_obj, accept_component_list):
        conn = httplib.HTTPConnection(dest_node_obj.get_ip(), \
                                      dest_node_obj.get_port())
        self.__logger.debug("HTTP Connection object is: %s" % conn)
        try:
            self.__logger.info("AccountUpdater recovery is %s get http \
                 connected  from destination id" % self.__source_service_id)
            headers = {}
            headers['Content-Length'] = len(str(accept_component_list))
            conn.request('ACCEPT_COMPONENT_TRANSFER', '', str(accept_component_list), headers)
        except Exception as err:
            for comp in accept_component_list:
                self.__final_transferred_component_list.append((comp, False))
            self.__final_status = False
            self.__logger.error("Request` from destination failed %s" %err)
            return ""
        return conn

    def __send_acceptComponent(self, dest_node_obj, accept_component_list):
        self.__logger.debug("Inside __send_acceptComponent() for %s, %s " \
	    % (dest_node_obj.get_id(), dest_node_obj.get_ip()))
        self.__logger.debug("Comp list is %s" % accept_component_list)
        conn = self.__getconnObject(dest_node_obj, accept_component_list)
        if not conn:
            self.__logger.error("connection failed, no need to do anything for this thread")
            return    #TODO: have to provide proper handling for this
        resp = conn.getresponse()
        self.__logger.debug("Resp is %s, *%s* " % (resp, resp.status))
        if resp:
            self.__logger.debug("Connection established with destination node \
                 %s" % dest_node_obj.get_ip())
        resp_obj = Response(resp, dest_node_obj)
        self.__status_queue.put(resp_obj)
        intermediate_list = []
        self.__logger.info("Sending intermediate response to GL from %s" \
	    % self.__source_service_id)
        try:
            if resp_obj.is_success():
                self.__logger.info("Response is success from dest node")
                for comp in accept_component_list:
                    intermediate_list.append((comp, dest_node_obj))
                self.__logger.debug("intermediate_list is %s" % intermediate_list)
                if self.__send_transfer_status(intermediate_list):
                    for comp in accept_component_list:
                        self.__final_transferred_component_list.append((comp, True))
                else:
                    self.__final_transferred_component_list.append((comp, False))
                    self.__final_status = False
                self.__logger.debug("Returning from __send_acceptComponent after \
                    success, final_transferred_component_list size is %s" % \
                    len(self.__final_transferred_component_list))
                conn.close()
                return
            else:
                self.__logger.info("Resp is failure from dest node")
                for comp in accept_component_list:
                    self.__final_transferred_component_list.append((comp, False))
                self.__final_status = False
                return
        except Exception as err:
            self.__logger.info("Exception raised in __send_acceptComponent: %s" % err)

    def start_recovery(self):
        # start heart beat thread
        heart_beat_thread = Thread(target = self.start_heart_beat, args = ())
        heart_beat_thread.start()
        # get conditional flag
        self.__strm_ack_cond.acquire()
        # wait until recovery_map not populated by heartbeat thread
        if not self.__strm_ack_flag:
            self.__logger.info(" Main thread waiting for the recovery map to \
                 populate ")
            self.__strm_ack_cond.wait()
        self.__strm_ack_cond.release()
        threadid_list = []
        thread_count = 0
        recovery_map = self.__recovery_data_obj.get_recovery_map()
        for dest_id, comp_list in recovery_map.iteritems():
            if thread_count <= MAX_THREAD_COUNT:
                thread_id = Thread(target = self.__send_acceptComponent, \
                                   args=(dest_id, comp_list))
                thread_id.start()
                threadid_list.append(thread_id)
                thread_count += 1
                if thread_count > MAX_THREAD_COUNT:
                    for thread in threadid_list:
                        if thread.is_alive():
                            thread.join()
                        thread_count = 0
        for thread in threadid_list:
            if thread.is_alive():
                thread.join()
                thread_count = 0
        self.__logger.info("All components transferred, now sending END mon")
        try:
            if self._send_finalTransfer_status( \
               self.__final_transferred_component_list, self.__final_status):
                self.__con_obj.close()
                self.__logger.debug("AccountUpdater server recovery server_id: %s \
                     Connection closed from Global Leader after final status" \
                          % self.__source_service_id)
            else:
                self.__logger.error("Final status sending failed to GL")

            # set the finish transfer flag
            self.__finish_recovery_flag = True
            self.__logger.info(" AccountUpdater server recovery server_id finished \
                 : %s " % self.__source_service_id)
            # join heartbeat thread for its completion
            heart_beat_thread.join()
            self.__logger.info("AccountUpdater server id : %s Recovery exiting" \
                          % self.__source_service_id)
        except Exception as err:
            self.__logger.error("Exception catched while calling send_final_transfer_status: %s" % err)

    def _send_finalTransfer_status(self, final_component_list, final_status):
        #Send Final Status of recovery
        self.__logger.debug("Sending End monitoring to GL: %s" % final_component_list)
        self.__logger.info("Sending End monitoring to GL: Final status %s" % final_status)
        retry_count = 3
        counter = 0
        while counter < retry_count:
            ret = self.__communicator_obj.comp_transfer_final_stat(\
                  self.__source_service_id, final_component_list, \
                          final_status, self.__con_obj)
            self.__logger.info("Status of End Mon: %s" % ret.status.get_status_code())
            if ret.status.get_status_code() in self.__success_status_list:
                self.__logger.info("AccountUpdater server recovery server_id \
                    : %s Final Component transfer Completed " \
                    % self.__source_service_id)
                return True
            elif ret.status.get_status_code() in \
                self.__network_fail_status_list:
                self.__logger.error("AccountUpdater server recovery server_id \
                    : %s, killing recovery process" \
                          % self.__source_service_id)
                os._exit(0)
            else:
                self.__logger.error("AccountUpdater server recovery server_id \
                     : %s Final Component transfer Failed, Retrying... " \
                          % self.__source_service_id)
                os._exit(0)
            counter += 1
        sys.exit()

    def __send_transfer_status(self, accepted_component_list):
        ''' Send Intermediate Status of AcceptComponen '''
        try:
            self.__logger.debug("inside __send_transfer_status()")
            retry_count = 3
            counter = 0
            while counter < retry_count:
                self.__logger.debug("Sending intermediate response to GL")
            
                statusObj = self.__communicator_obj.comp_transfer_info(
                                           self.__source_service_id,
                                           accepted_component_list,
                                           self.__con_obj)
                self.__logger.info("Response got from GL: %s" % statusObj.status.get_status_code())
                if statusObj.status.get_status_code() in \
                         self.__success_status_list:
                    self.__logger.info("AccountUpdater server recovery server_id: \
                        %s Component transfer Completed" \
                        % self.__source_service_id)
                    return True
                elif statusObj.status.get_status_code() in \
                        self.__network_fail_status_list:
                    self.__logger.error("AccountUpdater server recovery server_id: \
                        %s, Connection resetting" \
                        % self.__source_service_id)
                    os._exit(0)
                else:
                    self.__logger.error("AccountUpdater server recovery server_id: \
                        %s Component transfer Failed, killing Recovery-Process"\
                        % self.__source_service_id)
                    os._exit(0)
                counter += 1
        except Exception as ex:
            self.__logger.info("Exception raised while sending intermediate \
                resp: %s " % ex)
        sys.exit()

    def start_heart_beat(self):
        #Other Way: create health monitoring obj and start monitoring
        self.__logger.info(" Populating recovery_map ")
        try:
            ret = self.__communicator_obj.recv_pro_start_monitoring( \
                            self.__source_service_id, self.__con_obj)
        except Exception as ex:
            self.__logger.error("recv_pro_start_monitoring object \
                creation  %s" %(ex))
        self.__logger.info("Rcvd STRM ack from GL: %s" % ret.status.get_status_code())
        if ret.status.get_status_code() in self.__success_status_list:
            self.__logger.info("Success rcvd from RSTRM")
            self.__strm_ack_cond.acquire()
            self.__recovery_data_obj.set_recovery_map( \
                                     ret.message.dest_comp_map())
            # set flag, main thread will read this flag
            self.__strm_ack_flag = True
            self.__strm_ack_cond.notify()
            self.__strm_ack_cond.release()
            self.__logger.info("Populated recovery map and \
                          Notified STRM ack cond and Released STRM ack cond")
        else:
            self.__logger.info("Failed status from communication object\
                Exiting..... account-updater Recovery process")
            os._exit(0)
        sequence_counter = 1
        #start heart beat
        self.__logger.info("########self.__finish_recovery_flag: %s" % self.__finish_recovery_flag)
        while not self.__finish_recovery_flag:
            self.__logger.info("Sending heartbeat##########")
            ret = self.__communicator_obj.send_heartbeat( \
                            self.__source_service_id, \
                            sequence_counter, self.__con_obj)
            if ret.status.get_status_code() in self.__network_fail_status_list:
                self.__logger.error("Heartbeat sending failed :::::::::: %s" % ret.status.get_status_code())
                self.__reset_con_obj()
            else:
                self.__logger.info("Heartbeat Sent#########")
            sequence_counter += 1
            time.sleep(3)

if __name__ == "__main__":
    config = {}
    logger_obj = SimpleLogger(config).get_logger_object()
    logger_obj.info("AU Recovery logger Object created")
    accountupdater_script_prefix = accountUpdater_script_prefix
    parser = argparse.ArgumentParser(description= \
             "Recovery process option parser")
    parser.add_argument("server_id", help = "Service id from Global leader.")
    args = parser.parse_args()
    logger_obj.debug("Recovery: nodeId argument parsed")
    def kill_children(signum, stack):
        logger_obj.info("recieved signal : %s" %signum)
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        os.killpg(0, signal.SIGTERM)
        sys.exit(130)
    signal.signal(signal.SIGTERM, kill_children)
    #Process Name chaging
    if args.server_id:
        accupdater_server_id = args.server_id
        process_name = accountupdater_script_prefix + accupdater_server_id
        procname.setprocname(process_name)
    else:
        logger_obj.debug("node id parameter missing")
    logger_obj.debug("Creating AU Recovery object")
    rec_obj = Recovery(config, logger_obj, accupdater_server_id)
    logger_obj.info("AU Recovery object created and going to start")
    # Start Recovery
    rec_obj.start_recovery()

