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

import sys
import os
import time
import traceback
import socket
import subprocess
import pickle
from uuid import uuid4
from eventlet import Timeout, sleep
from osd import gettext_ as _
from osd.common.http import HTTP_CONTINUE 
from osd.common.utils import GreenthreadSafeIterator, \
    GreenAsyncPile, float_comp
from osd.common.bufferedhttp import http_connect
import libraryUtils
import libaccountLib
from osd.common.return_code import *
from osd.common.request_helpers import get_param
from osd.accountService.account_service_interface import \
    AccountServiceInterface
from osd.common.request_helpers import get_listing_content_type, \
    split_and_validate_path
from osd.common.utils import get_logger, config_true_value, public, \
    timing_stats, create_recovery_file, remove_recovery_file
from osd.common.constraints import check_mount, check_float, check_utf8, \
    ACCOUNT_LISTING_LIMIT
from osd.common.swob import HTTPAccepted, HTTPBadRequest, \
    HTTPCreated, HTTPInternalServerError, HTTPNotFound, \
    HTTPMethodNotAllowed, HTTPNoContent, HTTPPreconditionFailed, Request, \
    HTTPInsufficientStorage, HTTPException, HTTPForbidden, \
    HTTPOk, HTTPMovedPermanently, HTTPTemporaryRedirect
from osd.common.exceptions import ChunkWriteTimeout, ConnectionTimeout
from osd.glCommunicator.communication.osd_exception import \
    FileNotFoundException, FileCorruptedException
from osd.common.ring.ring import hash_path
from osd.accountService.utils import get_filesystem, SingletonType, \
    temp_clean_up_in_comp_transfer
from osd.glCommunicator.req import Req
from libHealthMonitoringLib import healthMonitoring
from osd.glCommunicator.communication.communicator import IoType
from osd.glCommunicator.response import Resp
from osd.glCommunicator.messages import TransferCompMessage, \
    TransferCompResponseMessage 
from osd.glCommunicator.message_type_enum_pb2 import *


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

class AccountController(object):
    """WSGI controller for the account server."""

    __metaclass__ = SingletonType

    def __init__(self, conf, logger=None):
        self.logger = logger or get_logger(conf, log_route='account-server')
        self.conf = conf
        libraryUtils.OSDLoggerImpl("account-library").initialize_logger()
        self.log_requests = config_true_value(conf.get('log_requests', 'true'))
        self.__port = int(conf.get('bind_port', 61006))
        self.__ll_port = int(conf.get('llport', 61014))
        self._base_fs = conf.get('base-fs', 'OSP_01')
        self._server = 'account'
        self._service_id = socket.gethostname() + "_" + str(self.__ll_port) \
        + "_account-server"
       
        self.root = conf.get('filesystems', '/export')
        self.mount_check = config_true_value(conf.get('mount_check', 'true'))
        self.auto_create_account_prefix = \
            conf.get('auto_create_account_prefix') or '.'


        # status list
        self._network_fail_status_list = [Resp.BROKEN_PIPE, \
        Resp.CONNECTION_RESET_BY_PEER, Resp.INVALID_SOCKET]
        self._success_status_list = [Resp.SUCCESS]

        # communicator and con object creation
        self._communicator_obj = Req(self.logger)

        # get account server ownership
        self._acc_ownership_map = list()
        status = self.__get_my_ownership()
        if not status:
            sys.exit(130)
        # dict for block request flag and on 
        # going count {'compid' : [flag, req_count]}
        self._comp_flag_req_count_map = dict()
        self.__update_blocking_flag_map()

        #load gl info
        self._global_map = list()
        self._global_map_version = None
        status = self.__load_gl_map_info()
        if not status:
            sys.exit(130)
 
        # set account map version
        self._acc_version = None
        self._acc_service_obj_map = list()
        self.__set_acc_version_from_gmap()

        #Start account recovery
        if 'recovery_status' in conf:
            recovery_status = conf['recovery_status'][0].get_status()
            if not recovery_status:
                ret = self.__account_recovery()
                if not ret:
                    self.logger.debug("*** Removing recovery file ***")
                    remove_recovery_file('account-server')
                    sys.exit(130)
                conf['recovery_status'][0].set_status(True)

        #start heart beats with local leader
        self.health_instance = healthMonitoring(self.__get_node_ip(\
        socket.gethostname()), self.__port, self.__ll_port, self._service_id)
        self.logger.debug("*** Removing recovery file ***")
        remove_recovery_file('account-server')
        self.logger.info("*** Account Service Started ***")

    def add_green_thread(self):
        AccountServiceInterface.start_event_wait_func(self.logger)

    def __del__(self):
        AccountServiceInterface.stop_event_wait_func(self.logger)

    def __get_node_ip(self, hostname):
        """
        Get internal node ip on which 
        service is running
        """
        try:
            command = "grep -w " + hostname + " /etc/hosts | awk {'print $1'}"
            child = subprocess.Popen(command, stdout = subprocess.PIPE, \
            stderr = subprocess.PIPE, shell = True)
            std_out, std_err = child.communicate()
            return std_out.strip()
        except Exception as err:
            self.logger.error("Error occurred while getting ip for node: %s" \
            % hostname)
            self.logger.error(err)
            return ""

    def __get_major_version(self, version):
        tmp = str(version).split('.')
        version = tmp[0]
        return int(version)

    def __create_con_obj(self):
        try:
            gl_info_obj = self._communicator_obj.get_gl_info()
        except FileNotFoundException, FileCorruptedException:
            return None
        return self._communicator_obj.connector(IoType.EVENTIO, gl_info_obj)

    def __load_gl_map_info(self):
        retry = 3
        counter = 0
        while counter < retry:
            con_obj = self.__create_con_obj()
            if con_obj:
                ret = self._communicator_obj.get_global_map(\
                self._service_id, con_obj)
                if ret.status.get_status_code() \
                    in self._success_status_list:
                    self._global_map = ret.message.map()
                    self._global_map_version = ret.message.version()
                    return True
                self.logger.warn("get_global_map error code: %s msg: %s" \
                % (ret.status.get_status_code(), \
                ret.status.get_status_message()))
                con_obj.close()
            self.logger.warn("Failed to get global map, Retry: %s" % counter)
            counter += 1
        self.logger.error("Failed to laod global map")
        return False
                

    def __get_my_ownership(self):
        retry = 3
        counter = 0
        while counter < retry:
            con_obj = self.__create_con_obj()
            if con_obj:
                ret = self._communicator_obj.get_comp_list(\
                self._service_id, con_obj)
                if ret.status.get_status_code() in self._success_status_list:
                    self._acc_ownership_map = ret.message  # message itself contains list
                    return True
                self.logger.warn("get_comp_list() error code: %s msg: %s" \
                % (ret.status.get_status_code(), \
                ret.status.get_status_message()))
                con_obj.close()
            self.logger.warn("Failed to get comp list, Retry: %s" % counter)
            counter += 1
        self.logger.error("Failed to get ownership components")
        return False

    def __clear_blocked_list(self):
        for comp_id in self._comp_flag_req_count_map.keys():
            self._comp_flag_req_count_map[comp_id][0] = False

    def __update_blocking_flag_map(self):
        #add new component to map
        for comp_id in self._acc_ownership_map:
            if comp_id not in self._comp_flag_req_count_map:
                self._comp_flag_req_count_map[comp_id] = [False, 0]
        #remove component from map
        for comp_id in self._comp_flag_req_count_map.keys():
            if comp_id not in self._acc_ownership_map:
                del self._comp_flag_req_count_map[comp_id]


    def __set_acc_version_from_gmap(self):
        self._acc_service_obj_map = self._global_map['account-server'][0] # [serv obj1, obj2]
        self._acc_version = self._global_map['account-server'][1]
        
    def __get_my_ownership_from_gmap(self):
        self.__set_acc_version_from_gmap()
        component_list = [index for index, service_obj in \
        enumerate(self._acc_service_obj_map) \
        if service_obj.get_id() == self._service_id]
        return component_list        

    def _respond_deleted(self, req, resp, body=''):
        headers = {'X-Account-Status': 'Deleted'}
        return resp(request=req, headers=headers, charset='utf-8', body=body)

    def __log_lib_response(self, method, return_code):
        self.logger.info("Account library responded with: %s for %s" \
            % (return_code, method))

    @public
    @timing_stats()
    def DELETE(self, req):
        """Handle HTTP DELETE request."""
        filesystem, directory, account = split_and_validate_path(req, 3, 3)
        self.logger.debug("Deleting account: %s" % account)
        if 'x-timestamp' not in req.headers or \
            not check_float(req.headers['x-timestamp']):
            return HTTPBadRequest(body='Missing timestamp', request=req,
                content_type='text/plain')

        account_path = os.path.join(self.root, filesystem, directory, \
            hash_path(account))
        self.logger.debug("Deleting account path: %s" % account_path)
        temp_path = os.path.join(self.root, self._base_fs, 'tmp', \
			self._server, req.headers['X-COMPONENT-NUMBER'])
        status = AccountServiceInterface.delete_account(temp_path, \
            account_path, self.logger)
        self.__log_lib_response('DELETE', status.get_return_status())
        if status.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
            self.logger.info("Account %s deleted" % account)
            return self._respond_deleted(req, HTTPNoContent)
        elif status.get_return_status() == INFO_FILE_DELETED:
            return self._respond_deleted(req, HTTPNotFound)
        elif status.get_return_status() == INFO_FILE_NOT_FOUND:
            return HTTPNotFound(request=req, charset='utf-8')
        else:
            self.logger.warn("Could not delete account: %s" %account)
            return HTTPInternalServerError(request=req)


    def _validate_container_map(self, container_map):
        """Validate to container map, if a container 
           map hold data about single container."""
        if len(container_map) == 1:
            return True
        return False

    def split_and_validate_container_info_list(self, container_info_list):
        "split to container info list, and validate."
        container_hash_list = []
        container_data_list = libaccountLib.list_less__recordStructure_scope_ContainerRecord__greater_()
        self.logger.debug("Iterate container list")
        for container_map in container_info_list:
            if self._validate_container_map(container_map):
                for container_hash, container_data in container_map.iteritems():
                    container_hash_list.append(container_hash)
                    container_data_list.append(\
                        AccountServiceInterface.create_container_record_for_updater(\
                        container_data['container'], container_hash, \
                        container_data))
        return container_hash_list, container_data_list


    @public
    @timing_stats()
    def PUT(self, req):
        """Handle HTTP PUT request."""
        filesystem, directory, account, container = \
                                split_and_validate_path(req, 3, 4)
        self.logger.debug("Put called for Account: %s" % account)
        if self.mount_check and not check_mount(self.root, filesystem):
            self.logger.warn("Filesystem %s is not mounted" % filesystem)
            return HTTPInsufficientStorage(filesystem=filesystem, request=req)
        if container:   # put account container
            account_path = os.path.join(self.root,
                                        filesystem,
                                        directory,
                                        hash_path(account)
                                        )
            self.logger.debug("PUT request from container server for account \
                path: %s" % account_path)
            if req.headers['x-put-timestamp'] > \
            req.headers['x-delete-timestamp']:
                try:
                    self.logger.debug("Request to add container: %s" \
                        % container)
                    status = AccountServiceInterface.add_container(
                            account_path,
                            AccountServiceInterface.create_container_record(\
                                container, hash_path(account, container), \
                                req.headers, False), self.logger)
                    self.__log_lib_response('Container PUT', \
                    status.get_return_status())
                except Exception, ex:
                    self.logger.error("Exception while adding container in \
                                                            account: %s" % ex)
                    return HTTPInternalServerError(request=req)
                if status.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
                    self.logger.debug("Container %s added successfully" \
                        % container)
                    return HTTPAccepted(request=req)
                elif status.get_return_status() == INFO_FILE_NOT_FOUND:
                    self.logger.debug("Account path %s doesn't exist for \
                        container %s" % (account_path, container))
                    return HTTPNotFound(request=req)
            else:
                try:
                    self.logger.debug("Request to delete container: %s" \
                        % container)
                    status = AccountServiceInterface.delete_container(
                            account_path,
                            AccountServiceInterface.create_container_record(\
                                container, hash_path(account, container), \
                                req.headers, True), self.logger)
                    self.__log_lib_response('Container DELETE', \
                    status.get_return_status())
                except Exception, ex:
                    self.logger.error("Exception while deleting container in \
                                                            account: %s" % ex)
                    return HTTPInternalServerError(request=req)
                if status.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
                    self.logger.debug("Container %s deleted successfully" \
                        % container)
                    return HTTPNoContent(request=req)
                elif status.get_return_status() == INFO_FILE_NOT_FOUND:
                    self.logger.debug("Account path %s doesn't exist for \
                        container %s" % (account_path, container))
                    return HTTPNotFound(request=req)
        else:
            info = {
                    'account': account,
                    'created_at': str(time.time()),
                    'id': str(uuid4()),
                    'put_timestamp': req.headers['x-timestamp'],
                    'delete_timestamp': '0',
                    'container_count': 0,
                    'object_count': 0,
                    'bytes_used': 0,
                    'hash': '00000000000000000000000000000000',
                    'status': '',
                    'status_changed_at': '0',
                    'metadata': AccountServiceInterface.get_metadata(\
                    req, self.logger)
                }
            account_path = os.path.join(self.root,
                                        filesystem,
                                        directory,
                                        hash_path(account)
                                        )
            try:
                temp_path = os.path.join(self.root, self._base_fs, 'tmp', \
                    self._server, req.headers['X-COMPONENT-NUMBER'])
                status = AccountServiceInterface.create_account(temp_path, \
                account_path, \
                AccountServiceInterface.create_AccountStat_object(info), \
                self.logger)
                self.__log_lib_response('PUT', status.get_return_status())
            except Exception, ex:
                self.logger.error("Exception while creating account: %s" % ex)
                return HTTPInternalServerError(request=req)
            if status.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
                self.logger.info("Account %s created successfully" % account)
                return HTTPCreated(request=req)
            elif status.get_return_status() == INFO_FILE_ACCEPTED:
                self.logger.info("Account %s already exist" % account)
                return HTTPAccepted(request=req)
            elif status.get_return_status() == INFO_FILE_MAX_META_COUNT_REACHED:
                self.logger.debug('Too many metadata items found')
                return HTTPBadRequest(body='Too many metadata items', \
                request=req)
            elif status.get_return_status() == INFO_FILE_MAX_META_SIZE_REACHED:
                self.logger.debug('Max metadata size reached')
                return HTTPBadRequest(body='Max metadata size reached', \
                       request=req)
            elif status.get_return_status() == INFO_FILE_MAX_ACL_SIZE_REACHED:
                self.logger.debug('Max ACL size reached')
                return HTTPBadRequest(body='Max ACL size reached', \
                       request=req)
            elif status.get_return_status() == INFO_FILE_MAX_SYSTEM_META_SIZE_REACHED:
                self.logger.debug('Max sys metadata size reached')
                return HTTPBadRequest(body='Max sys metadata size reached', \
                       request=req)

        if status.get_return_status() == INFO_FILE_DELETED:
            self.logger.debug("Account is deleted")
            return self._respond_deleted(req,
                                        HTTPForbidden,
                                        body ='Recently deleted')
        elif status.get_return_status() == INFO_FILE_MOUNT_CHECK_FAILED:
            self.logger.warn("Filesystem %s is not mounted" % filesystem)
            return HTTPInsufficientStorage(filesystem=filesystem, \
                request=req)
        return  HTTPInternalServerError(request=req)


    @public
    @timing_stats()
    def HEAD(self, req):
        """Handle HTTP HEAD request."""
        filesystem, directory, account = split_and_validate_path(req, 3)
        self.logger.debug("HEAD called for account %s" % account)
        out_content_type = get_listing_content_type(req)
        headers = {}
        if self.mount_check and not check_mount(self.root, filesystem):
            self.logger.warn("filesystem %s is not mounted" % filesystem)
            return HTTPInsufficientStorage(filesystem=filesystem, request=req)
        account_path = os.path.join(self.root, filesystem, directory, \
            hash_path(account))
        try:
            temp_path = os.path.join(self.root, self._base_fs, 'tmp', \
            self._server, req.headers['X-COMPONENT-NUMBER'])
            status, info = AccountServiceInterface.get_account_stat(\
                               temp_path, account_path, self.logger)
            self.__log_lib_response('HEAD', status)
        except Exception, ex:
            self.logger.error("Exception while getting account stat: %s" % ex)
            return HTTPInternalServerError(request=req)
        if status == INFO_FILE_NOT_FOUND:
            return HTTPNotFound(request=req, charset='utf-8')
        elif status == INFO_FILE_DELETED:
            return self._respond_deleted(req, HTTPNotFound)
        elif status == INFO_FILE_MOUNT_CHECK_FAILED:
            self.logger.warn("Filesystem %s is not mounted" % filesystem)
            return HTTPInsufficientStorage(filesystem=filesystem, request=req)
        elif status == INFO_FILE_INTERNAL_ERROR or \
            status == INFO_FILE_OPERATION_ERROR:
            self.logger.error("Internal error in HEAD account %s" % account)
            return HTTPInternalServerError(request=req)
        if status == INFO_FILE_OPERATION_SUCCESS:
            headers = {
            'X-Account-Container-Count': info['container_count'],
            'X-Account-Object-Count': info['object_count'],
            'X-Timestamp': info['created_at'],
            'X-Account-Bytes-Used': info['bytes_used'],
            'X-PUT-Timestamp': info['put_timestamp']
            }
            headers.update((key, value)
            for (key, value) in
            info['metadata'].iteritems())
        headers['Content-Type'] = out_content_type
        self.logger.debug("Account service responding HEAD request with headers: %s" % headers)
        return HTTPNoContent(request=req, headers=headers, charset='utf-8')



    @public
    @timing_stats()
    def GET(self, req):
        """Handle HTTP GET request."""
        filesystem, directory, account = split_and_validate_path(req, 3)
        self.logger.debug("GET called for account %s" % account)
        prefix = get_param(req, 'prefix', '')
        delimiter = get_param(req, 'delimiter', '')
        if delimiter and (len(delimiter) > 1 or ord(delimiter) > 254):
            # delimiters can be made more flexible later
            return HTTPPreconditionFailed(body='Bad delimiter')
        limit = ACCOUNT_LISTING_LIMIT
        given_limit = get_param(req, 'limit')
        if given_limit and given_limit.isdigit():
            limit = int(given_limit)
            if limit > ACCOUNT_LISTING_LIMIT:
                return HTTPPreconditionFailed(request=req,
                                              body='Maximum limit is %d' %
                                              ACCOUNT_LISTING_LIMIT)
        marker = get_param(req, 'marker', '')
        end_marker = get_param(req, 'end_marker', '')
        out_content_type = get_listing_content_type(req)
        if self.mount_check and not check_mount(self.root, filesystem):
            self.logger.warn("filesystem %s is not mounted" % filesystem)
            return HTTPInsufficientStorage(filesystem=filesystem, request=req)
        account_path = os.path.join(self.root, filesystem, directory, \
            hash_path(account))
        self.logger.debug("Account GET calling list_account for path: %s" \
            % account_path)
        temp_path = os.path.join(self.root, self._base_fs, 'tmp', \
        self._server, req.headers['X-COMPONENT-NUMBER'])
        return AccountServiceInterface.list_account(temp_path, account_path, \
            out_content_type, req, limit, marker, end_marker, prefix, \
            delimiter, self.logger)

    @public
    @timing_stats()
    def POST(self, req):
        """Handle HTTP POST request."""
        filesystem, directory, account = split_and_validate_path(req, 3, 3)
        self.logger.debug("POST called for account %s" % account)
        if 'x-timestamp' not in req.headers or \
            not check_float(req.headers['x-timestamp']):
            return HTTPBadRequest(body='Missing or bad timestamp',
                    request=req, content_type='text/plain')
        account_path = os.path.join(self.root, filesystem, directory, \
            hash_path(account))
        try:
            info = {'metadata' : AccountServiceInterface.get_metadata( \
                                                             req, \
                                                             self.logger)}
            acc_stat_obj = AccountServiceInterface.create_AccountStat_object(info)
            status = AccountServiceInterface.set_account_stat(account_path, \
                acc_stat_obj, \
                self.logger)
            self.logger.debug("Account library responded for method: \
                set_account_stat with return code: %s" \
                % status.get_return_status())
        except Exception, ex:
            self.logger.error("Exception: %s" % ex)
            return HTTPInternalServerError(request=req)
        if status.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
            self.logger.debug("POST for account %s successfull" % account)
            return HTTPNoContent(request=req)
        elif status.get_return_status() == INFO_FILE_NOT_FOUND:
            self.logger.debug("Account %s is not found for POST" % account)
            return HTTPNotFound(request=req)
        elif status.get_return_status() == INFO_FILE_MAX_META_COUNT_REACHED:
            self.logger.debug('Too many metadata items found')
            return HTTPBadRequest(body='Too many metadata items', \
                request=req)
        elif status.get_return_status() == INFO_FILE_MAX_META_SIZE_REACHED:
            self.logger.debug('Max metadata size reached')
            return HTTPBadRequest(body='Max metadata size reached', \
                request=req)
        elif status.get_return_status() == INFO_FILE_MAX_ACL_SIZE_REACHED:
            self.logger.debug('Max ACL size reached')
            return HTTPBadRequest(body='Max ACL size reached', \
                request=req)
        elif status.get_return_status() == \
        INFO_FILE_MAX_SYSTEM_META_SIZE_REACHED:
            self.logger.debug('Max sys metadata size reached')
            return HTTPBadRequest(body='Max sys metadata size reached', \
                request=req)
        self.logger.warn("Internal error in POST account %s" % account)
        return HTTPInternalServerError(request=req)


    @public
    @timing_stats()
    def PUT_CONTAINER_RECORD(self, req):
        """Handle HTTP PUT_CONTAINER_RECORD request."""
        try:
            account = req.headers['account']
            filesystem = req.headers['filesystem']
            directory = req.headers['dir']
        except Exception, ex:
            self.logger.error("Exception: %s" % ex)
        try:
            container_name_list, container_data_list = \
                self.split_and_validate_container_info_list(eval(req.body))
            account_path = os.path.join(
                                        self.root,
                                        filesystem,
                                        directory,
                                        account
                                        )
            self.logger.debug("Going to update containers for account \
                path: %s" % (account_path))
            status = AccountServiceInterface.update_container(
                                                account_path,
                                                container_data_list,
                                                self.logger
                                                )
            self.__log_lib_response('PUT_CONTAINER_RECORD', \
            status.get_return_status())
        except Exception, ex:
            self.logger.error("Exception: %s" % ex)
            return HTTPInternalServerError(request=req)
        if status.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
            self.logger.debug("PUT_CONTAINER_RECORD for account %s \
                successfull" % account)
            return HTTPAccepted(request=req)
        elif status.get_return_status() == INFO_FILE_NOT_FOUND:
            self.logger.debug("Account %s is not found for \
                PUT_CONTAINER_RECORD" % account)
            return HTTPNotFound(request=req)
        elif status.get_return_status == INFO_FILE_MOUNT_CHECK_FAILED:
            self.logger.debug("Filesystem %s is not mounted" % filesystem)
            return HTTPInsufficientStorage(filesystem=filesystem, request=req)
        self.logger.debug("Internal error in PUT_CONTAINER_RECORD account %s" \
            % account)
        return HTTPInternalServerError(request=req)

    @public
    @timing_stats()
    def ACCEPT_COMPONENTS(self, request):
        """Handle HTTP ACCEPT_COMPONENT_CONT_DATA request"""
        try:
            self.logger.info("ACCEPT_COMPONENTS interface is called")
            self.logger.debug("Req headers:%s" % request.headers)
            pickled_string = ""
            for chunk in iter(lambda: request.environ['wsgi.input'].read(65536),''):
                pickled_string += chunk
            list_received = pickle.loads(pickled_string)

            self.logger.debug("List Received in ACCEPT_COMPONENTS: %s" \
            % list_received)
            # gl version check and load new gl map is already done at __call__
            return  HTTPOk(request=request)
        except Exception as err:
            self.logger.exception(err)
            self.logger.error(('Exception raised in ACCEPT_COMPONENTS'
                'with component list :- %(comp_list)s'), {'comp_list' : list_received})
            return HTTPInternalServerError(request=request)

    @public
    @timing_stats()
    def TRANSFER_COMPONENTS(self, req):
        """Handle HTTP TRANSFER_COMPONENTS request."""
        self.logger.info("Transfer component called")
        try:
            # deserialize http req body
            msg = TransferCompMessage()
            msg.de_serialize(req.environ['wsgi.input'].read())
            comp_transfer_map = msg.service_comp_map()
            self.logger.debug("Map given by GL: %s" % comp_transfer_map)

            #Exit (Return HTTP OK) if comp_transfer_map is empty
            if not comp_transfer_map:
                self.logger.warn("Map is empty, return HTTP OK!")
                #Just responce success with empty list.
                resp_obj = TransferCompResponseMessage([], True)
                message = resp_obj.serialize()
                resp_headers = {'Message-Type' : \
                    typeEnums.TRANSFER_COMPONENT_RESPONSE}
                # return HTTP response
                return HTTPOk(
                             body=message,
                             request=req,
                             headers=resp_headers
                        )

        except Exception, ex:
            self.logger.error("Exception: %s" % ex)

        ongoing_operation_list = list()
        for dest_ser_obj, component_list in comp_transfer_map.iteritems():
            for component in component_list:
                self._comp_flag_req_count_map[component][0] = True
                if self._comp_flag_req_count_map[component][1] > 0:
                    ongoing_operation_list.append(component)

        # wait till completion of ongoing operation 
        while len(ongoing_operation_list) > 0:
            sleep(2)
            for comp in ongoing_operation_list:
                if self._comp_flag_req_count_map[comp][1] == 0:
                    ongoing_operation_list.remove(comp)
        self.logger.info("Ongoing requests completed")

        # temp cleanup
        pile_size = len(comp_transfer_map)
        pile =  GreenAsyncPile(pile_size)
        target_node_service_iter = GreenthreadSafeIterator(comp_transfer_map) #can be used with .values
        for target_service_obj in target_node_service_iter:
            pile.spawn(self.__transfer_ownership,
                       target_service_obj,
                       comp_transfer_map[target_service_obj],
                       self.logger.thread_locals)
            
        self.logger.debug("temp cleanup done, and notified to target node")

        final_status = True
        final_transfer_status_list = list()
        failed_target_server_list = list()
        # send intermediate response to GL
        for dest_serv_obj, component_list in pile:
            if component_list:
                status, glmap, glversion = self.__send_message_to_gl(\
                dest_serv_obj, component_list)
                self.logger.info("After sending intermediate response, Status: %s : gl version: %s" \
                % (status, glversion))
                if status:
                    final_transfer_status_list.extend([(compid, True) \
                    for compid in component_list])
                    # set global map and account map as per intermediate transfer
                    self._global_map = glmap
                    self._global_map_version = glversion
                    #self. __set_acc_version_from_gmap()
                    #self.__get_my_ownership_from_gmap()
                    self._acc_ownership_map = self.__get_my_ownership_from_gmap()
                    self.__update_blocking_flag_map()
                else:
                    final_transfer_status_list.extend([(compid, False) \
                    for compid in component_list])
                    failed_target_server_list.append(dest_serv_obj.get_id())
                    final_status = False
            else:
                failed_target_server_list.append(dest_serv_obj.get_id())

            final_transfer_status_list.extend([(compid, False) for compid in \
                set(comp_transfer_map[dest_serv_obj]) - set(component_list)])

        self.logger.info("gl requests failed for targets server ids: %s" \
        % failed_target_server_list)
        pile.waitall(30) 
        #send final state to GL
        self.logger.info("Sending final response to GL")
        comp_transfer_response = TransferCompResponseMessage(\
        final_transfer_status_list, final_status)
        serialized_body = comp_transfer_response.serialize()
        resp_headers = {'Message-Type' : typeEnums.TRANSFER_COMPONENT_RESPONSE}

        # return HTTP response 
        return HTTPOk(
                     body = serialized_body,
                     request=req,
                     headers=resp_headers
                     )

    def __clean_temp_data(self, target_service_obj, comp_list): 
        #self.logger.thread_locals = logger_thread_locals
        transfered_comp_list = list()
        for comp_id in comp_list:
            # temp cleanup
            temp_dir = os.path.join(self.root, self._base_fs, 'tmp')
            self.logger.debug("temp cleaning up for component: %s" % comp_id)
            status = temp_clean_up_in_comp_transfer(temp_dir, self._server, \
            comp_id, self.logger)
            if status:
                transfered_comp_list.append(comp_id)
        return (target_service_obj, transfered_comp_list)

    def __send_message_to_gl(self, target_service_obj, comp_list):
        # send intermediate status to GL
        self.logger.info("Send intermediate state to GL for target service id: %s" % target_service_obj.get_id())
        retry_count = 3
        counter = 0
        while counter < retry_count:
            con_obj = self.__create_con_obj()
            if con_obj:
                ret = self._communicator_obj.comp_transfer_info(\
                self._service_id, [(compid, target_service_obj) \
                for compid in comp_list], con_obj)
                if ret.status.get_status_code() in self._success_status_list:
                    self.logger.debug("Component transfer Completed at GL %s" \
                    % self._service_id)
                    return (True, ret.message.map(), ret.message.version())
                self.logger.warn("comp_transfer_info error code: %s msg: %s" \
                % (ret.status.get_status_code(), \
                ret.status.get_status_message()))
                con_obj.close()
            self.logger.warn("Failed to comp_transfer_info, Retry: %s" \
            % counter)
            counter += 1
        self.logger.error("Failed to send_message_to_gl")
        return (False, None, None)

    def __send_http_to_target(self, target_service, transfered_comp_list, \
                           logger_thread_locals):
        self.logger.thread_locals = logger_thread_locals
        method = "ACCEPT_COMPONENTS"
        http_con = self.__connect_target_node(target_service, \
        transfered_comp_list, logger_thread_locals, method)
        if http_con:
            self.logger.info("http connection for ACCEPT_COMPONENTS is established from source %s to target %s" \
                            % (self._service_id, target_service.get_id()))
            #serialize data
            # note : take care of transfered_comp_list in case of some components cleanup
            http_con.reader = pickle.dumps(transfered_comp_list)
            self.logger.debug("Comp list going to be transfer is: %s" % transfered_comp_list)
            self.__send_data(http_con, target_service)
            resp = http_con.getresponse()
            if resp.status == 200: #HTTPOk 
                self.logger.debug("ACCEPT_COMPONENTS request for target %s completed(HTTPOk received)" % target_service.get_id())
                return True
            else:
                self.logger.error("ACCEPT_COMPONENTS request failed for target %s , return status %s" \
                                                              % (target_service.get_id(), resp.status))
        else:
            self.logger.error("http connection for ACCEPT_COMPONENTS is failed from source %s to target %s" % (self._service_id, target_service.get_id()))
#        self.__clear_blocked_list()
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
            headers['X-GLOBAL-MAP-VERSION'] = self._global_map_version
            headers['Content-Length'] = len(pickle.dumps(comp_list))

            with ConnectionTimeout(conn_timeout):
                conn = http_connect(target_service.get_ip(), \
                target_service.get_port(), filesystem, directory, \
                method, path, headers)
            with Timeout(node_timeout):
                resp = conn.getexpect()
            if resp.status == HTTP_CONTINUE:
                conn.resp = None
                conn.target_service = target_service
                return conn
            else:
                self.logger.info("Http connection status: %s" % resp.status)
                return None
        except (Exception, Timeout) as err:
            self.logger.exception(
                _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                '%(info)s'),
                {'type': "Account", 'ip': target_service.get_ip(), 'port': \
                target_service.get_port(), #check
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
                    conn.send(chunk)
                    conn.reader = conn.reader[65536:]
            except (Exception, ChunkWriteTimeout):
                self.logger.exception(
                    _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                    '%(info)s'),
                    {'type': "Account", 'ip': service_obj.get_ip(), \
                    'port': service_obj.get_port(),
                    'info': "send data failed "})
        self.logger.info("Bytes transferred: %s" % bytes_transferred)

    def __get_conn_response(self, conn):
        node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
        try:
            with Timeout(node_timeout):
                if conn.resp:
                    self.logger.debug("conn.resp returned")
                    return (conn.resp, conn)
                else:
                    return (conn.getresponse(), conn)
        except (Exception, Timeout):
            self.logger.exception(_(\
            "connection: %(conn)s get_put_response: %(status)s" ), \
            {'conn': conn.target_service, 'status': _(\
            'Trying to get final status ')})

    @public
    @timing_stats()
    def STOP_SERVICE(self, req):
        """
         Handle HTTP STOP_SERVICE request.
        :param http_request
        return HTTPOk : when all operation completed
        """
        try:
            self.logger.info("STOP_SERVICE request received")
            # set block flag for all components
            ongoing_operation_list = list()
            for component in self._acc_ownership_map:
                self._comp_flag_req_count_map[component][0] = True
                if self._comp_flag_req_count_map[component][1] > 0:
                    ongoing_operation_list.append(component)

            # wait for ongoing request
            while len(ongoing_operation_list) > 0:
                sleep(2)
                for comp in ongoing_operation_list:
                    if self._comp_flag_req_count_map[comp][1] == 0:
                        ongoing_operation_list.remove(comp)
            self.logger.info("Ongoing requests completed")

            # temp cleanup
            base_path = os.path.join(self.root, self._base_fs, 'tmp')
            for component in self._acc_ownership_map:
                # dont care for failure (cleaned up in start up)
                temp_clean_up_in_comp_transfer(base_path, self._server, \
                component, self.logger)
            self.logger.info("temp cleanup done")
            return HTTPOk(request = req, headers = {'Message-Type' : \
            typeEnums.BLOCK_NEW_REQUESTS_ACK} )
        except Exception as err:
            self.logger.exception(err)
            self.logger.debug('Exception raised in STOP_SERVICE')
            return HTTPInternalServerError(request = req, headers = \
            {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK} )

    def __check_filesystem_mount(self):
        """
        Check mount of all OSD filesystems
        before recovery
        """
        filesystem_list = get_filesystem(self.logger)
        if self.mount_check:
            for file_system in filesystem_list:
                if not check_mount(self.root, file_system):
                    self.logger.info(('Processing mount check '
                        'for filesystem %(file)s'), {'file' : file_system})
                    return False
        return True


    def __account_recovery(self):
        """Clean old temp account files on service start-up """
        status = True
        create_recovery_file('account-server')
        self.logger.info('Account Server startup recovery start.')
        ret = self.__check_filesystem_mount()
        if not ret:
            self.logger.error('Mount check before recovery failed!')
            return ret
        base_path = os.path.join(self.root, self._base_fs, 'tmp')
        status = self.__start_recovery(base_path)
        self.logger.info('Account Server startup recovery Done.')
        return status

    def __start_recovery(self, recovery_base_path=None):
        status = True
        for comp in self._acc_ownership_map:
            ret = temp_clean_up_in_comp_transfer(recovery_base_path, \
            self._server, comp, self.logger)
            if not ret:
                status = False
        return status


    def __call__(self, env, start_response):
        start_time = time.time()
        req = Request(env)
        self.logger.txn_id = req.headers.get('x-trans-id', None)
        req_gl_map_version = float(req.headers.get('X-GLOBAL-MAP-VERSION'))
        req_component_id = req.headers.get('X-COMPONENT-NUMBER', None)
        method = getattr(self, req.method)
        self.logger.info("got request : %s" % req.method)
        self.logger.debug("Gl map version in account: %s, in req: %s" \
        % (self._global_map_version, req_gl_map_version))
        map_diff = float_comp(self._global_map_version, req_gl_map_version)
        if not check_utf8(req.path_info):
            self.logger.error("failed check_utf8")
            res = HTTPPreconditionFailed(body='Invalid UTF8 or contains NULL')
        elif not req_gl_map_version:
            self.logger.error("req check failed %s " % req_gl_map_version)
            res = HTTPPreconditionFailed(\
            body="x-global-map-version not defined in req header")
        elif  req.method.upper() != 'STOP_SERVICE' and map_diff > 0:
            self.logger.error('Map version in req: %s less than account: %s' \
            % (req_gl_map_version, self._global_map_version))
            res = HTTPMovedPermanently(body='Old map version in the request')
        else:
            try:
                # disallow methods which are not publicly accessible
                try:
                    getattr(method, 'publicly_accessible')
                    self.logger.debug("method accquired : %s" %method)
                except AttributeError, ex:
                    self.logger.error("attribute error: %s" % ex)
                    res = HTTPMethodNotAllowed()
                else:
                    # version check & load gl map conditionaly for other than STOP SERVICE
                    while req.method.upper() != 'STOP_SERVICE' and \
                    map_diff != 0:
                        if map_diff < 0:
                            self.logger.error('Map version in req header is greater than service gl version \
                            (request gl version :  %s, service gl version %s)' \
                            % (req_gl_map_version, self._global_map_version))
                            #load account map information i.e map and map version
                            if self.__get_major_version(req_gl_map_version) > self.__get_major_version(self._global_map_version):
                                self.__clear_blocked_list()
                            self.__load_gl_map_info()
                            self.logger.info('After reloading gl Map, gl map version at service %s' % self._global_map_version)
                            self._acc_ownership_map = \
                            self.__get_my_ownership_from_gmap()
                            self.__update_blocking_flag_map()
                            break
                        if map_diff > 0:
                            self.logger.error('Map version in req header is lesser than service new gl version, \
                                  (request gl version :  %s, service gl version %s)' % (req_gl_map_version, self._global_map_version))
                            break
                        map_diff = float_comp(self._global_map_version, \
                        req_gl_map_version)

                    if req.method.upper() in ['TRANSFER_COMPONENTS', \
                        'ACCEPT_COMPONENTS', 'STOP_SERVICE']:
                        self.logger.info("http request method : %s " \
                        % req.method)
                        res = method(req)
                    else:

                        if not req_component_id:
                            self.logger.error("failed on component_id")
                            res = HTTPPreconditionFailed(\
                            body="x-component_id not defined in req header")
                        else:
                            req_component_id = float(req_component_id)
                            if req_component_id not in \
                                self._comp_flag_req_count_map:
                                self._comp_flag_req_count_map[\
                                req_component_id] = [False, 0]

                            if self._comp_flag_req_count_map[\
                                req_component_id][0]:
                                # checked block status flag
                                res = HTTPTemporaryRedirect(body='Component transfer is going on for the requested component')
                            else:
                                self.logger.debug("checking the block status")
                                self._comp_flag_req_count_map[\
                                req_component_id][1] += 1
                                res = method(req)
                                #TODO: check suitable place, decrement count of ongoing request for particular component
                                if self._comp_flag_req_count_map.has_key(req_component_id):
                                    self._comp_flag_req_count_map[\
                                    req_component_id][1] -= 1

            except HTTPException, error_response:
                res = error_response
            except (Exception, Timeout):
                self.logger.exception(('ERROR __call__ error with %(method)s'
                                        ' %(path)s '),
                                    {'method': req.method, 'path': req.path})
                res = HTTPInternalServerError(body=traceback.format_exc())
        trans_time = '%.4f' % (time.time() - start_time)
        additional_info = ''
        if res.headers.get('x-container-timestamp') is not None:
            additional_info += 'x-container-timestamp: %s' % \
                res.headers['x-container-timestamp']
        if self.log_requests:
            log_msg = '%s - - [%s] "%s %s" %s %s "%s" "%s" "%s" %s "%s"' % (
                req.remote_addr,
                time.strftime('%d/%b/%Y:%H:%M:%S +0000', time.gmtime()),
                req.method, req.path,
                res.status.split()[0], res.content_length or '-',
                req.headers.get('x-trans-id', '-'),
                req.referer or '-', req.user_agent or '-',
                trans_time,
                additional_info)
            if req.method.upper() == 'REPLICATE':
                self.logger.debug(log_msg)
            else:
                self.logger.info(log_msg)
        # TODO: rasingh check below code
        #if self._comp_flag_req_count_map[req_component_id][1] == 0:
        #    del self._comp_flag_req_count_map[req_component_id]

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
    """paste.deploy app factory for creating WSGI account server apps"""
    conf = global_conf.copy()
    conf.update(local_conf)
    return AccountController(conf)


