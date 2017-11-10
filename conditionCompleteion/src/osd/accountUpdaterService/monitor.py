import os
import sys
import time
import threading
import Queue
from osd.glCommunicator.req import Req
from osd.glCommunicator.response import Resp
from osd.glCommunicator.service_info import ServiceInfo
from osd.glCommunicator.communication.communicator import IoType

class SingletonType(type):
    """ Class to create singleton classes object"""
    __instance = None

    def __call__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(SingletonType, cls).__call__(*args, **kwargs)
        return cls.__instance

class WalkerMap(list):
    def __init__(self):
        super(WalkerMap, self).__init__()
        self._lock = threading.Lock()

    def getEntries(self):
        return self.__iter__() or []

    def __setitem__(self, index, value):
        self._lock.acquire()
        try:
            super(WalkerMap, self).__setitem__(index,value)
        finally:
            self._lock.release()

    def __getitem__(self, index):
        self._lock.acquire()
        try:
           item = super(WalkerMap, self).__getitem__(index)
        finally:
            self._lock.release()
        return item

class StatFile(object):
    def __init__(self, stat_file):
        self._stat_file = stat_file
        super(StatFile, self).__init__()
        self._lock_flag = False

    def get_stat_file(self):
        return self._stat_file

    def set_lock_flag(self, value):
        self._lock_flag = value

    def get_lock_flag(self):
        return self._lock_flag

class ReaderMap(dict):
    #Lock based dict
    def __init__(self):
        super(ReaderMap,self).__init__()
        self._lock = threading.Lock()
        self.__containerCount = 0

    def getContainerCount(self):
        return self.__containerCount

    def incrementContainer(self):
        self.__containerCount += 1

    def decrementContainerCount(self, count=1):
        self.__containerCount -= count

    def __setitem__(self, key, value):
        #Create or update the dictionary entry for a Reader
        self._lock.acquire()
        try:
            super(ReaderMap,self).__setitem__(key, value)
        finally:
            self._lock.release()

    def __getitem__(self, key):
        self._lock.acquire()
        try:
            item = super(ReaderMap, self).__getitem__(key)
        finally:
            self._lock.release()
        return item

    def keys(self):
        self._lock.acquire()
        try:
            key_list = super(ReaderMap, self).keys()
        finally:
            self._lock.release()
        return key_list

class SweeperQueueList(list):

    def __init__(self):
        super(SweeperQueueList,self).__init__()
        self._lock = threading.Lock()

    def __setitem__(self, index, value):
        self._lock.acquire()
        try:
            super(SweeperQueueList,self).__setitem__(index, value)
        finally:
            self._lock.release()

    def __getitem__(self):
        self._lock.acquire()
        try:
           item = super(SweeperQueueList, self).__getitem__()
        finally:
            self._lock.release()
        return item

    def __delsice__(self):
        self._lock.acquire()
        try:          
            super(SweeperQueueList, self).__delslice__()
        finally:
            self._lock.release()


class GlobalVariables:
    __metaclass__ = SingletonType
    def __init__(self, logger):
        self.logger = logger
        self.conn_obj = None
        self.__acc_updater_map_version = 0.0
        self.__acc_map_version = 0.0
        self.__cont_map_version = 0.0
        self.__global_map_version = 0.0
        self.__global_map = {}
        self.__acc_updater_map = []
        self.__acc_map = []
        self.__cont_map = []
        self.__ownership_list = []
        self.__get_comp_counter_queue = Queue.Queue()
        self.__container_sweeper_list = []
        self.__condition = threading.Condition()
        self.__transfer_comp_list = []
        #self.__getcmpmap = threading.Event()
        self.__complete_all_event = threading.Event()
        self.__transfer_cmp_event  = threading.Event()
        self.__accept_cmp_event = threading.Event()
        self.request_handler = Req(self.logger)

    def put_into_Queue(self):
        self.__get_comp_counter_queue.put(True)

    def get_from_Queue(self):
        try:
            comp_counter = self.__get_comp_counter_queue.get(block=True, \
                timeout=300)
            return comp_counter
        except Queue.Empty:
            self.logger.error("Queue is empty")
            raise Queue.Empty("Nothing in queue")
     
    def load_ownership(self):
        self.logger.info("Loading my ownership")
        retry = 3
        counter = 0
        self.ownership_status = False
        while counter < retry:
            counter += 1
            try:
                self.gl_info = self.request_handler.get_gl_info()
                self.conn_obj = self.request_handler.connector(\
                                IoType.SOCKETIO, self.gl_info)
                if self.conn_obj != None:
                    result = self.request_handler.get_comp_list(\
                        self.__service_id, self.conn_obj)
                    if result.status.get_status_code() == Resp.SUCCESS:
                        self.__ownership_list = result.message
                        self.logger.debug("Ownership list:%s" %self.__ownership_list)
                        self.ownership_status = True
                        self.conn_obj.close()
                        return
                    else:
                        self.logger.warning("get_comp_list() error code:%s msg:%s"\
                            % (result.status.get_status_code(), \
                            result.status.get_status_message()))
                        self.conn_obj.close()
            except Exception as ex:
                self.logger.exception("Exception occured during load_ownership"
                    ":%s" %ex)
                if self.conn_obj:
                    self.conn_obj.close()
            self.logger.warning("Failed to get comp list, Retry: %s" % counter)
            time.sleep(10)

    def get_my_ownership(self):
        self.logger.info("Sending request for retrieving my ownership")
        self.load_ownership()
        if not self.ownership_status:
            self.logger.error("Get my ownership list failed")
            os._exit(130)
        while not self.__ownership_list:
            self.logger.debug("Ownership list is empty, retry after 20 seconds ")
            time.sleep(20)
            self.load_ownership()

    def set_ownershipList(self, ownership_list):
        self.__ownership_list = ownership_list
 
    def get_ownershipList(self):
        return self.__ownership_list

    def load_gl_map(self):
        self.logger.info("Sending request for retrieving global map")
        retry = 3
        counter = 0
        while counter < retry:
            counter += 1
            try:
                self.gl_info = self.request_handler.get_gl_info()
                self.conn_obj = self.request_handler.connector(IoType.SOCKETIO, self.gl_info)
                if self.conn_obj != None:
                    result = self.request_handler.get_global_map(self.__service_id, self.conn_obj)
                    if result.status.get_status_code() == Resp.SUCCESS:
                        self.__global_map = result.message.map()
                        self.__global_map_version = result.message.version()
                        self.logger.info("Global map version: %s" 
                            %(self.__global_map_version))
                        self.__acc_updater_map = self.__global_map['accountUpdater-server'][0]
                        self.__acc_updater_map_version = self.__global_map['accountUpdater-server'][1]
                        self.__acc_map = self.__global_map['account-server'][0]
                        self.__acc_map_version = self.__global_map['account-server'][1]
                        self.__cont_map = self.__global_map['container-server'][0]
                        self.__cont_map_version = self.__global_map['container-server'][1]
                        self.conn_obj.close()
                        return True 
                    else:
                        self.logger.warning("load_gl_map() error code: %s msg: %s" \
                            % (result.status.get_status_code(), \
                            result.status.get_status_message()))
                        self.conn_obj.close()
            except Exception as ex:
                self.logger.exception("Exception occured during get_global_map"
                    ":%s" %ex)
                if self.conn_obj:
                    self.conn_obj.close()
            self.logger.warning("Failed to get global map, Retry: %s" % counter)
        self.logger.error("Failed to load global map")
        return False

    def get_account_map(self):
        while not self.__acc_map:
            self.load_gl_map()
            time.sleep(10)
        return self.__acc_map

    def get_container_map(self):
        while not self.__cont_map:
            self.load_gl_map()
            time.sleep(10)
        return self.__cont_map

    def get_acc_updater_map(self):
        while not self.__acc_updater_map:
            self.load_gl_map()
            time.sleep(10)
        return self.__acc_updater_map

    def get_acc_updater_map_version(self):
        return self.__acc_updater_map_version

    def get_global_map_version(self):
        return self.__global_map_version

    def set_service_id(self, service_id):
        self.__service_id = service_id

    def get_service_id(self):
        return self.__service_id

    def create_sweeper_list(self, comp_tuple):
        self.__container_sweeper_list.append(comp_tuple)

    def get_sweeper_list(self):
        return self.__container_sweeper_list

    def pop_from_sweeper_list(self, temp_list):
        self.__container_sweeper_list = temp_list

    def get_conditionVariable(self):
        return self.__condition

    def set_transferCompList(self, comp_list):
        self.__transfer_comp_list = comp_list

    def get_transferCompList(self):
        return self.__transfer_comp_list

    """
    def get_componentGetEventObject(self):
        return self.__getcmpmap
    """
    def get_complete_all_event(self):
        return self.__complete_all_event

    def get_accept_cmp_event(self):
        return self.__accept_cmp_event

    def get_transfer_cmp_event(self):
        return self.__transfer_cmp_event
