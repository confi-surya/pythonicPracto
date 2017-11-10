# Copyright (c) 2010-2012 OpenStack Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

""" Container controller class"""
import pickle
import os
import sys
import time
import traceback
import signal
import socket
from time import sleep
import cPickle as pickle

from eventlet.queue import Queue as queue
from eventlet.greenthread import spawn as free_lock_container
#from Queue import Queue as queue
from eventlet import Timeout, GreenPile
from osd.common.request_helpers import get_param, \
    split_and_validate_path, is_sys_or_user_meta, \
    get_listing_content_type, is_param_exist
from osd.common.utils import get_logger, public, normalize_timestamp, \
    config_true_value, timing_stats, create_recovery_file, \
    remove_recovery_file, get_global_map, ismount, float_comp
from osd.common.utils import ContextPool , GreenthreadSafeIterator, \
    GreenAsyncPile, RECOVERY_FILE_PATH
from osd.common.exceptions import ChunkReadTimeout, \
    ChunkWriteTimeout, ConnectionTimeout
from osd.common.ring.ring import hash_path
from osd.common.constraints import CONTAINER_LISTING_LIMIT, \
    check_mount, check_float, check_utf8
from osd.common.bufferedhttp import http_connect
from osd.common.exceptions import ConnectionTimeout
from osd.common.http import HTTP_NOT_FOUND, HTTP_INTERNAL_SERVER_ERROR, \
    is_success, HTTP_CONFLICT, HTTP_FORBIDDEN, HTTP_NOT_ACCEPTABLE
from osd.common.swob import HTTPAccepted, HTTPBadRequest, HTTPConflict, \
    HTTPCreated, HTTPInternalServerError, HTTPNoContent, HTTPNotFound, \
    HTTPPreconditionFailed, HTTPMethodNotAllowed, Request, Response, HTTPForbidden, \
    HTTPInsufficientStorage, HTTPException, HTTPServiceUnavailable, HeaderKeyDict, \
    HTTPOk, HTTPTemporaryRedirect, HTTPMovedPermanently
from osd.common.http import is_success, HTTP_CONTINUE, \
    HTTP_INTERNAL_SERVER_ERROR, HTTP_INSUFFICIENT_STORAGE
from osd.containerService.container_operations import ContainerOperationManager
from osd.containerService.transcation_handler import TransactionManager
from osd.containerService.utils import SingletonType, OsdExceptionCode, \
    FILENAME, get_filesystem, releaseStdIOs, isFile, FILE_EXISTS 
from osd.containerService.recovery_handler import container_recovery, \
    recover_object_expired_id, process_bulk_delete_transctions, \
    recover_conflict_expired_id
from osd.common.bulk_delete import ACCEPTABLE_FORMATS, bulk_delete_response
from osd.glCommunicator.messages import TransferCompResponseMessage
from osd.glCommunicator.req import Req, TimeoutError
from osd.glCommunicator.messages import TransferCompMessage
from libHealthMonitoringLib import healthMonitoring
import libraryUtils
from osd.glCommunicator.message_type_enum_pb2 import *
import subprocess
import osd_utils
from osd import gettext_ as _
import libContainerLib
from hashlib import md5
from osd.glCommunicator.communication.communicator import IoType
from osd.glCommunicator.response import Resp
import eventlet
from urllib import quote
REQ_BODY_CHUNK_SIZE = 65536

CHILD_PID_FILE = 'container-server.pid'

class RecoveryStatus(object):
    """ Class to get recovery status """
    def __init__(self):
        self.status = False
    def get_status(self):
        """ Getter function for recovery status """
        return self.status
    def set_status(self, status):
        """ Setter function for recovery status """
        self.status = status

class ContainerController(object):
    """WSGI Controller for the container server."""

    __metaclass__ = SingletonType
    global_map = {}

    def __init__(self, conf, logger=None):
        self.logger = logger or get_logger(conf, log_route='container-server')
        self.con_op = None
        libraryUtils.OSDLoggerImpl("container-library").initialize_logger()
        self.log_requests = config_true_value \
            (conf.get('log_requests', 'true'))

        self.__clean_journal = (False, True)[conf.get("clean_journal") == "True"]
        self.__port = int(conf.get('bind_port', 61007))
        self.__ll_port = int(conf.get('llport', 61014))
        self.__trans_port = int(conf['child_port'])
        hostname = socket.gethostname()
        self.local_leader_id = hostname + "_" + str(self.__ll_port)
        self.__service_id = hostname + "_" + str(self.__ll_port) + "_container-server"
        self.__child_service_id = hostname + "_" + str(self.__ll_port) + "_containerChild-server"
        self.conf = conf
        self.__root = conf.get('filesystems', '/export')
        self.__port = int(conf.get('bind_port', 61007))
        self.__cont_journal_path = \
            conf.get('journal_path_cont', os.path.join(self.__root, '.' + self.local_leader_id + '_container_journal'))
        self.__trans_journal_path = \
            conf.get('journal_path_trans', os.path.join(self.__root, '.' + self.local_leader_id + '_transaction_journal'))

        #Create Node ID from service ID
        self.__node_id = self.__create_node_id(self.local_leader_id)
        self.my_ip = self.__get_node_ip(hostname)
        self.__pid_path = conf.get('PID_PATH','/var/run/osd/children')


        self.__mount_check = config_true_value(conf.get('mount_check', 'true'))
        self.__conn_timeout = float(conf.get('conn_timeout', 0.5))
        self.__node_timeout = int(conf.get('node_timeout', 3))
        self.__conflict_timeout = int(conf.get('recover_conflict_timeout', 400))
    
        #Component Request  Dictionary
        self.comp_request_dict = {}

        #Ask Global Leader for my ownership list
        self.logger.info("request for my_components")
        try:
            with Timeout(self.__node_timeout):
                self.__my_components = self.get_my_ownership()
        except (Timeout, TimeoutError) as err:
            self.logger.error("Exiting :Timeout in get_my_ownership %s" % err)
            sys.exit(130)
        except Exception as err:
            self.logger.error(err)
            sys.exit(130)

        self.logger.info("my_components received %s "% self.__my_components)

        c_status = 0
        t_status = 0
        if not ismount(self.__cont_journal_path):
            c_status = osd_utils.mount_filesystem('.' + self.local_leader_id + \
                "_container_journal", ['-O'], str(hostname))
        if not ismount(self.__trans_journal_path):
            t_status = osd_utils.mount_filesystem('.' + self.local_leader_id + \
                "_transaction_journal", ['-O'], str(hostname))

        # Check if --clean_journal option is present on not
        # if --clean_journal option is present than clean all the journal files and 
        # create a blank journal file with a new name:
        if self.__clean_journal:
            self.clean_journals()

        # create instances of other components
        self.con_op = ContainerOperationManager(self.__root, \
            self.__cont_journal_path, self.__node_id, self.logger)

        # create process for expired transaction IDs
        self.create_process()

        if not (c_status == 0 and t_status == 0):
            self.logger.error("Containerjournal mount status: %s, \
                              Transaction-journal mount status: %s" \
                              % (c_status, t_status))
            sys.exit(130)

        self.logger.info("Containerjournal mount status: %s , \
                          Transactionjournal mount status:%s" \
                          % (c_status, t_status ))
        #Component Number needs to  be blocked:
        self.__blocked_component_list = []

        #Component needs to be blocked while accept_component_transfer
        self.__accept_comp_block_list = []

        #Request for Global Map
        self.logger.info("Global Map request")
        self.global_map, self.global_map_version, self.conn_obj = get_global_map(self.__service_id, self.logger)
        self.logger.debug("global_map : %s %s %s" % (self.global_map, self.global_map_version, self.conn_obj))


        #Current Major Version of map
        self.global_map_major_version = self.get_major_version(self.global_map_version)

        self.trans_mgr = TransactionManager(self.__trans_journal_path, self.__node_id, self.__trans_port, self.logger)
        #create internal memory
        self.__active_trans_dict = {}
        #start recovery of libraries and service
        if 'recovery_status' in conf:
            recovery_status = conf['recovery_status'][0].get_status()
            if not recovery_status:
                ret = self.__start_recovery(self.__my_components)
                if not ret:
                    print "Container Service Recovery Failed."
                    sys.exit(130)
                conf['recovery_status'][0].set_status(True)

        #for object versioning 
        self._object_subversion_y = 0
        #get object major version from GL
        self._object_major_version_x = self.get_object_gl_version()

        # Start sending health to local leader
        self.logger.debug("Health monitoring Starting")
        self.health_instance = healthMonitoring(self.my_ip, self.__port, self.__ll_port, self.__service_id)
        self.logger.debug("Health monitoring Started")

	#Removing recovery file after loading Health Monitoring Library
        self.logger.debug("Removing recovery file")
        remove_recovery_file('container-server')

    def clean_journals(self):
        """
	Function to clean journal files and create a new blank journal file
        """
        journal_files = os.listdir(self.__cont_journal_path)
        tmp_id = 0
	#Finding the latest journal id
        for file in journal_files:
            try:
                if tmp_id < int(file.split('.')[2]):
                    tmp_id = int(file.split('.')[2])
            except Exception as ex:
                self.logger.error("un wanted file found: %s" %file)
        tmp_id += 1
        self.logger.info("New journal file ID is %s" %tmp_id)
	#Creating a new journal file with an increamental id
        new_journal_file = self.__cont_journal_path + "/" + "journal.log." + str(tmp_id)
        os.system('touch %s' % new_journal_file)

	#removing old journals
        for file in journal_files:
            os.system('rm -f %s' %(self.__cont_journal_path + "/" + file))

    def __addition_in_comp_request_dict(self, comp):
        """
        Function to increase comp request map
	"""
        try:
            if comp not in self.comp_request_dict.keys():
                self.comp_request_dict[comp] = 1
            else:
                self.comp_request_dict[comp] = self.comp_request_dict[comp] + 1
        except:
            self.logger.error("Could not add in comp_request_dict ")

    def __deletion_in_comp_request_dict(self, comp):
        """
        Function to decrease comp request map
        """
        try:
            if comp in self.comp_request_dict.keys():
                self.comp_request_dict[comp] = self.comp_request_dict[comp] - 1
            if self.comp_request_dict[comp] == 0:
                del self.comp_request_dict[comp]
        except:
            self.logger.error("Could not delete from comp_request_dict ")

    def __search_in_comp_request_dict(self, comp_list):
        """
        Function to search in comp request map
        """
        ret = False
        for comp in comp_list:
            if self.comp_request_dict.has_key(comp):
                ret = True
                break
        return ret

    def __process_bulk_delete_request(self, sock):
        """
       Function to process the bulk delete records
       """
        EMPTY_DATA_LENGTH = 8
        try:
            total_data = ''
            counter = 0
            length = 0
            while 1:
                self.logger.debug('Start getting data from socket!')
                data = sock.recv(1024)
                self.logger.debug('End getting data from socket!')
                if counter == 0:
                    length = int(data.split('$$$$')[0])
                    counter = 1
                    if length == EMPTY_DATA_LENGTH:
                        self.logger.info('There is no transction for BULK DELETE'
                            ' so break and continue')
                        break
                total_data += data
                length -= len(data)
                if length <= 0:
                    self.logger.debug(('Length %(len)s is less than equal to zero'
                        ' so exit!'), {'len' : length})
                    break
            self.logger.debug('Close the open socket!')
            sock.close()
            self.logger.debug('Socket closed!')
            req_flag = True
            time_ = 10
            gl_version = get_global_map(self.__service_id, self.logger, req_flag, time_)[1]
            if length != EMPTY_DATA_LENGTH:
                total_data = total_data.split('$$$$')[1:]
                obj_list = []
                for obj in total_data:
                    obj_list = obj_list + obj.split(':')
                self.logger.debug('Calling BULK DELETE interface of object service : %s' %total_data)
                process_bulk_delete_transctions(obj_list, self.__conn_timeout, \
                self.__node_timeout, gl_version, self.__ll_port, self.logger)
                self.logger.debug('BULK DELETE object interface call finished')
        except ValueError as err:
            self.logger.error("Global map could not be loaded, exiting now")
            self.logger.exception(err)
        except Exception as err:
            self.logger.exception(err)
            raise err

    def get_major_version(self, version):
        tmp = str(version).split('.')
        version = tmp[0]
        return int(version)

    def get_object_gl_version(self):
        gl_comm = Req(self.logger)
        gl_info_obj = gl_comm.get_gl_info()
        #Making Connection object for the first time.
        retry_count = 0
        while retry_count < 3:
            retry_count += 1
            communicator = gl_comm.connector(IoType.EVENTIO, gl_info_obj)
            if communicator:
                ret =  gl_comm.get_object_version(self.__service_id, communicator)
                if ret.status.get_status_code() == Resp.SUCCESS:
                    return ret.message.get_version()
                else:
                    self.logger.warn("COMM fail with, status code: %s, msg: %s"
                    %(ret.status.get_status_code(), ret.status.get_status_message()))
            else:
                self.logger.warn("Failed to connect with: %s" % gl_info_obj)
            self.logger.warn("Retry count: %s" % retry_count)

    def __get_object_version_id(self):
        """
        combine x & y version and return X.Y
        """
        return str(self._object_major_version_x) + '.' + \
                            str(self._object_subversion_y)

    def __create_node_id(self, local_leader_id):
        """
        Function to Create Node ID from Service ID
        """
        hash_ll = md5(local_leader_id)
        node_id = int(hash_ll.hexdigest()[:8], 16) % 256
        self.logger.info("Node ID for this set of services is %r"% node_id)
        return node_id


    def __get_node_ip(self, hostname):
        """
        Get internal node ip on which 
        service is running
        """
        try:
            command = "grep -w " + hostname + " /etc/hosts | awk {'print $1'}"
            child = subprocess.Popen(command, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = True)
            std_out, std_err = child.communicate()
            return std_out.strip()
        except Exception as err:
            self.logger.info("Error occurred while getting ip of node")
            self.logger.error(err)
            return ""


    def get_my_ownership(self):
        #Create the Object of Req class of gl communication module
        gl_comm = Req(self.logger)

        retry_count = 0
        while retry_count < 3:
            retry_count += 1
            gl_info_obj = gl_comm.get_gl_info()
            communicator = gl_comm.connector(IoType.EVENTIO, gl_info_obj)
            if communicator:
                self.logger.debug("CONN object success")
                ret =  gl_comm.get_comp_list(self.__service_id, communicator)
                if ret.status.get_status_code() != 0 :
                    eventlet.sleep(self.__conn_timeout)
                    ret = self.get_my_ownership()
                self.logger.info("Ownership from Gl %s" % ret.message)
                component_list = libContainerLib.list_less__component_names__greater_()
                for comp in ret.message:
                    component_list.append(comp)
                return component_list	 
            self.logger.debug("conn obj:%s, retry_count:%s " % (communicator, retry_count))
            eventlet.sleep(self.__conn_timeout)
        self.logger.error("FAILED to  get conn obj")
        raise


    def get_object_map_from_global_map(self):
        self.logger.debug("In get_object_map_from_gl")
        object_map = {}
        service_objects = self.global_map["object-server"][0]
        for obj in service_objects:
            service_id = obj.get_id()
            ip = obj.get_ip()
            port = obj.get_port()
            object_map[service_id] = [ip, port]
        self.logger.debug("object_map: %s" % object_map)
        return object_map




    def __check_block_status(self, component_name):
        self.logger.debug("Checking Blocked status for component %s of type %s" %\
                           (component_name, type(component_name)))
        #converting to int as component name is received as a string.
        if int(component_name) in self.__blocked_component_list:
            self.logger.debug("Component %s is in transfer block list" %component_name)
            return True
        elif int(component_name) in self.__accept_comp_block_list:
            self.logger.debug("Component %s is in accept block list" %component_name)
            return True
        else:
            self.logger.debug("Component %s is not in my block list" %component_name)
            return False

    def __clean_block_component_list(self):
        self.logger.info("Cleaning blocked component list")
        self.__blocked_component_list = []

    def __check_map_version(self, map_version):
        self.logger.info("GL Map version in CS: %s, in req: %s" \
        % (self.global_map_version, map_version))
        map_diff = float_comp(self.global_map_version, map_version)
        #Check map version
        if map_diff > 0:
            return 1
        if map_diff < 0:
            return 2
        return 0

    def __get_my_components_from_map(self):
        #Provide list of components from the map
        self.logger.info("Started __get_my_components_from_map")
        service_obj_list = []
        component_list = libContainerLib.list_less__component_names__greater_()
        service_obj_list, version = self.global_map["container-server"]
        for i in range(0, 512):
            if service_obj_list[i].get_id() == self.__service_id:
                component_list.append(i)
                self.logger.debug("My Component : %s" %i)
        self.logger.debug("__get_my_components_from_map Component list : %s"% component_list)
        return component_list

    def add_green_thread(self):
        self.con_op.start_event_wait_func()

    def add_process(self):
        # create process for expired transaction IDs
        #self.create_process()
        return True

    def __del__(self):
        if self.con_op:
            self.con_op.stop_event_wait_func()

    def __check_filesystem_mount(self):
        """
        Check mount of all OSD filesystems
        before recovery
        """
        filesystem_list = get_filesystem(self.logger)
        if self.__mount_check:
            for file_system in filesystem_list:
                if not check_mount(self.__root, file_system):
                    self.logger.info(('mount check failed '
                        'for filesystem %(file)s'), {'file' : file_system})
                    return False
            if not check_mount(self.__root, '.' + self.local_leader_id + '_transaction_journal'):
                self.logger.info(('mount check failed '
                    'for filesystem %(file)s'), 
                    {'file' : self.__trans_journal_path})
                return False
            if not check_mount(self.__root, '.' + self.local_leader_id + '_container_journal'):
                self.logger.info(('mount check failed '
                    'for filesystem %(file)s'), 
                    {'file' : self.__cont_journal_path})
                return False
        return True

    def __start_recovery(self, my_components):
        """
        Initiates recovery for container, transaction libraries
        and container service
        """
        try:
            create_recovery_file('container-server')
            ret = self.__check_filesystem_mount()
            if not ret:
                self.logger.error('Mount check before recovery failed!')
                remove_recovery_file('container-server')
                return ret
            ret = self.con_op.start_recovery_cont_lib(my_components)
            if not ret:
                self.logger.error(('Recovery of container library failed'
                    ' with status %(ret)s'),
                    {'ret': ret} )
                remove_recovery_file('container-server')
                return ret
            ret = self.trans_mgr.start_recovery_trans_lib(my_components)
            if not ret:
                self.logger.error(('Recovery of transaction library failed'
                    ' with status %(ret)s'),
                    {'ret': ret} )
                remove_recovery_file('container-server')
                return ret
            ret = self.__start_container_recovery(my_components)
            if not ret:
                self.logger.error(('Recovery of container service failed'
                    ' with status %(ret)s'),
                    {'ret': ret})
                remove_recovery_file('container-server')
            self.logger.debug('Recovery completed!')
            return ret
        except Exception as err:
            remove_recovery_file('container-server')
            self.logger.error((
                'ERROR Could not complete recovery process for '
                'container service'
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack())})
            return False

    def __update_internal_memory(self, path):
        """
        Utility function to update internal memory
        """
        _, container, _ = path.split('/')
        if container in self.__active_trans_dict:
            value = self.__active_trans_dict[container]
            value[0] += 1
        else:
            self.__active_trans_dict.update({container : [1, False]})

    def __recover_internal_memory(self):
        """
        create memory and generate expired transaction IDs list
        """
        try:
            internal_map = self.trans_mgr.get_list()
            for key in internal_map:
                path = internal_map[key].get_object_name()
                self.__update_internal_memory(path)
            return True
        except Exception as err:
            self.logger.error('Problem in making container memory')
            self.logger.exception(err)
            return False

    def __start_container_recovery(self, my_components):
        """
        Start startup recovery of container service
        """
        try:
            self.logger.debug('Container service recovery started!')
            ret_rec = container_recovery(self.__root, self.logger, my_components)
            ret_mem = self.__recover_internal_memory()
            self.logger.debug('End of container service recovery!')
            if (ret_rec and ret_mem) == True:
                return True
            return False
        except Exception as err:
            self.logger.exception(err)
            return False

    def __close_socket(self):
        """
        Close socket of child process
        """
        try:
            pid = os.getpid()
            os.system("/usr/sbin/lsof -p %s | grep %s |grep LISTEN > %s" %(pid, self.__port, FILENAME))
            #os.system("/usr/sbin/lsof -p %s | grep containerLibrary.log >> %s" %(pid, FILENAME))
            with open(FILENAME, "r") as fd:
                data = fd.readlines()
                for sockLine in data:
                    sock = sockLine.split()[3][:-1]
                    os.close(int(sock))
            self.logger.debug('Socket closed!')
            # Remove redundant file
            try:
                os.remove(FILENAME)
            except (Exception, OSError) as err:
                self.logger.error(('Error in deleting socket file: '
                    '%(file)s with error %(error)s'), 
                    {'file' : FILENAME, 'error' : err})
        except Exception as err:
            raise err

    def __free_conflict_lock(self, req, trans_list):
        """
        Function to submit free lock 
	request to object service for conflict lock
        """
        try:
            #TODO:a library interface for fetching the list of trans id would be needed here
            #TODO:List format:trans_list = [(trans_id, full_path, obj_server_id, operation),..]
            self.logger.info('free lock conflict called')
            self.global_map, self.global_map_version, self.conn_obj = get_global_map(self.__service_id, self.logger)
            object_map = self.get_object_map_from_global_map()
            if object_map:
                #self.logger.debug('Calling recover with data :%s object map : %s' % (total_data, object_map))
                recover_conflict_expired_id(req, trans_list, self.__conn_timeout, \
                    self.__node_timeout, self.logger, object_map, self.__service_id, self.global_map_version)
                self.logger.debug('Recover object interface call finished')
            else:
                self.logger.debug('Object Map received Empty from GL')
        except Exception as err:
            self.logger.exception(err)

    def __get_data_recover(self, sock):
        """
        Function to get data from socket and
        sent to object service for recovery
        """
        EMPTY_DATA_LENGTH = 8
        try:
            total_data = ''
            counter = 0
            length = 0
            while 1:
                self.logger.debug('Start getting data from socket!')
                data = sock.recv(1024)
                self.logger.debug('End getting data from socket!')
                if counter == 0:
                    length = int(data.split('$$$$')[0])
                    counter = 1
                    if length == EMPTY_DATA_LENGTH:
                        self.logger.info('There is no transaction ID'
                            ' so break and continue')
                        break
                total_data += data 
                length -= len(data)
                if length <= 0:
                    self.logger.debug(('Length %(len)s is less than equal to zero'
                        ' so exit!'), {'len' : length}) 
                    break
            self.logger.debug('Close the open socket!')
            sock.close()
            self.logger.debug('Socket closed!')
            if length != EMPTY_DATA_LENGTH:
                total_data = total_data.split('$$$$')[1:]
                req_flag = True
                time_ = 10
                self.global_map, self.global_map_version, self.conn_obj = get_global_map(self.__service_id, self.logger, req_flag, time_)
                object_map = self.get_object_map_from_global_map()
                if object_map:
                    self.logger.debug('Calling recover with data :%s object map : %s' % (total_data, object_map))
                    recover_object_expired_id(total_data, self.__conn_timeout, \
                        self.__node_timeout, self.logger, object_map, self.__service_id, self.global_map_version)
                    self.logger.debug('Recover object interface call finished')
                else:
                    self.logger.debug('Object Map received Empty from GL')
        except ValueError as err:
            self.logger.error("Global map could not be loaded, exiting now")
            self.logger.exception(err)
        except Exception as err:
            self.logger.exception(err)
            raise err

    def __start_process(self):
        """
        Start a new process to recover expired transaction
        IDs of object
        """
        try:
            releaseStdIOs(self.logger)
            self.__close_socket()
            libraryUtils.OSDLoggerImpl("container-library").logger_reset()
            libraryUtils.OSDLoggerImpl("containerchildserver-library").initialize_logger()
            msg_type = {'BULK_DELETE':'1', 'EXPIRED_IDS':'2'}
            while isFile(RECOVERY_FILE_PATH + '/', CHILD_PID_FILE) == FILE_EXISTS:
                self.logger.debug("recovery is still going on parent process. sleep for 5 secs")
                time.sleep(5)
            # Start sending health to local leader
            self.logger.debug("Health monitoring Started")
            #child_id = self.__service_id+"-child"
            self.health_instance = healthMonitoring(self.my_ip, self.__port, self.__ll_port, self.__child_service_id)
            self.logger.debug("Health monitoring completed, id: %s" % self.__child_service_id)
            while True:
                connected = False
                self.logger.info('Start Bulk Delete object list!')
                try:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    host = socket.gethostname()
                    try:
                        self.logger.debug('Connect to transaction library')
                        sock.connect((host, self.__trans_port))
                        self.logger.debug('Connected to library')
                        connected = True
                    except Exception as err:
                        self.logger.exception(err)
                    if connected:
                        sock.send(msg_type['BULK_DELETE'])
                        self.logger.info('Start processing bulk delete object list')
                        self.__process_bulk_delete_request(sock)
                        self.logger.info('Finish processing bulk delete object list')
                    else:
                        self.logger.info('Problem in connecting to library')
                    connected = False
                    self.logger.info('Start getting expired id list')
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    host = socket.gethostname()
                    try:
                        self.logger.debug('Connect to transaction library')
                        sock.connect((host, self.__trans_port))
                        self.logger.debug('Connected to library')
                        connected = True
                    except Exception as err:
                        self.logger.exception(err)
                    if connected:
                        sock.send(msg_type['EXPIRED_IDS'])
                        self.logger.info('Start Recovery of expired ids')
                        self.__get_data_recover(sock)
                        self.logger.info('Finish Recovery of expired ids')
                    else:
                        self.logger.info('Problem in connecting to library')
                except Exception as err:
                    self.logger.error(('Problem in executing process'
                        ' for expired list!!'
                        ' close failure: %(exc)s : %(stack)s'),
                        {'exc': err, 'stack': ''.join(traceback.format_stack())})
                self.logger.info('End of getting expired list!')
                #sleep for 15 minutes
                sleep(900)
        except Exception as err:
            self.logger.error(('Process start failed!'
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def create_process(self):
        """
        Function to create a monitoring process
        """
        try:
            def kill_children(*args):
                self.logger.error('SIGTERM received')
                signal.signal(signal.SIGTERM, signal.SIG_IGN)
                self.logger.debug('killing child process')
                os.killpg(0, signal.SIGKILL)
                self.logger.debug('killed child process')

            def hup(*args):
                self.logger.error('SIGHUP received')
                signal.signal(signal.SIGHUP, signal.SIG_IGN)

            def sig(*args):
                self.logger.error('SIGSEGV received')
                signal.signal(signal.SIGSEGV, signal.SIG_DFL)

            signal.signal(signal.SIGTERM, kill_children)
            signal.signal(signal.SIGHUP, hup)
            #signal.signal(signal.SIGSEGV, sig)
            #signal.signal(signal.SIGCHLD, signal.SIG_IGN)
            # create monitoring process
            child_pid = os.fork()
            if child_pid == 0:
                signal.signal(signal.SIGHUP, signal.SIG_DFL)
                signal.signal(signal.SIGTERM, signal.SIG_DFL)
                #signal.signal(signal.SIGSEGV, signal.SIG_DFL)
                self.logger.debug('Starting new process')
                self.__start_process()
            else:
#                libraryUtils.OSDLoggerImpl("container-library").initialize_logger()
                if not os.path.exists(self.__pid_path):
                    os.makedirs(self.__pid_path)
                filename = os.path.join(self.__pid_path, CHILD_PID_FILE)
                with open(filename, "w") as child_file:
                    child_file.write(str(child_pid))
                self.logger.debug('Started child %s' % child_pid)
        except Exception as err:
            self.logger.error(('Problem in starting monitoring process'
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack())})
            sys.exit(130)

    def __active_transaction_check(self, hash_cont, req):
        """
        Helper function to raise exception in case
        container deletion is in progress
        """
        if hash_cont in self.__active_trans_dict:
            value = self.__active_trans_dict[hash_cont][1]
            if value == True:
                return HTTPConflict(body='Parallel active transactions'
                    ' are in progress', request=req)
        return None

    def __get_account_headers(self, req, req_type, cont_stat):
        """
        Helper function to get headers to request account service
        """
        try:
            account_headers = HeaderKeyDict({})
            if req_type == 'PUT':
                account_headers.update({
                    'x-put-timestamp': cont_stat.put_timestamp,
                    'x-delete-timestamp': cont_stat.delete_timestamp,
                    'x-object-count': cont_stat.object_count,
                    'x-bytes-used': cont_stat.bytes_used})
            else:
                timestamp = normalize_timestamp(req.headers['x-timestamp'])
                account_headers.update({'x-put-timestamp': '0', \
                'x-delete-timestamp': timestamp, 'x-object-count': 0, \
                'x-bytes-used' : 0})
            account_headers.update({
                'x-trans-id': req.headers.get('x-trans-id', '-'),
                'user-agent': 'container-server %s' % os.getpid(),
                'X-GLOBAL-MAP-VERSION': req.headers.get('x-global-map-version'),
                'X-COMPONENT-NUMBER': req.headers.get('x-component-number'),
                'referer': req.as_referer()})
            return account_headers
        except Exception as err:
            self.logger.exception(err)
            return HTTP_INTERNAL_SERVER_ERROR

    def __account_update(self, req, account, \
            container, req_type, cont_stat=None):
        """
        Update the account server(s) with latest container info.

        :param req: swob.Request object
        :param account: account name
        :param container: container name
        :param req_type: Type of request(PUT or DELETE)
        :param cont_stat: Container stat for PUT request
        :returns: if account request return a 404 error code,
                  HTTPNotFound response object is returned,
                  otherwise None.
        """
        #TODO: this code will fail. 
        #In case of node failure, this ip wont be valid.
        account_host = req.headers.get('X-Account-Host', '')
        account_device = req.headers.get('X-Account-Filesystem', '')
        account_partition = req.headers.get('X-Account-Directory', '')

        account_ip, account_port = account_host.rsplit(':', 1)
        new_path = '/' + '/'.join([account, container])
        try:
            self.logger.debug(('Started account update request for container:-'
                '%(container)s'), {'container' : container})
            account_headers = self.__get_account_headers \
                (req, req_type, cont_stat)
            self.logger.debug(('Got account headers:- %(headers)s'),
                {'headers' : account_headers})
            if account_headers == HTTP_INTERNAL_SERVER_ERROR:
                self.logger.debug('Internal error raised from getting headers')
                return HTTPInternalServerError(request=req)
            with ConnectionTimeout(self.__conn_timeout):
                self.logger.debug('Connection to account service started')
                conn = http_connect(
                    account_ip, account_port, account_device,
                    account_partition, 'PUT', new_path, account_headers)
                self.logger.debug('Connection to account service completed')
            with Timeout(self.__node_timeout):
                account_response = conn.getresponse()
                account_response.read()
                if account_response.status == HTTP_NOT_FOUND:
                    self.logger.debug('Not found error raised from'
                        ' account update request')
                    return HTTPNotFound(request=req)
                elif account_response.status == HTTP_FORBIDDEN:
                    self.logger.debug('Forbidden error raised from'
                        ' account update request')
                    return HTTPForbidden(request=req)
                elif not is_success(account_response.status):
                    self.logger.error((
                        'ERROR Account update failed '
                        'with %(ip)s:%(port)s/%(device)s (will retry '
                        'later): Response %(status)s %(reason)s'),
                        {'ip': account_ip, 'port': account_port,
                        'device': account_device,
                        'status': account_response.status,
                        'reason': account_response.reason})
        except (Exception, Timeout):
            self.logger.exception((
                'ERROR account update failed with '
                '%(ip)s:%(port)s/%(device)s (will retry later)'),
                {'ip': account_ip, 'port': account_port,
                'device': account_device})
        return None

    def __check_status(self, status, req):
        """
        check status from library and raises exception
        in case of any failure
        """
        if status == OsdExceptionCode.OSD_INTERNAL_ERROR:
            self.logger.debug('Internal error raised from library')
            return HTTPInternalServerError(request=req)
        elif status == OsdExceptionCode.OSD_FILE_OPERATION_ERROR:
            self.logger.debug('File operation error raised from library')
            return HTTPInternalServerError(request=req)
        elif status == OsdExceptionCode.OSD_MAX_META_COUNT_REACHED:
            self.logger.debug('Too many metadata items found')
            return HTTPBadRequest(body='Too many metadata items', \
                request=req)
        elif status == OsdExceptionCode.OSD_MAX_META_SIZE_REACHED:
            self.logger.debug('Max metadata size reached')
            return HTTPBadRequest(body='Max metadata size reached', \
                request=req)
        elif status == OsdExceptionCode.OSD_MAX_ACL_SIZE_REACHED:
            self.logger.debug('Max ACL size reached')
            return HTTPBadRequest(body='Max ACL size reached', \
                request=req)
        elif status == OsdExceptionCode.OSD_MAX_SYSTEM_META_SIZE_REACHED:
            self.logger.debug('Max sys metadata size reached')
            return HTTPBadRequest(body='Max sys metadata size reached', \
                request=req)
        elif status == OsdExceptionCode.OSD_NOT_FOUND:
            self.logger.debug('container not found raised from library')
            return HTTPNotFound(request=req)
        elif status == OsdExceptionCode.OSD_INFO_FILE_ACCEPTED:
            self.logger.debug('container already present')
            return HTTPAccepted(request=req)
        elif status == OsdExceptionCode.OSD_BULK_DELETE_TRANS_ADDED:
            self.logger.debug('Lock added for bulk delete request')
            return HTTPOk(request=req)
        elif status == OsdExceptionCode.OSD_BULK_DELETE_CONTAINER_SUCCESS:
            self.logger.debug('Container update success for bulk delete request')
            return HTTPOk(request=req)
        elif status == OsdExceptionCode.OSD_BULK_DELETE_CONTAINER_FAILED:
            self.logger.debug('Container update failed for bulk delete request')
            return HTTPInternalServerError(request=req)
        elif status == OsdExceptionCode.OSD_FLUSH_ALL_DATA_SUCCESS:
            self.logger.debug('All the data has been flushed')
            return HTTPOk(request = req , \
                           headers = {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK})
        elif status == OsdExceptionCode.OSD_FLUSH_ALL_DATA_ERROR:
            self.logger.debug('Error while flushing the data in case of stop service')
            return HTTPInternalServerError(request = req , \
                            headers = {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK})
        else:
            return None

    @public
    @timing_stats()
    def PUT(self, req):
        """Handle HTTP PUT request."""
        req_trans_id = req.headers.get('x-trans-id', '-')
        try:
            self.logger.debug(('Req headers got in PUT:- %(headers)s'
                ' with transaction ID:- %(id)s'),
                {'headers' : req.headers, 'id' : req_trans_id})
            filesystem, acc_dir, cont_dir, account, container, obj = \
                split_and_validate_path(req, 5, 6, True)
            if 'x-timestamp' not in req.headers or \
                not check_float(req.headers['x-timestamp']):
                return HTTPBadRequest(body='Missing timestamp', request=req,
                    content_type='text/plain')
            if self.__mount_check and not check_mount(self.__root, filesystem):
                return HTTPInsufficientStorage(drive=filesystem, request=req)
            if obj:
                self.logger.debug(('object PUT initiated for'
                    ' object:- %(object)s with transaction id:- %(id)s'),
                    {'object' : obj, 'id' : req_trans_id})
                # put container object
                status = self.con_op.put_object \
                    (filesystem, acc_dir, cont_dir, account, container, obj, req)
                resp = self.__check_status(status, req)
                if resp:
                    return resp
                self.logger.debug(('object PUT completed for '
                    'transaction id:- %(id)s'), {'id' : req_trans_id})
            else:
                # put container
                # check if active transcation is in progress
                acc_comp_number = req.headers.get('X-Account-Component-Number', '')
                if not acc_comp_number:
                    return HTTPBadRequest(body='Missing Component Number of account', request=req,
                        content_type='text/plain')
                hash_cont = hash_path(account, container)
                resp = self.__active_transaction_check(hash_cont, req)
                if resp:
                    return resp
                # Get metadata, if present
                self.logger.debug(('container PUT initiated for container:-'
                    '%(container)s with transaction id:- %(id)s'),
                    {'container' : container, 'id' : req_trans_id})
                metadata = self.con_op.get_metadata(req)
                self.logger.debug(('Got metadata for PUT request:- %(meta)s with '
                    'transaction id:- %(id)s'),
                    {'meta' : metadata, 'id' : req_trans_id})
                created, cont_stat = self.con_op.put_container \
                    (filesystem, acc_dir, cont_dir, account, container, metadata, req)
                resp = self.__check_status(created, req)
                if resp:
                    return resp
                # Send request to account, in case container updated or created
                req.headers['X-Component-Number'] = acc_comp_number
                resp = self.__account_update(req, account, \
                    container, 'PUT', cont_stat)
                if resp:
                    self.logger.debug(('Account updated raised an error '
                    'with transaction id:- %(id)s'), {'id' : req_trans_id})
                    return resp
                self.logger.debug(('Container PUT completed with '
                    'transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPCreated(request=req)
        except Exception as err:
            self.logger.exception(err)
            self.logger.debug(('Exception raised in PUT '
            'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPInternalServerError(request=req)

    def __revert_cont_changes(self, hash_cont):
        """
        Revert back the deletion flag
        """
        if hash_cont in self.__active_trans_dict:
            self.logger.debug('changes reverted in container')
            self.__active_trans_dict[hash_cont][1] = False

    @public
    @timing_stats()
    def DELETE(self, req):
        """Handle HTTP DELETE request."""
        req_trans_id = req.headers.get('x-trans-id', '-')
        self.logger.debug(('Req headers got in DELETE:- %(headers)s'
            ' with transaction ID :- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        hash_cont = ''
        try:
            filesystem, acc_dir, cont_dir, account, container, obj = \
                split_and_validate_path(req, 5, 6, True)
            if 'x-timestamp' not in req.headers or \
                not check_float(req.headers['x-timestamp']):
                return HTTPBadRequest(body='Missing timestamp', request=req,
                    content_type='text/plain')
            if self.__mount_check and not check_mount(self.__root, filesystem):
                return HTTPInsufficientStorage(drive=filesystem, request=req)
            if obj:
                # delete object
                self.logger.debug(('object DELETE initiated for'
                    ' object:- %(object)s with transaction id:- %(id)s'), 
                    {'object' : obj, 'id' : req_trans_id})
                status = self.con_op.delete_object \
                    (filesystem, acc_dir, cont_dir, account, container, obj, req)
                resp = self.__check_status(status, req)
                if resp:
                    return resp
            else:
                acc_comp_number = req.headers.get('X-Account-Component-Number', '')
                if not acc_comp_number:
                    return HTTPBadRequest(body='Missing Component Number of account', request=req,
                        content_type='text/plain')
                self.logger.debug(('container DELETE initiated for container:-'
                    '%(container)s with transaction id:- %(id)s'),
                    {'container' : container, 'id' : req_trans_id})
                hash_cont = hash_path(account, container)
                # add entry of container in memory, if not present
                # This will handle the case when empty container is deleted and
                # put request for the same container is initiated at the same time.
                if hash_cont not in self.__active_trans_dict:
                    self.__active_trans_dict.update({hash_cont : [0, False]})
                # check active operation flag
                value = self.__active_trans_dict[hash_cont]
                op_flag = value[0]
                if op_flag > 0:
                    return HTTPConflict(request=req)
                value[1] = True
                # delete container
                deleted = self.con_op.delete_container \
                    (filesystem, acc_dir, cont_dir, account, container)
                if deleted == OsdExceptionCode.OSD_INTERNAL_ERROR:
                    self.logger.debug(('Internal error raised from library '
                        'with transaction id:- %(id)s'), {'id' : req_trans_id})
                    # Revert back changes in memory
                    self.__revert_cont_changes(hash_cont)
                    return HTTPInternalServerError(request=req)
                elif deleted == OsdExceptionCode.OSD_NOT_FOUND:
                    self.logger.debug(('container not found raised from library '
                        'with transaction id:- %(id)s'), {'id' : req_trans_id})
                    # Delete container entry from internal memory
                    if hash_cont in self.__active_trans_dict:
                        del self.__active_trans_dict[hash_cont]
                    return HTTPNotFound(request=req)
                else:
                    pass
                # Delete container entry from internal memory
                if hash_cont in self.__active_trans_dict:
                    del self.__active_trans_dict[hash_cont]
                self.logger.debug(('container deleted from internal memory '
                    'with transaction id:- %(id)s'), {'id' : req_trans_id})

                req.headers['X-Component-Number'] = acc_comp_number
                resp = self.__account_update(req, account, container, 'DELETE')
                if resp:
                    self.logger.debug(('Account updated raised an error '
                        'with transaction id:- %(id)s'), {'id' : req_trans_id})
                    return resp
                self.logger.debug(('Container DELETE completed '
                    'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPNoContent(request=req)
        except HTTPException as error:
            self.logger.exception(error)
            self.__revert_cont_changes(hash_cont)
            self.logger.debug(('Conflict occurred in DELETE '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPConflict(request=req)
        except Exception as err:
            self.logger.exception(err)
            self.__revert_cont_changes(hash_cont)
            self.logger.debug(('Exception raised in DELETE '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPInternalServerError(request=req)

    @public
    @timing_stats(sample_rate=0.1)
    def HEAD(self, req):
        """Handle HTTP HEAD request."""
        req_trans_id = req.headers.get('x-trans-id', '-')
        self.logger.debug(('Req headers got in HEAD:- %(headers)s'
            ' with transaction ID:- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        try:
            filesystem, acc_dir, cont_dir, account, container, _ = \
                split_and_validate_path(req, 5, 6, True)
            if self.__mount_check and not check_mount(self.__root, filesystem):
                return HTTPInsufficientStorage(drive=filesystem, request=req)
            hash_cont = ''
            if 'x-updater-request' in req.headers:
                hash_cont = container
            else:
                hash_cont = hash_path(account, container)
            resp = self.__active_transaction_check(hash_cont, req)
            if resp:
                return resp
            self.logger.debug(('Container HEAD initiated for container:-'
                '%(container)s with transaction id:- %(id)s'),
                {'container' : container, 'id' : req_trans_id})
            headers = self.con_op.head \
                (filesystem, acc_dir, cont_dir, account, container, req)

            #object versioning support in n-HN
            if is_param_exist(req, 'generate_next_version_id'):
                # increment subversion y value
                self._object_subversion_y += 1 
                object_version_id = self.__get_object_version_id()
                #update header (add X-OSD-Version-Id)
                headers.update({'X-OSD-Version-Id' : object_version_id})
 
            self.logger.debug(('Container HEAD completed with '
                'transaction id:- %(id)s'), {'id' : req_trans_id})
        except HTTPException as error:
            # we do not want to raise an exception in case container not found by updater.
            if error.status_int == HTTP_NOT_FOUND and 'x-updater-request' in req.headers:
                self.logger.debug(('Container not found raised from library '
                    'with transaction id:- %(id)s'), {'id' : req_trans_id})
                return HTTPNotFound(request=req)

            self.logger.exception(error)
            if error.status_int == HTTP_NOT_FOUND:
                self.logger.debug(('Container not found raised from library '
                    'with transaction id:- %(id)s'), {'id' : req_trans_id})
                return HTTPNotFound(request=req)
            return HTTPInternalServerError(request=req)
        except Exception as err:
            self.logger.exception(err)
            self.logger.debug(('Exception raised in HEAD '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPInternalServerError(request=req)
        return HTTPNoContent(request=req, headers=headers, charset='utf-8')
       
    @public
    @timing_stats()
    def GET(self, req):
        """Handle HTTP GET request."""
        req_trans_id = req.headers.get('x-trans-id', '-')  
        self.logger.debug(('Req headers got in GET:- %(headers)s'
            ' with transaction ID:- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        try:
            filesystem, acc_dir, cont_dir, account, container, _ = \
                split_and_validate_path(req, 5, 6, True)
            delimiter = get_param(req, 'delimiter')
            if delimiter and (len(delimiter) > 1 or ord(delimiter) > 254):
            # delimiters can be made more flexible later
                return HTTPPreconditionFailed(body='Bad delimiter')
            limit = CONTAINER_LISTING_LIMIT
            given_limit = get_param(req, 'limit')
            if given_limit and given_limit.isdigit():
                limit = int(given_limit)
                if limit > CONTAINER_LISTING_LIMIT:
                    return HTTPPreconditionFailed(
                    request=req,
                    body='Maximum limit is %d' % CONTAINER_LISTING_LIMIT)
            if self.__mount_check and not check_mount(self.__root, filesystem):
                return HTTPInsufficientStorage(drive=filesystem, request=req)
            hash_cont = hash_path(account, container)
            resp = self.__active_transaction_check(hash_cont, req)
            if resp:
                return resp
            # get list of objects
            self.logger.debug(('container GET initiated for container:-'
                '%(container)s with transaction id:- %(id)s'),
                {'container' : container, 'id' : req_trans_id})
            body_content, headers = self.con_op.get \
                (filesystem, acc_dir, cont_dir, account, container, req)
            if not body_content:
                return HTTPNoContent(request=req, headers=headers)
            out_content_type = get_listing_content_type(req)
            ret = Response(request=req, headers=headers,
                content_type=out_content_type, charset='utf-8')
            ret.body = body_content
            self.logger.debug(('Container GET completed with '
                'transaction id:- %(id)s'), {'id' : req_trans_id})
            return ret
        except HTTPException as error:
            self.logger.exception(error)
            if error.status_int == HTTP_NOT_FOUND:
                self.logger.debug(('container not found raised from library '
                    'with transaction id:- %(id)s'), {'id' : req_trans_id})
                return HTTPNotFound(request=req)
            self.logger.debug(('Internal error raised from library '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPInternalServerError(request=req)
        except Exception as err:
            self.logger.exception(err)
            self.logger.debug(('Exception raised in GET '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPInternalServerError(request=req)

    @public
    @timing_stats()
    def POST(self, req):
        """Handle HTTP POST request."""
        req_trans_id = req.headers.get('x-trans-id', '-')
        self.logger.debug(('Req headers got in POST:- %(headers)s'
            ' with transaction ID:- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        try:
            filesystem, acc_dir, cont_dir, account, container = \
                split_and_validate_path(req, 5)
            if 'x-timestamp' not in req.headers or \
                not check_float(req.headers['x-timestamp']):
                return HTTPBadRequest(body='Missing or bad timestamp',
                    request=req, content_type='text/plain')
            if self.__mount_check and not check_mount(self.__root, filesystem):
                return HTTPInsufficientStorage(drive=filesystem, request=req)
            hash_cont = hash_path(account, container)
            resp = self.__active_transaction_check(hash_cont, req)
            if resp:
                return resp
            self.logger.debug(('container POST initiated for container:-'
                '%(container)s with transaction id:- %(id)s'),
                {'container' : container, 'id' : req_trans_id})
            metadata = self.con_op.get_metadata(req)
            self.logger.debug(('Got metadata:- %(meta)s '
                'with transaction id:- %(id)s'),
                {'meta' : metadata, 'id' : req_trans_id})
            status = self.con_op.post \
                (filesystem, acc_dir, cont_dir, account, container, metadata)
            resp = self.__check_status(status, req)
            if resp:
                return resp
            return HTTPNoContent(request=req)
        except Exception as err:
            self.logger.exception(err)
            self.logger.debug(('Exception raised in POST '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            return HTTPInternalServerError(request=req)

    @public
    @timing_stats()
    def GET_LOCK(self, req):
        """Handle HTTP GET_LOCK request."""
        req_trans_id = req.headers.get('x-trans-id', '-')
        self.logger.debug(('Req headers got in GET_LOCK:- %(headers)s'
            ' with transaction ID:- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        try:
            full_path = ''
            try:
                full_path = req.headers['x-object-path']
            except Exception as err:
                self.logger.debug('GET_LOCK Request doesnot contain '
                'the object path!')
                return HTTPConflict(request=req)
            # Check if active transaction is in progress 
            _, container, _ = full_path.split('/')
            resp = self.__active_transaction_check(container, req)
            if resp:
                return resp
            # Acquire lock
            #TODO:(jaivish)Either catch the exception here and rethrow it after filtering the value
            trans_id_dict = self.trans_mgr.get_lock(req)
            # Populate memory
            self.__update_internal_memory(full_path)
            response = Response(request=req, \
                conditional_response=True, headers=trans_id_dict)
            return response                                  
        except HTTPException as error:
            self.logger.exception(error)
            if error.status_int == HTTP_NOT_ACCEPTABLE:
                #Mapped NOTAcceptable to conflict as it could be raised from 2 reasons
                #but only when lib conflicts needs to be like this
                path_obj = req.headers['x-object-path']
                component_no = req.headers.get('x-component-number')
                trans_id_dict = self.trans_mgr.conflict_dict(path_obj, component_no)
                #Does it needs to be blocking or threaded
                #Call library interface here... and pass trans_list here
                self.conflict_spawn = free_lock_container(self.__free_conflict_lock, req, trans_id_dict)
                try:
                    with Timeout(self.__conflict_timeout):#This timeout val is also local for now.
                        self.conflict_spawn.wait()
                except Timeout:
                    self.logger.debug("waited for some time to finish")
                    # (Jaivish)I think we should wait for some time for green threads also that's why added a timeout
                    # Also spawning a thread when function returns ends the context of that spawn also
                    # i am not very sure if async needs to be attached to the object of the class as done,
                    # but could not find other way, as making a thread async and daemonising is  diff maybe
                return HTTPConflict(request=req)
            if error.status_int == HTTP_CONFLICT:
                return HTTPConflict(request=req)
            return HTTPServiceUnavailable(request=req)
        except Exception as err:
            self.logger.exception(err)
            return HTTPInternalServerError(request=req)

    @public
    @timing_stats()
    def FREE_LOCK(self, req):
        """Handle HTTP FREE_LOCK request."""
        req_trans_id = req.headers.get('x-trans-id', '-')
        self.logger.debug(('Req headers got in FREE_LOCK:- %(headers)s'
            ' with transaction ID:- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        try:
            full_path = ''
            try:
                full_path = req.headers['x-object-path']
            except Exception as err:
                self.logger.debug('FREE_LOCK Request doesnot contain '
                'the object path!')
                return HTTPConflict(request=req)
            # Release lock
            ret = self.trans_mgr.free_lock(req)
            if not ret:
                return HTTPServiceUnavailable(request=req)
            # Populate memory
            _, container, _ = full_path.split('/')
            if container in self.__active_trans_dict:
                self.logger.debug('container found in memory for lock release')
                value = self.__active_trans_dict[container]
                value[0] -= 1
            else:
                self.logger.debug(('Request came for lock release'
                    ' for container:- %(cont)s that'
                    ' doesnt exist'), {'cont' : container})
            response = Response(request=req, conditional_response=True)
            return response
        except Exception as err:
            self.logger.exception(err)
            return HTTPInternalServerError(request=req)

    @public
    @timing_stats()
    def FREE_LOCKS(self, req):
        """Handle HTTP FREE_LOCKS request."""
        req_trans_id = req.headers.get('x-trans-id', '-')
        self.logger.debug(('Req headers got in FREE_LOCKS:- %(headers)s'
            ' with transaction ID:- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        try:
            full_path = ''
            try:
                full_path = req.headers['x-object-path']
            except Exception as err:
                self.logger.debug('FREE_LOCKS Request doesnot contain '
                'the object path!')
                return HTTPConflict(request=req)
            # Release lock
            ret = self.trans_mgr.free_locks(req)
            if not ret:
                return HTTPServiceUnavailable(request=req)
            # Populate memory
            _, container, _ = full_path.split('/')
            if container in self.__active_trans_dict:
                self.logger.debug('container found in memory for locks release')
                value = self.__active_trans_dict[container]
                value[0] -= 1
            else:
                self.logger.debug(('Request came for locks release'
                    ' for container:- %(cont)s that'
                    ' doesnt exist'), {'cont' : container})
            response = Response(request=req, conditional_response=True)
            return response
        except Exception as err:
            self.logger.exception(err)
            return HTTPInternalServerError(request=req)

    @public
    @timing_stats()
    def ACCEPT_COMPONENT_CONT_DATA(self, request):
        """Handle HTTP ACCEPT_COMPONENT_CONT_DATA request"""
        try:
            self.logger.info("ACCEPT_COMPONENT_CONTAINER_DATA")
            self.logger.debug("Headers: %s" % request.headers)
            length = request.headers['Content-Length']
            req_object_gl_version = int(request.headers['X-Object-Gl-Version-Id'])
            x_accept_component_list = eval(request.headers['X-COMPONENT-NUMBER-LIST'])
            x_recovery_request = bool(request.headers.get('X-Recovery-Request', False))
            self.logger.info('req_object_gl_version of type %s -> %s, ' \
                             'self._object_major_version_x of type %s -> %s ' % \
                             (type(req_object_gl_version), req_object_gl_version, \
                              type(self._object_major_version_x), \
                              self._object_major_version_x))
            self.logger.info('Component list received in accept component :%s' % \
                              x_accept_component_list)

            component_list = libContainerLib.list_less__component_names__greater_()
            for i in x_accept_component_list:
                self.logger.debug('Component %s added to the list.' % i)
                component_list.append(int(i))
                self.__accept_comp_block_list.append(int(i))

            self.logger.debug('Component list : %s' % component_list)

            if req_object_gl_version > self._object_major_version_x:
                #get object gl version from GL and set the value
                try:
                    self._object_major_version_x = self.get_object_gl_version()
                    self.logger.debug("object major version(X) value %s" % self._object_major_version_x)
                except TimeoutError, ex:
                    self.logger.error("ERROR : Timeout while loading gl map: %s" % ex)
                    self.__accept_comp_block_list = []
                    return HTTPTemporaryRedirect(body='ERROR : Timeout while loading gl map')
                    #sys.exit(130)
                # reset subversion
                self._object_subversion_y = 0
            pickled_string = ""
            for chunk in iter(lambda: request.environ['wsgi.input'].read(65536),''):
                pickled_string += chunk
            list_received = pickle.loads(pickled_string)

            self.logger.debug("List Received: %s"% list_received)

            #if list_received:
            resp =  self.con_op.accept_components_cont_data(list_received, x_recovery_request, component_list)
            #The above else statement will be removed
            #else:
            #    resp = 400

            if resp == 400:
                self.__accept_comp_block_list = []
                return  HTTPOk(request=request)
            else:
                self.__accept_comp_block_list = []
                return HTTPInternalServerError(request=request)

        except Exception as err:
            self.logger.exception(err)
            self.logger.debug(('Exception raised in ACCEPT_COMPONENT_CONT_DATA'
                'with transaction id:- %(id)s'), {'id' : request.headers})
            return HTTPInternalServerError(request=request)

    @public
    @timing_stats()
    def ACCEPT_COMPONENT_TRANS_DATA(self, request):
        """Handle HTTP ACCEPT_COMPONENT_TRANS_DATA request"""
        try:
            self.logger.info("ACCEPT_COMPONENT_TRANS_DATA")
            length = request.headers['Content-Length']
            self.logger.debug("Headers: %s" % request.headers)
            req_object_gl_version = int(request.headers['X-Object-Gl-Version-Id'])
            self.logger.info('req_object_gl_version of type %s -> %s, ' \
                             'self._object_major_version_x of type %s -> %s ' % \
                             (type(req_object_gl_version), req_object_gl_version, \
                              type(self._object_major_version_x), \
                              self._object_major_version_x))
            if req_object_gl_version > self._object_major_version_x:
                #get object gl version from GL and set the value

                #TODO(rasingh): object version connection must be closed as this raises error.
                #eg: calling obj version from above interface and then this raises error.
                
                #self._object_major_version_x = self.get_object_gl_version()
                self.logger.debug("object major version(X) value %s" % self._object_major_version_x)
                # reset subversion
                self._object_subversion_y = 0
                
                
            pickled_string = ""
            for chunk in iter(lambda: request.environ['wsgi.input'].read(65536),''):
                pickled_string += chunk
            list_received = pickle.loads(pickled_string)

            self.logger.debug("List Received: %s"% list_received)
            if list_received:
                resp =  self.trans_mgr.accept_components_trans_data(list_received)
                self.logger.info("Response from Transaction Library: %s" % resp)
            else:
                resp = 302

            if resp == 302:
                return  HTTPOk(request=request)
            else:
                return HTTPInternalServerError(request=request)

        except Exception as err:
            self.logger.exception(err)
            self.logger.debug(('Exception raised in ACCEPT_COMPONENT_TRANS_DATA'
                'with transaction id:- %(id)s'), {'id' : request.headers})
            return HTTPInternalServerError(request=request)

    @public
    @timing_stats()
    def TRANSFER_COMPONENTS(self, req):
        try:
            self.logger.info("Inside Transfer Component")
            req.headers.update({'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE})
            message = TransferCompMessage()
            message.de_serialize(req.environ['wsgi.input'].read())
            comp_transfer_map = message.service_comp_map()
            self.logger.info("Msg deserialized")

            final_transfer_status = True
            list_of_tuple = queue(maxsize=2000)
            final_transferred_map = []
            finish_transfer_component = [True]

            #Exit (Return HTTP OK) if comp_transfer_map is empty
            if not comp_transfer_map:
                self.logger.info("Map is empty, return HTTP OK!") 
                resp_obj = TransferCompResponseMessage(final_transferred_map, final_transfer_status)
                message = resp_obj.serialize()
                resp_headers = {'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE}
                # return HTTP response
                return HTTPOk(
                             body=message,
                             request=req,
                             headers=resp_headers
                        )
            if len(self.__blocked_component_list) > 0:
                self.logger.info("Blocking Transfer components because previous reqeust is still in progress")
                block_process = True
                while(block_process):
                    for val in comp_transfer_map.values():
                        if val in self.__blocked_component_list:
                            self.logger.debug("component found in block list. waiting for cleaning block list")
                            block_process = True
                            eventlet.sleep(20)
                            break
                        else:
                            block_process = False

            comp_entries = libContainerLib.list_less__component_names__greater_()
            for list in comp_transfer_map.values():
                for i in list:
                    self.logger.debug("Blocking comp: %s" % i)
                    comp_entries.append(i)
                    self.__blocked_component_list.append(i)


            #For Debugging logging blocked component list
            self.logger.debug("Blocked component List :%s" % \
                self.__blocked_component_list)

            #Check if there is any operation going on the components
            #    which needs to be transfered
            while self.__search_in_comp_request_dict(self.__blocked_component_list):
                eventlet.sleep(2)             #sleep for 2 sec to complete the ongoing operations

            #Create communication object to communicate with GL
            gl_comm = Req(self.logger)
            gl_info_obj = gl_comm.get_gl_info()
            retry_count = 0
            while retry_count < 3:
                retry_count += 1
                communicator = gl_comm.connector(IoType.EVENTIO, gl_info_obj)
                if communicator:
                    self.logger.debug("CONN object success")
                    break
                else:
                    self.logger.error("Failed to get connection obj: count \
                        %s/3" % retry_count)
                    eventlet.sleep(2)

            if not communicator:
                #Exit in case of no connection object is found
                self.logger.error("Exiting After no communication object")
                return HTTPTemporaryRedirect(body='ERROR : no communication object')
                #sys.exit(130)

            self.logger.debug("conn obj:%s, retry_count:%s " % (communicator, retry_count))

            #Create Function to send the Intermediate response
            def send_transfer_status(gl_comm, communicator):
                """
                Start sending container status
                """
                self.logger.info("Sending intermediate state to GL")
                retry_count = 3
                counter = 0

                while finish_transfer_component[0]:
                    self.logger.debug("In finish_transfer_component while loop. Size of Queue: %s" % list_of_tuple.qsize())
                    if list_of_tuple.qsize() > 0:
                        li = []
                        while list_of_tuple.qsize() > 0:
                            li = list_of_tuple.get()
                            while counter < retry_count:

                                if not communicator:
                                    #Exit in case of no connection object is found
                                    self.logger.error("Exiting After no communication object")
                                    counter += 1
                                    continue

                                ret = gl_comm.comp_transfer_info(self.__service_id, \
                                            li, \
                                            communicator)
                                if ret.status.get_status_code() in [Resp.SUCCESS]:
                                    self.logger.info("container server server_id : %s Component transfer Completed" % self.__service_id)
                                    cont_entries = len(container_entries)
                                    components = libContainerLib.list_less__component_names__greater_()
                                    for var in [x[0] for x in li]:
                                        components.append(var)
                                        #Below line should be reviewed if container service is getting request for
    					# for those components which is not in its ownership
                                        try:
                                            self.__blocked_component_list.remove(var)
                                        except Exception as ex:
                                            self.logger.error("Exception in removing component from blocked component list :%s"%ex)

                                        if var in container_entries:
                                            del container_entries[var]
                                        if var in transaction_entries:
                                            del transaction_entries[var]
                                    #if cont_entries != 0:
                                    self.con_op.remove_transferred_record(components)

                                    #Update the local copy of global map and global map version
                                    if (ret.message != None):
                                        self.logger.debug("Update the global_map_version[%s]"\
                                            % ret.message.version)
                                        self.global_map = ret.message.map()
                                        self.global_map_version = ret.message.version()
                                    else:
                                        self.logger.debug("ret.message is of None type!")

                                    self.logger.info("send_transfer successfull")
                                    break
                                elif ret.status.get_status_code() in [Resp.BROKEN_PIPE, Resp.CONNECTION_RESET_BY_PEER, Resp.INVALID_SOCKET]:
                                    self.logger.info("container server server_id : %s, Connection resetting" % self.__service_id)
                                    try:
                                        gl_comm = Req(self.logger, timeout=0)
                                        gl_info_obj = gl_comm.get_gl_info()
                                    except TimeoutError, ex:
                                        self.logger.error("TIMEOUT OCCURED IN COMMUNICATION %s"% ex)
                                        return HTTPTemporaryRedirect(body='TIMEOUT OCCURED IN COMMUNICATION')
                                        #sys.exit(130)

                                    retry_count = 0
                                    while retry_count < 3:
                                        retry_count += 1
                                        communicator = gl_comm.connector(IoType.EVENTIO, gl_info_obj)
                                        if communicator:
                                            self.logger.debug("CONN object success")
                                            break
                                    self.logger.debug("conn obj:%s, retry_count:%s " % (communicator, retry_count))

                                else:
                                    self.logger.info("container server server_id : %s Component transfer Failed, Retrying... " % self.__service_id)
                                counter += 1

                            self.logger.info("send_transfer_complete")
                        #queue_finish_event.send()
                        list_of_tuple.task_done()
                    else:
                        self.logger.debug("List of tuple empty, sleeping")
                        eventlet.sleep(0.5)
                    if counter >= retry_count:
                        self.logger.info("Failed to Notify Gl. Reverting Back")
                        cont_records = []
                        trans_records = []
                        for cont_record_list in container_entries.values():
                            cont_records = cont_records + cont_record_list
                        for trans_record_list in transaction_entries.values():
                            trans_records = trans_records + trans_record_list
                        self.con_op.revert_back_cont_data(cont_records, False)
                        self.trans_mgr.revert_back_trans_data(trans_records, False)
                        finish_transfer_component[0] = False

            #Create thread for sending the Intermediate response
            pile = GreenPile(1)
            pile.spawn(send_transfer_status, gl_comm, communicator)
            self.logger.info("Extracting data from Container Library")
            container_entries = self.con_op.extract_container_data(comp_entries)

            self.logger.info("Size of data from Container Library: %s" % len(container_entries))
            self.logger.info("Extracting data from Transaction Library")

            transaction_entries = self.trans_mgr.extract_transaction_data(comp_entries)
            self.logger.info("Size of data from Transaction Library: %s" % len(transaction_entries))

            """
	    # Commenting below section of code because connection to destination service 
	    # should be created even in case of  no record need to be transfer
            if len(container_entries) == 0 and len(transaction_entries) == 0:
                tuple_list = []
                for key, var in comp_transfer_map.iteritems():
                    for comp in var:
                        tuple_list.append((comp, key))
                self.logger.info("List Insterted in Queue : %s" %tuple_list)
                list_of_tuple.put(tuple_list)
                while list_of_tuple.qsize() != 0:
                    self.logger.debug("list_of_tuple size: %s" % list_of_tuple.qsize())
                    eventlet.sleep(0.5)
                #while list_of_tuple.qsize() != 0:
                #    queue_finish_event.wait()
                #list_of_tuple.join()
                finish_transfer_component[0] = False
                list_of_tuple.join()
                for comp_list in comp_transfer_map.values():
                    for comp in comp_list:
                        final_transferred_map.append((comp, True))
                resp_obj = TransferCompResponseMessage(final_transferred_map, final_transfer_status)
                message = resp_obj.serialize()
		resp_headers = {'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE}
	        # return HTTP response 
        	return HTTPOk(
	                     body=message,
        	             request=req,
                	     headers=resp_headers
                     	)
              """

            self.service_component_map  = {}
            for key in comp_transfer_map.keys():
                ip_port = "%s:%s"% (key.get_ip(), str(key.get_port()))
                self.service_component_map[ip_port] = key
                self.logger.info("Inserting key in serv comp map: %s" % key.get_ip())

            dict_container = {}
            dict_transaction = {}
            self.inter_resp = {}
            try:
                #if len(container_entries) != 0:
                self.logger.info("Map size extracted from containerlibrary : %s" %len(container_entries))
		#Creating map for container records
                for service_ip in self.service_component_map.keys():
                    temp_list = []
                    for component_name in comp_transfer_map[self.service_component_map[service_ip]]:
                        if component_name in container_entries.keys():
                            [temp_list.append(i) for i in container_entries[component_name]]
                    dict_container[service_ip] = temp_list
                    self.inter_resp[service_ip] = False
                if len(transaction_entries) != 0:
                    self.logger.info("Map size extracted from transactionlibrary : %s" %len(transaction_entries))
		    #Creating map for transaction records
                    for service_ip in self.service_component_map.keys():
                        temp_list = []
                        for component_name in comp_transfer_map[self.service_component_map[service_ip]]:
                            if component_name in transaction_entries.keys():
                                [temp_list.append(i) for i in transaction_entries[component_name]]
                        dict_transaction[service_ip] = temp_list
            except KeyError:
                self.logger.info("Key Not found")
            target_node_service_iter = GreenthreadSafeIterator(dict_container) #can be used with .values
            container_pile_size = len(dict_container)
            if container_pile_size == 0:
                for comp_list in comp_transfer_map.values():
                    for comp in comp_list:
                        final_transferred_map.append((comp, True))
                self.logger.info("No object records found")
            else:
                pile =  GreenPile(container_pile_size)
                method = "ACCEPT_COMPONENT_CONT_DATA"
    	        # Create connection to destination services
                for target_service in target_node_service_iter:
                    headers = {}
                    self.logger.info("pickling data" )
                    headers['Content-Length'] = len(pickle.dumps(dict_container.get(target_service)))
                    self.logger.info("pickled data length: %s" %headers['Content-Length'])
                    headers['X-GLOBAL-MAP-VERSION'] = self.global_map_version
                    if self.service_component_map.has_key(target_service):
                        if comp_transfer_map.has_key(self.service_component_map[target_service]):
                            headers['X-COMPONENT-NUMBER-LIST'] = comp_transfer_map[self.service_component_map[target_service]]
                        else:
                            headers['X-COMPONENT-NUMBER-LIST'] = []
                    else:
                        headers['X-COMPONENT-NUMBER-LIST'] = []
                    pile.spawn(self._connect_target_node, target_service, \
                           self.logger.thread_locals, method, headers)

                conns = [conn for conn in pile if conn]
                try:
                    with ContextPool(container_pile_size) as pool:
                        for conn in conns:
                            conn.failed = False
                            #Create Key for destination service
                            key = "%s:%s" % (conn.node['ip'], conn.node['port'])
                            #serialize data
                            self.logger.info("Pickling Data")
                            conn.reader = pickle.dumps(dict_container[key])
                            #send http to destination servers
                            self.logger.info("Send ACCEPT_COMPONENT_CONT_DATA to target node, target service: %s:%s"\
                            			%(conn.node['ip'], conn.node['port']))
                            pool.spawn(self.__send_data, conn)
                        pool.waitall()
    
                    conns = [conn for conn in conns if not conn.failed]
                    self.logger.info("Alive Connections: %s" % conns)
                    if not conns:
                        self.logger.info("No connection Established, Target is down, Exit")
                        for comp_list in comp_transfer_map.values():
                            for comp in comp_list:
                                final_transferred_map.append((comp, False))
                        finish_transfer_component[0] = False
                        list_of_tuple.join()
                        self.__clean_block_component_list()
                        self.logger.info("Reverting back container data")
                        cont_records = []
                        for cont_record_list in container_entries.values():
                            cont_records = cont_records + cont_record_list
                        self.con_op.revert_back_cont_data(cont_records, True)
                        if len(transaction_entries) != 0:
                            self.logger.info("Reverting back transaction data")
                            trans_records = []
                            for trans_record_list in transaction_entries.values():
                                trans_records = trans_records + trans_record_list
                            self.trans_mgr.revert_back_trans_data(trans_records, True)
                        self.logger.info("Finishing Transfer Component for Container server id : %s" % self.__service_id)
                        resp_obj = TransferCompResponseMessage(final_transferred_map, final_transfer_status)
                        message = resp_obj.serialize()
                        resp_headers = {'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE}
                        # return HTTP response
                        return HTTPOk(
                                     body=message,
                                     request=req,
                                     headers=resp_headers
                                     )

                except Exception as ex:
                    self.logger.error("Exception raised: %s" % ex)
                    #return (['500'], [''], [''])
                    return HTTPInternalServerError(request = req)
                except Timeout as ex:
                    self.logger.info("Timeout occured : %s" % ex)
                    self.logger.exception(
                        _('ERROR Exception causing client disconnect'))
                    conn.close()
                    return HTTPInternalServerError(request = req)
                    #return (['500'], [''], [''])
            
                #getting response from dest services
                pile = GreenAsyncPile(len(conns))
                for conn in conns:
                    self.logger.info("In component transfer, Getting http responses from destination servers")
                    pile.spawn(self.get_conn_response, conn)

                for (response, conn) in pile:
                    if response:
                        self.logger.info("In component transfer, response status for destination \
				 server %s : %s" % (self.service_component_map[conn.target_service], conn.status))
                        if conn.status:
                            if response.status == 200:
                                for comp in comp_transfer_map[self.service_component_map[conn.target_service]]:
                                    final_transferred_map.append((comp, True))
                                self.inter_resp[conn.target_service] = True
                            else:
                                for comp in comp_transfer_map[self.service_component_map[conn.target_service]]:
                                    final_transferred_map.append((comp, False))
                                final_transfer_status = False
                                del comp_transfer_map[self.service_component_map[conn.target_service]]
                                self.__clean_block_component_list()
                                self.con_op.revert_back_cont_data(dict_container[conn.target_service], True)
                                if dict_transaction.has_key(conn.target_service):
                                    self.trans_mgr.revert_back_trans_data(dict_transaction[conn.target_service], True)
                                    del dict_transaction[conn.target_service]
                                del dict_container[conn.target_service]
                        
            self.logger.info("Now sending transaction data")
            for key in self.inter_resp:
                if self.inter_resp[key] ==  False:
                    self.con_op.revert_back_cont_data(dict_container[key], True)
                    for comp in comp_transfer_map[self.service_component_map[key]]:
                        final_transferred_map.append((comp, False))
                        if comp in self.__blocked_component_list:
                            self.__blocked_component_list.remove(comp)
                    final_transfer_status = False
                    del comp_transfer_map[self.service_component_map[key]]
                    if dict_transaction.has_key(key):
                        self.trans_mgr.revert_back_trans_data(dict_transaction[key], True)
                        del dict_transaction[key]
            target_node_service_iter = GreenthreadSafeIterator(dict_transaction) #can be used with .values
            transaction_pile_size = len(dict_transaction)
            pile =  GreenPile(transaction_pile_size)
            self.logger.info("pile size of connections pile: %s node: "% transaction_pile_size)
            if transaction_pile_size == 0:
                for conn in conns:
                    comp_service = self.service_component_map.get("%s:%s" % \
                                (conn.node['ip'], conn.node['port']))
                    tuple_list = []
                    for comp in comp_transfer_map[comp_service]:
                        tuple_list.append((comp, comp_service))
                    list_of_tuple.put(tuple_list)
                    eventlet.sleep(0.5)
                while list_of_tuple.qsize() != 0:
                    eventlet.sleep(0.5)
                finish_transfer_component[0] = False
                list_of_tuple.join()
                self.logger.info("Finish Transfer Component for Container server id : %s" % self.__service_id)
                resp_obj = TransferCompResponseMessage(final_transferred_map, final_transfer_status)
                message = resp_obj.serialize()
                resp_headers = {'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE}
                # return HTTP response
                return HTTPOk(
                             body=message,
                             request=req,
                             headers=resp_headers
                             )
            method = 'ACCEPT_COMPONENT_TRANS_DATA'
            for target_service in target_node_service_iter:
                try:
                    headers = {}
                    self.logger.info("pickling data")
                    headers['Content-Length'] = len(pickle.dumps(dict_transaction.get(target_service)))
                    self.logger.info("data pickled")
                    headers['X-GLOBAL-MAP-VERSION'] = self.global_map_version
                    self.logger.info("spawing green thread")
                    pile.spawn(self._connect_target_node, target_service, \
                           self.logger.thread_locals, method, headers)
                    self.logger.info("green thread spawned")
                except Exception as ex:
                    self.logger.error("Ex: %s" %ex)
                except:
                    self.logger.error("ERROR occurred")

            conns = [conn for conn in pile if conn]
            try:
                with ContextPool(transaction_pile_size) as pool:
                    self.logger.debug("ContextPool")
                    for conn in conns:
                        conn.failed = False
                        #Create Key for destination service
                        key = "%s:%s" % (conn.node['ip'], conn.node['port'])
                        #serialize data
                        self.logger.info("Pickling data")
                        conn.reader = pickle.dumps(dict_transaction[key])
                        # send http request to destination servers
                        self.logger.info("Send ACCEPT_COMPONENT_TRANS_DATA to target node, target service: %s:%s"\
                                                %(conn.node['ip'], conn.node['port']))
                        pool.spawn(self.__send_data, conn)
                    pool.waitall()

                conns = [conn for conn in conns if not conn.failed]
                self.logger.info("Alive Connections: %s" % conns)
                if not conns:
                    self.logger.info("No connection Established, Target is down, Exit")
                    final_transferred_map = []
                    for comp_list in comp_transfer_map.values():
                        for comp in comp_list:
                            final_transferred_map.append((comp, False))
                    finish_transfer_component[0] = False
                    list_of_tuple.join()
                    self.logger.info("Reverting back container data")
                    if len(container_entries) != 0:
                        cont_records = []
                        for cont_record_list in container_entries.values():
                            cont_records = cont_records + cont_record_list
                        self.con_op.revert_back_cont_data(cont_records, True)
                    if len(transaction_entries) != 0:
                        self.logger.info("Reverting back transaction data")
                        trans_records = []
                        for trans_record_list in transaction_entries.values():
                            trans_records = trans_records + trans_record_list
                        self.trans_mgr.revert_back_trans_data(trans_records, True)
                    self.logger.info("Finishing Transfer Component for Container server id : %s" % self.__service_id)
                    resp_obj = TransferCompResponseMessage(final_transferred_map, final_transfer_status)
                    message = resp_obj.serialize()
                    resp_headers = {'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE}
                    # return HTTP response
                    return HTTPOk(
                                 body=message,
                                 request=req,
                                 headers=resp_headers
                                 )
            except Exception as ex:
                self.logger.error("Exception raised: %s" % ex)
                return HTTPInternalServerError(request = req)
            except Timeout as ex:
                self.logger.info("Timeout occured : %s" % ex)
                self.logger.exception(
                    _('ERROR Exception causing client disconnect'))
                conn.close()
                return HTTPInternalServerError(request = req)

            self.logger.info("Sent trans data if any")
            #getting response from dest services
            pile = GreenAsyncPile(len(conns))
            for conn in conns:
                self.logger.info("In component transfer, Getting http responses from destination servers")
                pile.spawn(self.get_conn_response, conn)

            for (response, conn) in pile:
                if response:
                    self.logger.info("In component transfer, response status for destination server %s : %s" % (conn.target_service, conn.status))
                    if conn.status == True:
                        if response.status == 200:
                            # send intermediate status to GL
                            self.logger.info("Send intermediate state to GL for target service id: %s" % conn.target_service)
                            comp_service = self.service_component_map.get("%s:%s" % \
                                      (conn.node['ip'], conn.node['port']))
                            tuple_list = []
                            for comp in comp_transfer_map[comp_service]:
                                tuple_list.append((comp, comp_service))
                            list_of_tuple.put(tuple_list)
                            final_transferred_map.append((comp, True))
                        else:
                            for comp in comp_transfer_map[self.service_component_map[conn.target_service]]:
                                final_transferred_map.append((comp, False))
                            final_transfer_status = False
                            del comp_transfer_map[self.service_component_map[conn.target_service]]
                            self.con_op.revert_back_cont_data(dict_container[conn.target_service], True)
                            self.trans_mgr.revert_back_trans_data(dict_transaction[conn.target_service], True)
                            #del dict_transaction[conn.target_service]
                            #del dict_container[conn.target_service]
            while list_of_tuple.qsize() != 0:
                    self.logger.debug("list_of_tuple size: %s" % list_of_tuple.qsize())
                    eventlet.sleep(0.5)
            finish_transfer_component[0] = False
            self.logger.info("Finish Transfer Component for Container server id : %s" % self.__service_id)
            resp_obj = TransferCompResponseMessage(final_transferred_map, final_transfer_status)
            message = resp_obj.serialize()
	    resp_headers = {'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE}
            # return HTTP response 
            return HTTPOk(
                         body=message,
                         request=req,
                         headers=resp_headers
                    )

        except Exception as err:
            self.logger.exception(err)
            self.logger.debug('Exception raised in TRANSFER_COMPONENTS')
            finish_transfer_component = [False]
            return HTTPInternalServerError(request=req)

    def _connect_target_node(self, target_service, logger_thread_locals, method, headers):
        try:
            self.logger.info("connecting target node : %s" %method)
            conn_timeout = float(self.conf.get('CONNECTION_TIMEOUT', 30))
            node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
            headers['Expect'] = '100-continue'
            headers['X-Timestamp'] = time.time()
            headers['Content-Type'] = 'text/plain'
            headers['X-Object-Gl-Version-Id'] = self._object_major_version_x
            target_ip, target_port = target_service.split(':')
            with ConnectionTimeout(conn_timeout):
                conn = http_connect(
                         target_ip, target_port,"export", "OSP", method, self.__service_id, headers)
                self.logger.info("http connection complete")
                conn.node = {}
                conn.node['ip'] = target_ip
                conn.node['port'] = target_port
            with Timeout(node_timeout):
                self.logger.info("expecting 100-Continue header")
                resp = conn.getexpect()
                self.logger.info("got 100-Continue header")
            if resp.status == HTTP_CONTINUE:
                conn.resp = None
                conn.target_service = target_service
                self.logger.info("HTTP continue %s" % resp.status)
                return conn
            elif is_success(resp.status):
                conn.resp = resp
                conn.target_service = target_service
                self.logger.info("Successfull status:%s" % resp.status)
                return conn
            elif resp.status == HTTP_INSUFFICIENT_STORAGE:
                self.logger.error(_('%(msg)s %(ip)s:%(port)s'),
                      {'msg': _('ERROR Insufficient Storage'),
                      'ip': target_ip,
                      'port': target_port})
        except (Exception, Timeout) as err:
            self.logger.exception(
                _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                '%(info)s'),
                {'type': "Container", 'ip': target_ip, 'port': target_port, #check
                'info': "Expect: 100-continue on "})
    
    def __send_data(self, conn):
        """Method for sending data"""
        self.logger.info("send_data started")
        node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
        bytes_transferred = 0
        chunk = ""
        while conn.reader:
            chunk = conn.reader[:65536]
            bytes_transferred += len(chunk)

            if not conn.failed:
                try:
                    with ChunkWriteTimeout(node_timeout):
                        self.logger.debug('Sending chunk%s' % chunk)
                        conn.send(chunk)
                        conn.reader = conn.reader[65536:]
                except (Exception, ChunkWriteTimeout):
                    conn.failed = True
                    self.logger.exception(
                        _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                        '%(info)s'),
                    {'type': "Container", 'ip': conn.target_service.split(':')[0],
		    'port': conn.target_service.split(':')[1],
                    'info': "send data failed "})

    def get_conn_response(self, conn):
        node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
        try:
            with Timeout(node_timeout):
                if conn.resp:
                    self.logger.info("conn.resp returned")
                    conn.status = True
                    return (conn.resp, conn)

                else:
                    self.logger.info("conn.getresponse()")
                    conn.status = True
                    return (conn.getresponse(), conn)
        except (Exception, Timeout):
            conn.status = False
            self.logger.exception(_("connection: %(conn)s get_put_response: %(status)s" ),
                {'conn': conn.node,
                'status': _('Trying to get final status ')})
            return (conn.resp, conn)


    @public
    @timing_stats()
    def STOP_SERVICE(self, req):
        try:
            status = self.con_op.flush_all_data()
            resp = self.__check_status(status, req)
            if resp:   
                return resp
        except Exception as err:
            self.logger.exception(err)
            self.logger.debug('Exception raised in STOP_SERVICE')
            return HTTPInternalServerError(request = req, \
                            headers = {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK})

    @public
    @timing_stats()
    def BULK_DELETE(self, req):
        """
       Handle HTTP BULK_DELETE request.
       """
        req_trans_id = req.headers.get('x-trans-id', '-')
        self.logger.debug(('Req headers got in DELETE:- %(headers)s'
            ' with transaction ID :- %(id)s'),
            {'headers' : req.headers, 'id' : req_trans_id})
        hash_cont = ''
        failed_files = []
        obj_string = ""
        for chunk in iter(lambda: req.environ['wsgi.input'].read(\
        REQ_BODY_CHUNK_SIZE),''):
            obj_string += chunk
        object_names = eval(obj_string)
        self.logger.debug("Received obj list: %s" % object_names)
        try:
            filesystem, acc_dir, cont_dir, account, container = \
                split_and_validate_path(req, 4, 5, True)
            if 'x-timestamp' not in req.headers or \
                not check_float(req.headers['x-timestamp']):
                return HTTPBadRequest(body='Missing timestamp', request=req,
                    content_type='text/plain')
            if self.__mount_check and not check_mount(self.__root, filesystem):
                return HTTPInsufficientStorage(drive=filesystem, request=req)
            if container:
                hash_cont = hash_path(account, container)
                self.logger.debug(('BULK DELETE initiated for'
                    ' container:- %(container)s with transaction id:- %(id)s'),
                    {'container' : container, 'id' : req_trans_id})
                self.logger.debug("Going to aquire lock for account list")
                status = self.trans_mgr.acquire_bulk_delete_lock \
                         (filesystem, acc_dir, cont_dir, account, \
                         container, object_names)
                ret_val = status.get_value()
                resp_dict = {'Response Status': HTTPOk().status,
                'Response Body': '',
                'Number Deleted': 0,
                'Number Not Found': 0}
                if ret_val == 0:
                    self.logger.debug("Lock acquired")
                    status = self.con_op.bulk_delete_object(filesystem, \
                    acc_dir, cont_dir, account, container, object_names, \
                    req, resp_dict)
                    resp = self.__check_status(status, req)
                    if resp:
                        resp_dict['Response Body'] = 'BULK DELETE Intiated'
                else:
                    self.logger.error("Lock failed")
                    if ret_val == -1:
                        resp = HTTPConflict(request=req)
                        resp_dict['Response Status'] = HTTPConflict().status
                        for obj in object_names:
                            failed_files.append([quote(obj), \
                            HTTPConflict().status])
                    elif ret_val == -2:
                        resp = HTTPInsufficientStorage(request=req)
                        resp_dict['Response Status'] = \
                        HTTPInsufficientStorage().status
                        for obj in object_names:
                            failed_files.append([quote(obj), \
                            HTTPInsufficientStorage().status])
                    else:
                        resp = HTTPInternalServerError(request=req)
                        resp_dict['Response Status'] = \
                        HTTPInternalServerError().status
                        for obj in object_names:
                            failed_files.append([quote(obj), \
                            HTTPInternalServerError().status])
        except HTTPException as error:
            self.logger.exception(error)
            self.__revert_cont_changes(hash_cont)
            self.logger.debug(('Conflict occurred in DELETE '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            for obj in object_names:
                failed_files.append([quote(obj), HTTPConflict().status])
            resp = HTTPConflict(request=req)
        except Exception as err:
            self.logger.exception(err)
            self.__revert_cont_changes(hash_cont)
            self.logger.debug(('Exception raised in DELETE '
                'with transaction id:- %(id)s'), {'id' : req_trans_id})
            resp = HTTPInternalServerError(request=req)
        if 'X-Accept' in req.headers:
            self.logger.debug("input_content_type is: %s" % req.headers['X-Accept'])
        out_content_type = req.accept.best_match(ACCEPTABLE_FORMATS)
        if not out_content_type:
            self.logger.debug("content type is not given, set it to text/plain")
            out_content_type = 'text/plain'
        resp.content_type = out_content_type
        self.logger.debug("Response content type is: %s" % resp.content_type)
        resp.headers['transfer-encoding'] = 'chunked'
        resp.app_iter = bulk_delete_response(out_content_type, \
        resp_dict, failed_files)
        return resp


    def __call__(self, env, start_response):
        start_time = time.time()
        req = Request(env)
        map_version =  req.headers.get('x-global-map-version'.upper())
        component_number = req.headers.get('x-component-number')
        if map_version:
            map_version = float(req.headers.get('x-global-map-version'.upper()))
            self.logger.debug("map_version : %s" % map_version)
        self.logger.info("Headers received: %s " % req.headers)
        self.logger.txn_id = req.headers.get('x-trans-id', None)
        if not check_utf8(req.path_info):
            res = HTTPPreconditionFailed(body='Invalid UTF8 or contains NULL')
        elif not map_version:
            res = HTTPPreconditionFailed(body="x-global-map-version not defined in headers")
        else:
            try:
                # disallow methods which have not been marked 'public'
                try:
		    #if not component_number:
		    #res = HTTPPreconditionFailed(body="x-global-map-version not defined in headers")
                    method = getattr(self, req.method)
                    getattr(method, 'publicly_accessible')
                    map_status = 0
                    block_request = 0
                    if (map_version != -1):
                        map_status = self.__check_map_version(map_version)
                        self.logger.debug("Map status is: %s" % map_status)
                    component_number = req.headers.get('x-component-number')
                    if component_number:
                        block_request = self.__check_block_status(component_number)
                except AttributeError:
                    res = HTTPMethodNotAllowed()
                else:
                    if map_status == 1:
                        res = HTTPMovedPermanently(body='Old map version in the request')
                    elif map_status == 2:
                        self.logger.info("Version at CS level is small. Updating Global Map")
                        try:
                            self.global_map, self.global_map_version, self.conn_obj = get_global_map(self.__service_id, self.logger, req)
                        except ValueError as err:
                            self.logger.error("Global map could not be loaded, exiting now :%s "% err)
                            return HTTPInternalServerError(body="Map load error")(env, start_response)

                        major_version = self.get_major_version(self.global_map_version)
                        self.logger.info("Major Version at CS level : %s and New Major version : %s" %(self.global_map_major_version,major_version))
                        if major_version > self.global_map_major_version:
                            self.logger.info("Major Version changed Cleaning blocked component list")
                            base_version_changed = 1
                            self.__clean_block_component_list()
                            components = self.__get_my_components_from_map()
                            self.global_map_major_version = self.get_major_version(self.global_map_version)
                            #self.global_map_major_version = major_version
                        else:
                            self.logger.info("Major Version did not changed")
                            base_version_changed = 0
                            components = self.__get_my_components_from_map()
                        self.trans_mgr.commit_recovered_data(components, base_version_changed)
                        self.con_op.commit_recovered_data(components, base_version_changed)
                        if method in ['PUT', 'GET', 'DELETE', 'HEAD', 'POST', 'GET_LOCK', 'FREE_LOCK']:
                            if (not component_number) and (map_version != -1):
                                res = HTTPBadRequest(body='Missing Component Number', request=req,
                                                                          content_type='text/plain')
                        if block_request:
                            self.logger.info("Blocking request for component %s" % component_number)
                            res = HTTPTemporaryRedirect(body='Component transfer is going on for the requested component')
                        else:
                            if component_number:
                                self.__addition_in_comp_request_dict(component_number)
                            res = method(req)
                    elif block_request:
                        self.logger.info("Blocking request for component %s" % component_number)
                        res = HTTPTemporaryRedirect(body='Component transfer is going on for the requested component')
                    else:
                        if method in ['PUT', 'GET', 'DELETE', 'HEAD', 'POST', 'GET_LOCK', 'FREE_LOCK']:
                            if (not component_number) and (map_version != -1):
                                res = HTTPBadRequest(body='Missing Component Number', request=req,
                                                                          content_type='text/plain')
                        if component_number:
                            self.__addition_in_comp_request_dict(component_number)
                            #components = self.__get_my_components_from_map()
                            #if int(component_number) in components:
                            #    res = method(req)
                            #else:
                            #    res = HTTPMovedPermanently(body='Component is not in my ownership')
                        res = method(req)
            except HTTPException as error_response:
                res = error_response
            except (Exception, Timeout):
                self.logger.exception((
                    'ERROR __call__ error with %(method)s %(path)s '),
                    {'method': req.method, 'path': req.path})
                res = HTTPInternalServerError(body=traceback.format_exc())
        trans_time = '%.4f' % (time.time() - start_time)
        pid = os.getpid()
        self.logger.debug(('Request came to container service with pid '
            ': %(id)s'), {'id' : pid})
        if self.log_requests:
            log_message = '%s - - [%s] "%s %s" %s %s "%s" "%s" "%s" %s' % (
                req.remote_addr,
                time.strftime('%d/%b/%Y:%H:%M:%S +0000',
                              time.gmtime()),
                req.method, req.path,
                res.status.split()[0], res.content_length or '-',
                req.headers.get('x-trans-id', '-'),
                req.referer or '-', req.user_agent or '-',
                trans_time)
            self.logger.info(log_message)
        if component_number:
            self.__deletion_in_comp_request_dict(component_number)
        return res(env, start_response)



def global_conf_callback(preloaded_app_conf, global_conf):
    """
    Callback for osd.common.wsgi.run_wsgi during the global_conf
    creation so that we can prevent recovery process across multiple initialization
    of server 

    :param preloaded_app_conf: The preloaded conf for the WSGI app.
                               This conf instance will go away, so
                               just read from it, don't write.
    :param global_conf: The global conf that will eventually be
                        passed to the app_factory function later.
                        This conf is created before the worker
                        subprocesses are forked, so can be useful to
                        set up semaphores, shared memory, etc.
    """
    recovery_status = RecoveryStatus()
    if 'recovery_status'  not in global_conf:
        global_conf['recovery_status'] = [recovery_status]


def app_factory(global_conf, **local_conf):
    """paste.deploy app factory for creating WSGI container server apps"""
    conf = global_conf.copy()
    conf.update(local_conf)
    return ContainerController(conf)
