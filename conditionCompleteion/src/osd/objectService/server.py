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

""" Object Server for OSD """

import os, sys
import multiprocessing
import time
import traceback
import socket
import subprocess
import math
from datetime import datetime
from osd import gettext_ as _
from hashlib import md5

from eventlet import sleep, Timeout

from osd.common.utils import public, get_logger, \
    config_true_value, timing_stats, loggit, float_comp 
from osd.common.bufferedhttp import http_connect
from osd.common.constraints import check_object_creation, \
    check_float, check_utf8, check_mount
from osd.common.exceptions import ConnectionTimeout, DiskFileQuarantined, \
    DiskFileNotExist, DiskFileCollision, DiskFileNoSpace, DiskFileDeleted, \
    DiskFileDeviceUnavailable, DiskFileExpired, ChunkReadTimeout
from osd.common.http import is_success, HTTP_SERVICE_UNAVAILABLE, HTTP_CONFLICT, \
    HTTP_TEMPORARY_REDIRECT, HTTP_MOVED_PERMANENTLY
from osd.common.request_helpers import split_and_validate_path, is_user_meta
from osd.common.swob import HTTPAccepted, HTTPBadRequest, HTTPCreated, \
    HTTPInternalServerError, HTTPNoContent, HTTPNotFound, HTTPNotModified, \
    HTTPPreconditionFailed, HTTPRequestTimeout, HTTPUnprocessableEntity, \
    HTTPClientDisconnect, HTTPMethodNotAllowed, Request, Response, UTC, \
    HTTPInsufficientStorage, HTTPForbidden, HTTPException, HeaderKeyDict, \
    HTTPConflict, HTTPServerError, HTTPOk, HTTPServiceUnavailable, \
    HTTPMovedPermanently, HTTPTemporaryRedirect
from osd.objectService.diskfile import DATAFILE_SYSTEM_META, DiskFileManager, \
    EXPORT_PATH, DATADIR, METADIR
from osd.objectService.objectutils import LockManager, TranscationManager
from osd.objectService.recovery_handler import RecoveryHandler
from osd.common.utils import get_global_map, get_host_from_map
from osd.common.ring.ring import get_ip_port_by_id, get_service_id, \
    hash_path, get_fs_list, ObjectRing
from osd.containerService.utils import SingletonType
from osd.glCommunicator.message_type_enum_pb2 import *
import libraryUtils

WAIT_TIME_FOR_TRANSID_CHECK = 10

class RecoveryStatus(object):
    """Recovery status."""
    def __init__(self):
        self.status = False
    def get_status(self):
        return self.status
    def set_status(self, status):
        self.status = status



class ObjectController(object):
    """Implements the WSGI application for the OSD Object Server."""
    __metaclass__ = SingletonType

    def __init__(self, conf, logger=None):
        """
        Creates a new WSGI application for the OSD Object Server. An
        example configuration is given at
        /opt/HYDRAstor/objectStorage/config/object-server.conf
        """
        #Logger initialized
        self.logger = logger or get_logger(conf, log_route='object-server')
        self.logger.info("Starting Object Service")

        #Parameters received during startup
        self.retry_count = int(conf.get('retry_count', 3))
        self.__port = int(conf.get('bind_port', 61008))
        self.wait_map_load = int(conf.get('wait_map_load', 60))
        self.__ll_port = int(conf.get('llport', 61014))
        self.ll_host = socket.gethostname()
        self.__serv_id = self.ll_host + "_" + str(self.__ll_port) + "_object-server"
        self.logger.info("Local leader parameters-> port: %s, llport: %s, llhost: %s" \
                         % (self.__port, self.__ll_port, self.ll_host))

        #Loading Config Parameters
        self.node_timeout = int(conf.get('node_timeout', 3))
        self.osd_dir = conf.get('osd_dir', '/export/.osd_meta_config') 
        self.conn_timeout = float(conf.get('conn_timeout', 0.5))
        self.client_timeout = int(conf.get('client_timeout', 60))
        self.disk_chunk_size = int(conf.get('disk_chunk_size', 65536))
        self.network_chunk_size = int(conf.get('network_chunk_size', 65536))
        self.log_requests = config_true_value(conf.get('log_requests', 'true'))
        self.log_failure = config_true_value(conf.get('log_failure', 'true'))
        self.max_upload_time = int(conf.get('max_upload_time', 86400))
        self.slow = int(conf.get('slow', 0))
        self.conflict_timeout = int(conf.get('conflict_timeout', 30))
        self.keep_cache_private = \
            config_true_value(conf.get('keep_cache_private', 'false'))

        default_allowed_headers = '''
            content-disposition,
            content-encoding,
	    x-static-large-object,
        '''
        extra_allowed_headers = [
            header.strip().lower() for header in conf.get(
                'allowed_headers', default_allowed_headers).split(',')
            if header.strip()
        ]
        self.allowed_headers = set()
        for header in extra_allowed_headers:
            if header not in DATAFILE_SYSTEM_META:
                self.allowed_headers.add(header)
        socket._fileobject.default_bufsize = self.network_chunk_size



	#Creating Performance Stats parameter
        self.pystat = libraryUtils.PyStatManager(libraryUtils.Module.ObjectService)


        #self.statInstance = libraryUtils.StatsInterface()
        self.statInstance = self.pystat.createAndRegisterStat("object_stat", libraryUtils.Module.ObjectService)

        self.put_object_size = 0
        self.put_time = 0
        self.get_object_counter = 0

        # Provide further setup specific to an object server implemenation.
        self.setup(conf)
        

        #Config Parameters
        self.logger.info("Object Startup completed with config \
                          parameters: %s " % conf)

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
            self.logger.error("Error occurred while getting ip of node %s "% err)
            return ""

    def setup(self, conf):
        """
        Implementation specific setup. This method is called at the very end
        by the constructor to allow a specific implementation to modify
        existing attributes or add its own attributes.

        :param conf: WSGI configuration parameter
        """
        #Common on-disk hierarchy shared across account, container and object
        #servers.
        self._diskfile_mgr = DiskFileManager(conf, self.logger)

        ##keeps lock information in memory
        self.__lock_obj = LockManager(self.logger)

        #node_delete flag=True when it is going to be deleted
        self.node_delete = False

        #Communication module with gl
        try:
            with Timeout(self.node_timeout):
                self.logger.debug("Loading Global Map")
                self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger)
                self.logger.debug("Map loaded with parameters: %s %s %s " % \
                    (self.__initial_map_load, self.__map_version_object, self.conn_obj))
        except Exception as err:
            self.logger.debug("Exception raised in Map load: %s" % err)
            sys.exit(130)
        except Timeout as err:
            self.logger.debug("Timeout during Map load: %s" % err)
            sys.exit(130)

        #Global Variable for map_load
        self.__map_loaded_failure_case = False

        ##communicates with container server to acquire the lock
        self.__http_comm = TranscationManager(self.logger)
        self.__obj_port = int(conf.get('bind_port', '61008'))
        self.__root = conf.get('filesystems', '/export')
        self.__mount_check = config_true_value(conf.get('mount_check', 'true'))

        try:
            self.__recovery_handler_obj = \
                RecoveryHandler(self.__serv_id, self.logger)
            if 'recovery_status' in conf:
                recovery_status = conf['recovery_status'][0].get_status()
                if not recovery_status:
                    status = self.__check_filesystem_mount()
                    if not status:
                        self.logger.error('Mount check failed before recovery')
                        print "Object Server Recovery failed"
                        sys.exit(130)
                    status = self.__recovery_handler_obj.startup_recovery(self.__get_node_ip(self.ll_host), self.__port, self.__ll_port, self.__serv_id)
                    if not status:
                        self.logger.error('Object server recovery failed')
                        print "Object Server Recovery failed"
                        sys.exit(130)
                    conf['recovery_status'][0].set_status(True) 
        except Exception as err:
            self.logger.error('Object server recovery failed, error: %s' % err)
            print "Object Server Recovery Failed"
            sys.exit(130)

    def __check_filesystem_mount(self):
        """
        Check mount of all OSD filesystems
        before recovery
        """
        filesystem_list = get_fs_list(self.logger)
        if self.__mount_check:
            for file_system in filesystem_list:
                if not check_mount(self.__root, file_system):
                    self.logger.error(_('mount check failed '
                        'for filesystem %(file)s'), {'file' : file_system})
                    return False
        return True


    def get_diskfile(self, filesystem, acc_dir, cont_dir, obj_dir, account, container, obj,
                     **kwargs):
        """
        Utility method for instantiating a DiskFile object supporting a given
        REST API.

        An implementation of the object server that wants to use a different
        DiskFile class would simply over-ride this method to provide that
        behavior.
        """
        return self._diskfile_mgr.get_diskfile(
            filesystem, acc_dir, cont_dir, obj_dir, account, container, obj, **kwargs)

    def async_update(self, op, account, container, obj, host, cont_dir,
                     contfs, headers_out):
        """
        Sends or saves an async update.

        :param op: operation performed (ex: 'PUT', or 'DELETE')
        :param account: account name for the object
        :param container: container name for the object
        :param obj: object name
        :param host: host that the container is on
        :param cont_dir: directory that the container is on
        :param contfs: filesystem name that the container is on
        :param headers_out: dictionary of headers to send in the container
                            request
        :param objfs: filesystem name that the object is in
        """
        headers_out['user-agent'] = 'obj-server %s' % os.getpid()
        full_path = '/%s/%s/%s' % (account, container, obj)
        try:
            with ConnectionTimeout(self.conn_timeout):
                ip, port = host.rsplit(':', 1)
                conn = http_connect(ip, port, contfs, cont_dir, op,
                                    full_path, headers_out)
            with Timeout(self.node_timeout):
                response = conn.getresponse()
                response.read()
                if is_success(response.status):
                    return True
                elif response.status in [301, 307]:
                    #Moved Permanently
                    return 'redirect' 
                else:
                    self.logger.error(_(
                        'ERROR Container update failed '
                        '(saving for async update later): %(status)d '
                        'response from %(ip)s:%(port)s/%(dev)s'),
                        {'status': response.status, 'ip': ip, 'port': port,
                        'dev': contfs})
                    return False
        except (Exception, Timeout):
            self.logger.exception(_(
                'ERROR container update failed with '
                '%(ip)s:%(port)s/%(dev)s (saving for async update later)'),
                {'ip': ip, 'port': port, 'dev': contfs})
            return False

    def container_update(self, op, account, container, obj, request,
                         headers_out):
        """
        Update the container when objects are updated.

        :param op: operation performed (ex: 'PUT', or 'DELETE')
        :param account: account name for the object
        :param container: container name for the object
        :param obj: object name
        :param request: the original request object driving the update
        :param headers_out: dictionary of headers to send in the container
                            request(s)
        :param objfs: filesystem name that the object is in
        """
        try:
            headers_in = request.headers
            conthost = headers_in.get('X-Container-Host', '')
            contfilesystem = headers_in.get('X-Container-Filesystem', '')
            cont_directory = headers_in.get('X-Container-Directory', '')
            headers_out['x-trans-id'] = headers_in.get('x-trans-id', '-')
            headers_out['referer'] = request.as_referer()
            ret = self.async_update(op, account, container, obj, conthost,
                cont_directory, contfilesystem, headers_out)
            return ret
        except Exception as err:
            self.logger.exception(err)
            return False

    def remove_trans_id(self, object_path, trans_id):
        """
        Remove transaction ID from memory in case of any failure.
        :param trans_id: transaction ID
        """
        try:
            self.__lock_obj.delete(object_path, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory delete fail with transcation ID:'
            '%(transcation_ID)s)'),
            {'transcation_ID': trans_id})

    def release_lock(self, host, filesystem, cont_dir, headers):
        """
        Releases lock in case of any failure.
        :param host: IP, port of container service
        :param filesystem: filesystem
        :param cont_dir: container directory
        :param headers: headers
        """
        try:
            self.__http_comm.release_lock(host, filesystem, \
                cont_dir, headers)
        except(Exception, Timeout):
            self.logger.exception(_(
            'ERROR method FREE_LOCK failed with '
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : cont_dir})


    @public
    @timing_stats()
    @loggit('object-server')
    def POST(self, request):
        """Handle HTTP POST requests for the OSD Object Server."""
        filesystem, acc_dir, cont_dir, obj_dir, account, container, obj = \
            split_and_validate_path(request, 7, 7, True)

        if 'x-timestamp' not in request.headers or \
                not check_float(request.headers['x-timestamp']):
            return HTTPBadRequest(body='Missing timestamp', request=request,
                                  content_type='text/plain')
        try:
            disk_file = self.get_diskfile(
                filesystem, acc_dir, cont_dir, obj_dir, \
                account, container, obj)
        except DiskFileDeviceUnavailable:
            return HTTPInsufficientStorage(drive=filesystem, request=request)

        full_path = '/%s/%s/%s' % (account, container, obj)
        obj_hash = "%s/%s/%s" % (hash_path(account), \
                                 hash_path(account, container), \
                                 hash_path(account, container, obj))
        host = request.headers.get('X-Container-Host')
        map_version = request.headers.get('X-GLOBAL-MAP-VERSION')
        component_no = request.headers.get('X-COMPONENT-NUMBER')
        self.logger.info("host: %s, map_version: %s"% (host, map_version))

        data = HeaderKeyDict({'x-server-id' : self.__serv_id, \
            'x-object-path' : obj_hash, 'x-object_port' : self.__obj_port, \
            'x-mode' : 'w', 'x-operation' : 'POST', \
            'x-global-map-version': map_version, \
            'x-component-number': component_no})
        trans_id = ''
        try:
            resp = self.__http_comm.acquire_lock(host, filesystem, \
                cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
            resp.read()
            if not is_success(resp.status):
                self.logger.error((
                    'ERROR method %(method)s failed '
                    'response status: %(status)d '),
                    {'method': 'GET_LOCK', 'status': resp.status})
                if resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT, HTTP_CONFLICT]:
                    tries = 0
                    self.logger.debug("resp.status: %s" % resp.status)
                    while tries < self.retry_count:
                        tries += 1
                        if resp.status == HTTP_TEMPORARY_REDIRECT:
                            #eventlet sleep
                            sleep(self.wait_map_load)
                        if self.__map_loaded_failure_case == False:

                            try:
                                self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger, request)
                            except ValueError as err:
                                self.logger.error("Global map could not be loaded, exiting now")
                                return HTTPInternalServerError(request=request)

                            self.logger.debug("map: %s, version: %s, conn_obj: %s"% \
                                (self.__initial_map_load, self.__map_version_object, self.conn_obj))
                            self.__map_loaded_failure_case = True

                        host = get_host_from_map(account, container, self.__initial_map_load, \
                                    self.osd_dir, self.logger)
                        if not host:
                            return HTTPServiceUnavailable(request=request)

                        map_version = self.__map_version_object
                        self.logger.info("host: %s, map_version: %s"% (host, map_version))
                        data.update({'x-global-map-version': map_version})
                        resp = self.__http_comm.acquire_lock(host, filesystem, cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
                        resp.read()
                        if not is_success(resp.status):
                            self.__map_loaded_failure_case = False
                            if resp.status == HTTP_CONFLICT:
                                sleep(self.conflict_timeout)
                                continue
                                #return HTTPConflict(request=request)

                            elif resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT]:
                                continue

                            else:
                                return HTTPServiceUnavailable(request=request)
                        else:
                            break

                else:
                    return HTTPServiceUnavailable(request=request)

                if resp.status == HTTP_CONFLICT:
                    return HTTPConflict(request=request)  #being on the safer side

            trans_id = self.__http_comm.get_transaction_id_from_resp(resp)
            if not trans_id:
                self.logger.error(_(
                'ERROR method acquire_lock failed with'
                '%(host)s/%(fs)s/%(dir)s'),
                {'host': host, 'fs': filesystem, 'dir' : account})
                return HTTPServiceUnavailable(request=request)
        except(Exception, Timeout) as err:
            self.logger.exception(_(
            'ERROR method GET_LOCK failed with'
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
            if isinstance(err.__class__, type(socket.error)) and ('ECONNREFUSED' in str(err)):
                status, respond = self.service_unavailable_retry(request, filesystem, \
                    cont_dir, account, container, data)
                if status:
                    return respond(request=request)
                else:
                    trans_id = respond # it is transaction id in this case
            else:
                return HTTPServiceUnavailable(request=request)

        try:
            self.__lock_obj.add(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory add fail for path:'
            '%(full_path)s)'),
            {'full_path': full_path})
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.release_lock(host, filesystem, cont_dir, headers)
            return HTTPServerError(request=request)

        try:
            orig_metadata = disk_file.read_metadata()
        except DiskFileNotExist:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            return HTTPNotFound(request=request)
        except Exception:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            raise
        orig_timestamp = orig_metadata.get('X-Timestamp', '0')
        if orig_timestamp >= request.headers['x-timestamp']:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            return HTTPConflict(request=request)
        ##get the Sys metadata and put it in new metadata
        metadata = {'X-Timestamp': request.headers['x-timestamp']}
        sys_metadata = dict(
                [(key, val) for key, val in orig_metadata.iteritems()
                 if key.lower() in DATAFILE_SYSTEM_META])
        metadata.update(sys_metadata)
        
        metadata.update(val for val in request.headers.iteritems()
                        if is_user_meta('object', val[0]))
        for header_key in self.allowed_headers:
            if header_key in request.headers:
                header_caps = header_key.title()
                metadata[header_caps] = request.headers[header_key]
        try:
            disk_file.write_metadata(metadata)
        except Exception:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            raise
        try:
            data = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.__http_comm.release_lock(host, filesystem, \
                cont_dir, data)
        except(Exception, Timeout):
            self.logger.exception(_(
            'ERROR method FREE_LOCK failed with '
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
        try:
            self.__lock_obj.delete(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory delete fail with transcation ID:'
            '%(transcation_ID)s)'),
            {'transcation_ID': trans_id})
        return HTTPAccepted(request=request)

    @public
    @timing_stats()
    @loggit('object-server')
    def PUT(self, request):
        """Handle HTTP PUT requests for the OSD Object Server."""
        filesystem, acc_dir, cont_dir, obj_dir, account, container, obj = \
            split_and_validate_path(request, 7, 7, True)
        self.logger.debug("Directories :%s %s %s %s %s %s %s"% \
            (filesystem, acc_dir, cont_dir, obj_dir, account, container, obj))

        if 'x-timestamp' not in request.headers or \
                not check_float(request.headers['x-timestamp']):
            return HTTPBadRequest(body='Missing timestamp', request=request,
                                  content_type='text/plain')
        error_response = check_object_creation(request, obj)
        if error_response:
            return error_response
        try:
            fsize = request.message_length()
        except ValueError as e:
            return HTTPBadRequest(body=str(e), request=request,
                                  content_type='text/plain')
        try:
            disk_file = self.get_diskfile(
                filesystem, acc_dir, cont_dir, obj_dir, account, container, obj)
        except DiskFileDeviceUnavailable:
            return HTTPInsufficientStorage(drive=filesystem, request=request)

        full_path = '/%s/%s/%s' % (account, container, obj)
        obj_hash = "%s/%s/%s" % (hash_path(account), \
                                 hash_path(account, container), \
                                 hash_path(account, container, obj))
        #Headers
        map_version = request.headers.get('X-GLOBAL-MAP-VERSION')
        component_no = request.headers.get('X-COMPONENT-NUMBER')
        host = request.headers.get('X-Container-Host')

        #to keep object-map updated
        if float_comp(self.__map_version_object, float(map_version)) == 0:
            self.__map_loaded_failure_case = False

        self.logger.info("component:%s, host: %s, map_version: %s" \
                % (component_no, host, map_version))
        data = HeaderKeyDict({'x-server-id' : self.__serv_id, \
            'x-object-path' : obj_hash, 'x-object_port' : self.__obj_port, \
            'x-mode' : 'w', 'x-operation' : 'PUT', \
            'x-global-map-version': map_version, \
            'x-component-number': component_no})
        trans_id = ''
        try:
            resp = self.__http_comm.acquire_lock(host, filesystem, \
                cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
            resp.read()
            if not is_success(resp.status):
                self.logger.error((
                    'ERROR method %(method)s failed '
                    'response status: %(status)d '),
                    {'method': 'GET_LOCK', 'status': resp.status})
                if resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT, HTTP_CONFLICT]:
                    tries = 0
                    self.logger.debug("resp.status: %s" % resp.status)
                    while tries < self.retry_count: #sachin: need to add config parameter
                        tries += 1
                        if resp.status == HTTP_TEMPORARY_REDIRECT:
                            #eventlet sleep
                            sleep(self.wait_map_load)
                        if self.__map_loaded_failure_case == False:
                            try:
                                self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger, request, 30)
                            except ValueError as err:
                                self.logger.error("Global map could not be loaded, exiting now")
                                return HTTPInternalServerError(request=request)

                            self.logger.debug("map: %s, version: %s, conn_obj: %s"% \
                                (self.__initial_map_load, self.__map_version_object, self.conn_obj))
                            self.__map_loaded_failure_case = True

                        host = get_host_from_map(account, container, self.__initial_map_load, \
                                    self.osd_dir, self.logger)
                        if not host:
                            return HTTPServiceUnavailable(request=request)

                        request.headers['X-Container-Host'] = host
                        map_version = self.__map_version_object
                        self.logger.info("host: %s, map_version: %s"% (host, map_version))
                        data.update({'x-global-map-version': map_version}) 
                        resp = self.__http_comm.acquire_lock(host, filesystem, cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
                        resp.read()
                        if not is_success(resp.status):
                            self.__map_loaded_failure_case = False
                            if resp.status == HTTP_CONFLICT:
                                sleep(self.conflict_timeout) #sleep for (configurable val)1 second and try again till config value
                                continue #sachin: i want to continue in case of conflict //return HTTPConflict(request=request)

                            elif resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT]:
                                continue
                               
                            else:
                                return HTTPServiceUnavailable(request=request)
                        else:
                            break

                else:
                    return HTTPServiceUnavailable(request=request)

                if resp.status == HTTP_CONFLICT:
                    return HTTPConflict(request=request)
    
            trans_id = self.__http_comm.get_transaction_id_from_resp(resp)
            if not trans_id:
                self.logger.error(_(
                'ERROR method acquire_lock failed with'
                '%(host)s/%(fs)s/%(dir)s'),
                {'host': host, 'fs': filesystem, 'dir' : account})
                return HTTPServiceUnavailable(request=request)
        except(Exception, Timeout) as err:
            self.logger.exception(_(
            'ERROR method GET_LOCK failed with'
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
            if isinstance(err.__class__, type(socket.error)) and ('ECONNREFUSED' in str(err)):
                status, respond = self.service_unavailable_retry(request, filesystem, \
                    cont_dir, account, container, data)
                if status:
                    return respond(request=request)
                else:
                    trans_id = respond # it is transaction id in this case
            else:
                return HTTPServiceUnavailable(request=request)

        try:
            self.__lock_obj.add(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory add fail for path:'
            '%(full_path)s)'),
            {'full_path': full_path})
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.release_lock(host, filesystem, cont_dir, headers)
            return HTTPServerError(request=request)

        try:
            orig_metadata = disk_file.read_metadata()
        except (DiskFileNotExist, DiskFileQuarantined):
            orig_metadata = {}

        orig_timestamp = orig_metadata.get('X-Timestamp')
        if orig_timestamp and orig_timestamp >= request.headers['x-timestamp']:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.release_lock(host, filesystem, cont_dir, headers)
            self.remove_trans_id(obj_hash, trans_id)
            return HTTPConflict(request=request)

        #upload_expiration = time.time() + self.max_upload_time
        elapsed_time = 0
        try:
            with disk_file.create() as writer:
                upload_size = 0

                def timeout_reader():
                    with ChunkReadTimeout(self.client_timeout):
                        return request.environ['wsgi.input'].read(
                            self.network_chunk_size)

                writer.init_md5_hash()
                try:
                    for chunk in iter(lambda: timeout_reader(), ''):
                        start_time = time.time()
                        #if start_time > upload_expiration:
                        #    self.logger.increment('PUT.timeouts')
                        #    return HTTPRequestTimeout(request=request)
                        upload_size = writer.write(chunk)
                        elapsed_time += time.time() - start_time

                    #Performance Stats for PUT request
                    if elapsed_time > 0:
                        size_in_mb = upload_size/(1024*1024)
                        bandwidth = size_in_mb/elapsed_time
                        self.put_object_size += size_in_mb
                        self.put_time += elapsed_time
                        self.statInstance.avgBandForUploadObject = self.put_object_size/self.put_time
                        if self.statInstance.maxBandForUploadObject < bandwidth:
                            self.statInstance.maxBandForUploadObject = bandwidth
                        if self.statInstance.minBandForUploadObject == 0:
                            self.statInstance.minBandForUploadObject = bandwidth
                        else:
                            if self.statInstance.minBandForUploadObject > bandwidth:
                                self.statInstance.minBandForUploadObject = bandwidth
                    else:
                        self.logger.info("Elapsed time %s is less than zero" %elapsed_time)

                except ChunkReadTimeout:
                    headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                        'x-request-trans-id' : trans_id,
                        'x-global-map-version': map_version,
                        'x-component-number': component_no})
                    self.remove_trans_id(obj_hash, trans_id)
                    self.release_lock(host, filesystem, cont_dir, headers)
                    #self.remove_trans_id(trans_id)
                    return HTTPRequestTimeout(request=request)
                if upload_size:
                    self.logger.transfer_rate(
                        'PUT.' + filesystem + '.timing', elapsed_time,
                        upload_size)
                if fsize is not None and fsize != upload_size:
                    headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                        'x-request-trans-id' : trans_id, \
                        'x-global-map-version': map_version, \
                        'x-component-number': component_no})
                    self.remove_trans_id(obj_hash, trans_id)
                    self.release_lock(host, filesystem, cont_dir, headers)
                    #self.remove_trans_id(trans_id)
                    return HTTPClientDisconnect(request=request)
                etag = writer.get_md5_hash()
                if 'etag' in request.headers and \
                        request.headers['etag'].lower() != etag:
                    headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                        'x-request-trans-id' : trans_id, \
                        'x-global-map-version': map_version, \
                        'x-component-number': component_no})
                    self.remove_trans_id(obj_hash, trans_id)
                    self.release_lock(host, filesystem, cont_dir, headers)
                    #self.remove_trans_id(trans_id)
                    return HTTPUnprocessableEntity(request=request)
                metadata = {
                    'X-Timestamp': request.headers['x-timestamp'],
                    'Content-Type': request.headers['content-type'],
                    'ETag': etag,
                    'Content-Length': str(upload_size),
                }
                metadata.update(val for val in request.headers.iteritems()
                                if is_user_meta('object', val[0]))
                for header_key in (self.allowed_headers):
                    if header_key in request.headers:
                        header_caps = header_key.title()
                        metadata[header_caps] = request.headers[header_key]
                writer.put(metadata)
        except DiskFileNoSpace:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id,  \
                'x-global-map-version': map_version,  \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            return HTTPInsufficientStorage(drive=filesystem, request=request)
        except Exception:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id,  \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            raise

        headers_out = HeaderKeyDict({
                'x-size': metadata['Content-Length'],
                'x-content-type': metadata['Content-Type'],
                'x-timestamp': metadata['X-Timestamp'],
                'x-etag': metadata['ETag'],
                'x-global-map-version': map_version, \
                'x-component-number': component_no})

        if orig_metadata:
            headers_out.update({'x-duplicate-update': True})
            headers_out.update({'x-old-size': orig_metadata['Content-Length']})
        self.logger.debug(_('Sending request for container '
            'update for PUT object: %s, headers: %s' % \
            (request.path, headers_out)))
        retry = 0
        while retry <= self.retry_count:
            ret = self.container_update(
                'PUT', account, container, obj, request,
                headers_out)
            if ret == 'redirect' and retry == self.retry_count:
                #In case every try fail then give user created..
                ret = False
            if ret == False:
                try:
                    self.logger.error(_('ERROR container update failed '
                    'for PUT object: %s' % request.path))
                    self.__lock_obj.delete(obj_hash, trans_id)
                    return HTTPCreated(request=request, etag=etag)
                except Exception:
                    self.logger.exception(_(
                        'ERROR object server memory delete fail'
                        'for transaction ID:'
                        '%(transcation_ID)s)'),
                        {'transcation_ID': trans_id})
                    break
            elif ret == 'redirect':
                retry += 1
                try:
                    self.__initial_map_load, self.__map_version_object, self.conn_obj = \
                        get_global_map(self.__serv_id, self.logger, request, time_=self.wait_map_load)
                except ValueError as err:
                    self.logger.error("Global map could not be loaded, exiting now")
                    self.__lock_obj.delete(obj_hash, trans_id)
                    return HTTPCreated(request=request, etag=etag)
                    #return HTTPInternalServerError(request=request)
                map_version = self.__map_version_object
                headers_out.update({'x-global-map-version': map_version})
                try:
                    host = get_host_from_map(account, container, self.__initial_map_load, \
                                self.osd_dir, self.logger)
                    if not host:
                        self.__lock_obj.delete(obj_hash, trans_id)
                        return HTTPCreated(request=request, etag=etag)
                except Exception as err:
                    self.__lock_obj.delete(obj_hash, trans_id)
                    return HTTPCreated(request=request, etag=etag)
                    #return HTTPServiceUnavailable(request=request)
                self.logger.info('Calculated host to redirect: %s %s ' % (host, map_version))
                request.headers['X-Container-Host'] = host
                continue
                #break when success or continue to retry
            else:
                break

        self.logger.debug(_('Container update successful for '
            'PUT object: %s' % request.path))

        try:
            data = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.__http_comm.release_lock(host, filesystem, \
                cont_dir, data)
        except(Exception, Timeout):
            self.logger.exception(_(
            'ERROR method FREE_LOCK failed with '
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
            host = request.headers.get('X-Container-Host')
        try:
            self.__lock_obj.delete(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory delete fail with transcation ID:'
            '%(transcation_ID)s)'),
            {'transcation_ID': trans_id})
        return HTTPCreated(request=request, etag=etag)

    @public
    @timing_stats()
    @loggit('object-server')
    def GET(self, request):
        """Handle HTTP GET requests for the OSD Object Server."""
        filesystem, acc_dir, cont_dir, obj_dir, account, container, obj = \
            split_and_validate_path(request, 7, 7, True)
        self.logger.debug("%s %s %s %s %s %s %s" \
            % (filesystem, acc_dir, cont_dir, obj_dir, account, container, obj))
        keep_cache = self.keep_cache_private or (
            'X-Auth-Token' not in request.headers and
            'X-Storage-Token' not in request.headers)
        try:
            disk_file = self.get_diskfile(
                filesystem, acc_dir, cont_dir, obj_dir, \
                account, container, obj)
        except DiskFileDeviceUnavailable:
            return HTTPInsufficientStorage(drive=filesystem, request=request)

        full_path = '/%s/%s/%s' % (account, container, obj)
        obj_hash = "%s/%s/%s" % (hash_path(account), \
                                 hash_path(account, container), \
                                 hash_path(account, container, obj))
        #map_from_proxy
        map_version = request.headers.get('X-GLOBAL-MAP-VERSION')
        component_no = request.headers.get('X-COMPONENT-NUMBER')
        #to keep object-map updated
        self.logger.info("map version received  %r" % request.headers)
        if float_comp(self.__map_version_object, float(map_version)) == 0:
            self.__map_loaded_failure_case = False



        host = request.headers.get('X-Container-Host')

        self.logger.info("host: %s, map_version: %s"% (host, map_version))
        data = HeaderKeyDict({'x-server-id' : self.__serv_id, \
            'x-object-path' : obj_hash, 'x-object_port' : self.__obj_port, \
            'x-mode' : 'r', 'x-operation' : 'GET', \
            'x-global-map-version': map_version, \
            'x-component-number': component_no})
        trans_id = ''
        try:
            resp = self.__http_comm.acquire_lock(host, filesystem, \
                cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
            resp.read()
            if not is_success(resp.status):
                self.logger.error((
                    'ERROR method %(method)s failed '
                    'response status: %(status)d '),
                    {'method': 'GET_LOCK', 'status': resp.status})
                if resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT, HTTP_CONFLICT]:
                    tries = 0
                    self.logger.debug("resp.status: %s" % resp.status)
                    while tries < self.retry_count:
                        tries += 1
                        if resp.status == HTTP_TEMPORARY_REDIRECT:
                            #eventlet sleep
                            sleep(self.wait_map_load)
                        if self.__map_loaded_failure_case == False:
                            try:
                                self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger, request)
                            except ValueError as err:
                                self.logger.error("Global map could not be loaded, exiting now")
                                return HTTPInternalServerError(request=request)

                            self.logger.debug("map: %s, version: %s, conn_obj: %s"% \
                                (self.__initial_map_load, self.__map_version_object, self.conn_obj))
                            self.__map_loaded_failure_case = True

                        host = get_host_from_map(account, container, self.__initial_map_load, \
                                    self.osd_dir, self.logger)
                        if not host:
                            return HTTPServiceUnavailable(request=request)

                        map_version = self.__map_version_object
                        self.logger.info("host: %s, map_version: %s"% (host, map_version))

                        data.update({'x-global-map-version': map_version})
                        resp = self.__http_comm.acquire_lock(host, filesystem, cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
                        resp.read()
                        if not is_success(resp.status):
                            self.__map_loaded_failure_case = False
                            if resp.status == HTTP_CONFLICT:
                                sleep(self.conflict_timeout)
                                continue
                                #return HTTPConflict(request=request)

                            elif resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT]:
                                continue
                            
                            else:
                                return HTTPServiceUnavailable(request=request)
                        else:
                            break

                else:
                    return HTTPServiceUnavailable(request=request)

                if resp.status == HTTP_CONFLICT:
                    return HTTPConflict(request=request)

            trans_id = self.__http_comm.get_transaction_id_from_resp(resp)
            if not trans_id:
                self.logger.error(_(
                'ERROR method acquire_lock failed with'
                '%(host)s/%(fs)s/%(dir)s'),
                {'host': host, 'fs': filesystem, 'dir' : account})
                return HTTPServiceUnavailable(request=request)
        except(Exception, Timeout) as err:
            self.logger.exception(_(
            'ERROR method GET_LOCK failed with'
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
            if isinstance(err.__class__, type(socket.error)) and ('ECONNREFUSED' in str(err)):
                status, respond = self.service_unavailable_retry(request, filesystem, \
                    cont_dir, account, container, data)
                if status:
                    return respond(request=request)
                else:
                    trans_id = respond # it is transaction id in this case
            else:
                return HTTPServiceUnavailable(request=request)

        try:
            self.__lock_obj.add(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory add fail for path:'
            '%(full_path)s)'),
            {'full_path': full_path})
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.release_lock(host, filesystem, cont_dir, headers)
            return HTTPServerError(request=request)

        try:
            #Performance Start time
            #start_time = time.time()
            with disk_file.open():
                metadata = disk_file.get_metadata()
                if metadata.get('Transfer-Encoding'):
                    del metadata['Transfer-Encoding']
                obj_size = int(metadata['Content-Length'])
                file_x_ts = metadata['X-Timestamp']
                file_x_ts_flt = float(file_x_ts)
                file_x_ts_utc = datetime.fromtimestamp(file_x_ts_flt, UTC)

                if_unmodified_since = request.if_unmodified_since
                if if_unmodified_since and file_x_ts_utc > if_unmodified_since:
                    headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                        'x-request-trans-id' : trans_id, \
                        'x-global-map-version': map_version, \
                        'x-component-number': component_no})
                    self.remove_trans_id(obj_hash, trans_id)
                    self.release_lock(host, filesystem, cont_dir, headers)
                    #self.remove_trans_id(trans_id)
                    return HTTPPreconditionFailed(request=request)

                if_modified_since = request.if_modified_since
                if if_modified_since and file_x_ts_utc <= if_modified_since:
                    headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                        'x-request-trans-id' : trans_id, \
                        'x-global-map-version': map_version, \
                        'x-component-number': component_no})
                    self.remove_trans_id(obj_hash, trans_id)
                    self.release_lock(host, filesystem, cont_dir, headers)
                    #self.remove_trans_id(trans_id)
                    return HTTPNotModified(request=request)

                keep_cache = (self.keep_cache_private or
                              ('X-Auth-Token' not in request.headers and
                               'X-Storage-Token' not in request.headers))
                response = Response(
                    app_iter=disk_file.reader(keep_cache=keep_cache, statInstance=self.statInstance),
                    request=request, conditional_response=True)
                response.headers['Content-Type'] = metadata.get(
                    'Content-Type', 'application/octet-stream')
                for key, value in metadata.iteritems():
                    if is_user_meta('object', key) or \
                            key.lower() in self.allowed_headers:
                        response.headers[key] = value
                response.etag = metadata['ETag']
                response.last_modified = math.ceil(file_x_ts_flt)
                response.content_length = obj_size
                try:
                    response.content_encoding = metadata[
                        'Content-Encoding']
                except KeyError:
                    pass
                response.headers['X-Timestamp'] = file_x_ts
                resp = request.get_response(response)

		#Performance Stat for GET request 
                #band = obj_size / ( time.time() - start_time )
                #total_time = self.statInstance.avgBandForDownloadObject * self.get_object_counter
                #self.get_object_counter += 1
                #self.statInstance.avgBandForDownloadObject = (total_time + band)/self.get_object_counter
                #if self.statInstance.minBandForDownloadObject > band:
                #    self.statInstance.minBandForDownloadObject = band

        except DiskFileNotExist:
            resp = HTTPNotFound(request=request, conditional_response=True)
        except Exception:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            raise
        try:
            data = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.__http_comm.release_lock(host, filesystem, \
                cont_dir, data)
        except(Exception, Timeout):
            self.logger.exception(_(
            'ERROR method FREE_LOCK failed with '
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
        try:
            self.__lock_obj.delete(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory delete fail with transcation ID:'
            '%(transcation_ID)s)'),
            {'transcation_ID': trans_id})
        return resp

    @public
    @timing_stats()
    @loggit('object-server')
    def BULK_DELETE(self, req):
        """Handle HTTP BULK_DELETE requests for the OSD Object Server."""
        container_service_id = req.headers.get('x-server-id')
        host = '%(ip)s:%(port)s' % get_ip_port_by_id(container_service_id, self.logger)
        req.headers['X-Container-Host'] = host
        obj_hash_list = eval(req.body)
        obj_ring = ObjectRing(self.osd_dir)
        obj_hash_list = [entry for entry in obj_hash_list if entry]
        for obj_data in obj_hash_list:
            acc_hash, cont_hash, obj_hash = obj_data.split('/')
            file_system, directory = obj_ring.get_directories_by_hash(\
            acc_hash, cont_hash, obj_hash)
            acc_dir, cont_dir, obj_dir = directory.split('/')
            data_file_path = os.path.join(EXPORT_PATH, file_system, acc_dir,
                acc_hash, cont_dir, cont_hash,
                obj_dir, DATADIR, obj_hash + '.data')
            meta_file_path = os.path.join(EXPORT_PATH, file_system, acc_dir,
                acc_hash, cont_dir, cont_hash,
                obj_dir, METADIR, obj_hash + '.meta')
            try:
                headers_out = HeaderKeyDict({'x-timestamp': str(time.time()), 'x-size': 0})
                os.remove(data_file_path)
                self.logger.debug(_('Sending request for container '
                    'update for BULK_DELETE object: %s, headers: %s' % \
                    (cont_hash, headers_out)))
                self.__recovery_handler_obj.bulk_delete_request(data_file_path, meta_file_path, obj_data, host)
            except OSError as ex:
                self.logger.error('In BULK_DELETE Data file delete \
                failed for: %s: %s' % (data_file_path, ex))
                if ex.errno == 2:
                    self.__recovery_handler_obj.bulk_delete_request(data_file_path, meta_file_path, obj_data, host)
        return HTTPOk(request=req)

    @public
    @timing_stats(sample_rate=0.8)
    @loggit('object-server')
    def HEAD(self, request):
        """Handle HTTP HEAD requests for the OSD Object Server."""
        filesystem, acc_dir, cont_dir, obj_dir, account, container, obj = \
            split_and_validate_path(request, 7, 7, True)
        try:
            disk_file = self.get_diskfile(
                filesystem, acc_dir, cont_dir, obj_dir, account, container, obj)
        except DiskFileDeviceUnavailable:
            return HTTPInsufficientStorage(drive=filesystem, request=request)

        full_path = '/%s/%s/%s' % (account, container, obj)
        obj_hash = "%s/%s/%s" % (hash_path(account), \
                                 hash_path(account, container), \
                                 hash_path(account, container, obj))
        #Extract headers from_proxy
        map_version = request.headers.get('X-GLOBAL-MAP-VERSION')
        component_no = request.headers.get('X-COMPONENT-NUMBER')
        host = request.headers.get('X-Container-Host')

        #to keep object-map updated
        if float_comp(self.__map_version_object, float(map_version)) == 0:
            self.__map_loaded_failure_case = False
        self.logger.info("component_no:%s, host:%s, map_version:%s" \
                % (component_no, host, map_version))
        data = HeaderKeyDict({'x-server-id' : self.__serv_id, \
            'x-object-path' : obj_hash, 'x-object_port' : self.__obj_port, \
            'x-mode' : 'r', 'x-operation' : 'HEAD', \
            'x-global-map-version': map_version, 'x-component-number': component_no})
        trans_id = ''
        try:
            resp = self.__http_comm.acquire_lock(host, filesystem, \
                cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
            resp.read()
            if not is_success(resp.status):
                self.logger.error((
                    'ERROR method %(method)s failed '
                    'response status: %(status)d '),
                    {'method': 'GET_LOCK', 'status': resp.status})
                if resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT, HTTP_CONFLICT]:
                    tries = 0
                    self.logger.debug("resp.status: %s" % resp.status)
                    while tries < self.retry_count:
                        tries += 1
                        if resp.status == HTTP_TEMPORARY_REDIRECT:
                            #eventlet sleep
                            sleep(self.wait_map_load)
                        if self.__map_loaded_failure_case == False:
                            try:
                                self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger, request)
                            except ValueError as err:
                                self.logger.error("Global map could not be loaded, exiting now")
                                return HTTPInternalServerError(request=request)

                            self.logger.debug("map: %s, version: %s, conn_obj: %s"% \
                                (self.__initial_map_load, self.__map_version_object, self.conn_obj))
                            self.__map_loaded_failure_case = True

                        host = get_host_from_map(account, container, self.__initial_map_load, \
                                    self.osd_dir, self.logger)
                        if not host:
                            return HTTPServiceUnavailable(request=request)

                        map_version = self.__map_version_object
                        self.logger.info("host: %s, map_version: %s"% (host, map_version))
                        data.update({'x-global-map-version': map_version})
                        data.update({'x-component-number': component_no})
                        resp = self.__http_comm.acquire_lock(host, filesystem, cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
                        resp.read()
                        if not is_success(resp.status):
                            self.__map_loaded_failure_case = False
                            if resp.status == HTTP_CONFLICT:
                                sleep(self.conflict_timeout)
                                continue
                                #return HTTPConflict(request=request)

                            elif resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT]:
                                continue

                            else:
                                return HTTPServiceUnavailable(request=request)
                        else:
                            break

                else:
                    return HTTPServiceUnavailable(request=request)

                if resp.status == HTTP_CONFLICT:
                    return HTTPConflict(request=request)

            trans_id = self.__http_comm.get_transaction_id_from_resp(resp)
            if not trans_id:
                self.logger.error(_(
                'ERROR method acquire_lock failed with'
                '%(host)s/%(fs)s/%(dir)s'),
                {'host': host, 'fs': filesystem, 'dir' : account})
                return HTTPServiceUnavailable(request=request)
        except(Exception, Timeout) as err:
            self.logger.exception(_(
            'ERROR method GET_LOCK failed with'
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
            if isinstance(err.__class__, type(socket.error)) and ('ECONNREFUSED' in str(err)):
                status, respond = self.service_unavailable_retry(request, filesystem, \
                    cont_dir, account, container, data)
                if status:
                    return respond(request=request)
                else:
                    trans_id = respond # it is transaction id in this case
            else:
                return HTTPServiceUnavailable(request=request)

        try:
            self.__lock_obj.add(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory add fail for path:'
            '%(full_path)s)'),
            {'full_path': full_path})
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.release_lock(host, filesystem, cont_dir, headers)
            return HTTPServerError(request=request)
 
        try:
            metadata = disk_file.read_metadata()
        except (DiskFileNotExist, DiskFileQuarantined):
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            return HTTPNotFound(request=request, conditional_response=True)
        except Exception:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            raise
        response = Response(request=request, conditional_response=True)
        response.headers['Content-Type'] = metadata.get(
            'Content-Type', 'application/octet-stream')
        for key, value in metadata.iteritems():
            if is_user_meta('object', key) or \
                    key.lower() in self.allowed_headers:
                response.headers[key] = value
        response.etag = metadata['ETag']
        ts = metadata['X-Timestamp']
        response.last_modified = math.ceil(float(ts))
        # Needed for container sync feature
        response.headers['X-Timestamp'] = ts
        response.content_length = int(metadata['Content-Length'])
        try:
            response.content_encoding = metadata['Content-Encoding']
        except KeyError:
            pass

        try:
            data = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.__http_comm.release_lock(host, filesystem, \
                cont_dir, data)
        except(Exception, Timeout):
            self.logger.exception(_(
            'ERROR method FREE_LOCK failed with '
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
        try:
            self.__lock_obj.delete(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory delete fail with transcation ID:'
            '%(transcation_ID)s)'),
            {'transcation_ID': trans_id})
        return response

    @public
    @timing_stats()
    @loggit('object-server')
    def DELETE(self, request):
        """Handle HTTP DELETE requests for the OSD Object Server."""
        filesystem, acc_dir, cont_dir, obj_dir, account, container, obj = \
            split_and_validate_path(request, 7, 7, True)
        if 'x-timestamp' not in request.headers or \
                not check_float(request.headers['x-timestamp']):
            return HTTPBadRequest(body='Missing timestamp', request=request,
                                  content_type='text/plain')
        try:
            ret = None
            disk_file = self.get_diskfile(
                filesystem, acc_dir, cont_dir, obj_dir, account, container, obj)
        except DiskFileDeviceUnavailable:
            return HTTPInsufficientStorage(drive=filesystem, request=request)

        full_path = '/%s/%s/%s' % (account, container, obj)
        obj_hash = "%s/%s/%s" % (hash_path(account), \
                                 hash_path(account, container), \
                                 hash_path(account, container, obj))
        #map_from_proxy
        map_version = request.headers.get('X-GLOBAL-MAP-VERSION')
        component_no = request.headers.get('X-COMPONENT-NUMBER')
        #to keep object-map updated
        if float_comp(self.__map_version_object, float(map_version)) == 0:
            self.__map_loaded_failure_case = False


        host = request.headers.get('X-Container-Host')
        self.logger.info("host: %s, map_version: %s"% (host, map_version))
        data = HeaderKeyDict({'x-server-id' : self.__serv_id, \
            'x-object-path' : obj_hash, 'x-object_port' : self.__obj_port, \
            'x-mode' : 'w', 'x-operation' : 'DELETE', \
            'x-global-map-version': map_version, \
            'x-component-number': component_no})
        trans_id = ''
        try:
            resp = self.__http_comm.acquire_lock(host, filesystem, \
                cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
            resp.read()
            if not is_success(resp.status):
                self.logger.error((
                    'ERROR method %(method)s failed '
                    'response status: %(status)d '),
                    {'method': 'GET_LOCK', 'status': resp.status})
                if resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT, HTTP_CONFLICT]:
                    tries = 0
                    self.logger.debug("resp.status: %s" % resp.status)
                    while tries < self.retry_count:
                        tries += 1
                        if resp.status == HTTP_TEMPORARY_REDIRECT:
                            #eventlet sleep
                            sleep(self.wait_map_load)
                        if self.__map_loaded_failure_case == False:
                            try:
                                self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger, request)
                            except ValueError as err:
                                self.logger.error("Global map could not be loaded, exiting now")
                                return HTTPInternalServerError(request=request)
                            self.logger.debug("map: %s, version: %s, conn_obj: %s"% \
                                (self.__initial_map_load, self.__map_version_object, self.conn_obj))
                            self.__map_loaded_failure_case = True

                        host = get_host_from_map(account, container, self.__initial_map_load, \
                                    self.osd_dir, self.logger)
                        if not host:
                            return HTTPServiceUnavailable(request=request)

                        request.headers['X-Container-Host'] = host
                        map_version = self.__map_version_object
                        self.logger.info("host: %s, map_version: %s"% (host, map_version))
                        data.update({'x-global-map-version': map_version})
                        resp = self.__http_comm.acquire_lock(host, filesystem, cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
                        resp.read()
                        if not is_success(resp.status):
                            self.__map_loaded_failure_case = False
                            if resp.status == HTTP_CONFLICT:
                                sleep(self.conflict_timeout)
                                continue
                                #return HTTPConflict(request=request)

                            elif resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT]:
                                continue

                            else:
                                return HTTPServiceUnavailable(request=request)
                        else:
                            break

                else:
                    return HTTPServiceUnavailable(request=request)

                if resp.status == HTTP_CONFLICT:
                    return HTTPConflict(request=request)

            trans_id = self.__http_comm.get_transaction_id_from_resp(resp)
            if not trans_id:
                self.logger.error(_(
                'ERROR method acquire_lock failed with'
                '%(host)s/%(fs)s/%(dir)s'),
                {'host': host, 'fs': filesystem, 'dir' : account})
                return HTTPServiceUnavailable(request=request)
        except(Exception, Timeout) as err:
            self.logger.exception(_(
            'ERROR method GET_LOCK failed with'
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
            if isinstance(err.__class__, type(socket.error)) and ('ECONNREFUSED' in str(err)):
                status, respond = self.service_unavailable_retry(request, filesystem, \
                    cont_dir, account, container, data)
                if status:
                    return respond(request=request)
                else:
                    trans_id = respond # it is transaction id in this case
            else:
                return HTTPServiceUnavailable(request=request)

        try:
            self.__lock_obj.add(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory add fail for path:'
            '%(full_path)s)'),
            {'full_path': full_path})
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.release_lock(host, filesystem, cont_dir, headers)
            return HTTPServerError(request=request)

        try:
            orig_metadata = disk_file.read_metadata()
        except DiskFileExpired as e:
            orig_timestamp = e.timestamp
            orig_metadata = e.metadata
            response_class = HTTPNotFound
        except DiskFileDeleted as e:
            orig_timestamp = e.timestamp
            orig_metadata = {}
            response_class = HTTPNotFound
        except (DiskFileNotExist, DiskFileQuarantined):
            orig_timestamp = 0
            orig_metadata = {}
            response_class = HTTPNotFound
        except Exception:
            headers = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.remove_trans_id(obj_hash, trans_id)
            self.release_lock(host, filesystem, cont_dir, headers)
            #self.remove_trans_id(trans_id)
            raise
        else:
            orig_timestamp = orig_metadata.get('X-Timestamp', 0)
            if orig_timestamp < request.headers['x-timestamp']:
                response_class = HTTPNoContent
            else:
                response_class = HTTPConflict
        req_timestamp = request.headers['X-Timestamp']
        if orig_timestamp < req_timestamp:
            if disk_file.delete('data'):
                headers_out = HeaderKeyDict({'x-timestamp': req_timestamp, \
                                    'x-size': orig_metadata['Content-Length'], \
                                    'x-global-map-version': map_version, \
                                    'x-component-number': component_no})
                self.logger.debug(_('Sending request for container '
                    'update for DELETE object: %s, headers: %s' % \
                    (request.path, headers_out)))

                #TODO(jaivish:retry 3 times)
                retry = 0 
                while retry <= self.retry_count:
                    ret = self.container_update(
                        'DELETE', account, container, obj, request,
                        headers_out)
                    if ret == 'redirect' and retry == self.retry_count:
                        #In case every try fail the give user created..
                        ret = False
                    if ret == False:
                        try:
                            self.logger.error(_('ERROR container update failed '
                                'for DELETE object: %s' % request.path))
                            self.__lock_obj.delete(obj_hash, trans_id)
                            return HTTPAccepted(request=request)
                        except Exception:
                            self.logger.exception(_(
                                'ERROR object server memory deletion fail'
                                'for transcation ID:'
                                '%(transcation_ID)s)'),
                                {'transcation_ID': trans_id})
                            break
                    elif ret == 'redirect':
                        retry += 1
                        try:
                            self.__initial_map_load, self.__map_version_object, self.conn_obj = \
                                get_global_map(self.__serv_id, self.logger, request, time_=self.wait_map_load)
                        except ValueError as err:
                            self.logger.error("Global map could not be loaded, exiting now")
                            self.__lock_obj.delete(obj_hash, trans_id)
                            return HTTPAccepted(request=request)
                            #return HTTPInternalServerError(request=request)
                        map_version = self.__map_version_object
                        headers_out.update({'x-global-map-version': map_version})
                        try:
                            host = get_host_from_map(account, container, self.__initial_map_load, \
                                        self.osd_dir, self.logger)
                            if not host:
                                self.__lock_obj.delete(obj_hash, trans_id)
                                return HTTPAccepted(request=request)
                        except Exception as err:
                            self.__lock_obj.delete(obj_hash, trans_id)
                            return HTTPAccepted(request=request)
                            #return HTTPServiceUnavailable(request=request)
                        self.logger.info('Calculated host to redirect: %s %s'% (host, map_version))
                        request.headers['X-Container-Host'] = host
                        continue
                        #break when success or continue to retry
                    else:
                        self.logger.debug(_('Container update successful for '
                            'DELETE object: %s' % request.path))
                        if not disk_file.delete('meta'):
                            try:
                                self.logger.debug('ERROR Meta file DELETE '
                                                  'failed for %(object)s '
                                                  'for %(trans_id)s',
                                                  {'object': request.path,
                                                   'trans_id': trans_id})
                                self.__lock_obj.delete(obj_hash, trans_id)
                                return HTTPAccepted(request=request)
                            except Exception:
                                self.logger.exception(_(
                                    'ERROR object server memory deletion fail'
                                    'for transcation ID:'
                                    '%(transcation_ID)s)'),
                                    {'transcation_ID': trans_id})
                        break

            else:
                self.logger.debug('ERROR Data file DELETE failed for %(object)s'
                                  ' for %(trans_id)s',
                                  {'object': request.path, 'trans_id': trans_id})
        try:
            data = HeaderKeyDict({'x-object-path' : obj_hash, \
                'x-request-trans-id' : trans_id, \
                'x-global-map-version': map_version, \
                'x-component-number': component_no})
            self.__http_comm.release_lock(host, filesystem, \
                cont_dir, data)
        except(Exception, Timeout):
            self.logger.exception(_(
            'ERROR method FREE_LOCK failed with '
            '%(host)s/%(fs)s/%(dir)s)'),
            {'host': host, 'fs': filesystem, 'dir' : account})
        try:
            self.__lock_obj.delete(obj_hash, trans_id)
        except Exception:
            self.logger.exception(_(
            'ERROR object server memory delete fail with transcation ID:'
            '%(transcation_ID)s)'),
            {'transcation_ID': trans_id})
        return response_class(request=request)

    @public
    @timing_stats()
    @loggit('object-server')
    def RECOVER(self, request):
        """Handle HTTP RECOVER requests for the OSD Object Server."""
        self.logger.debug("RECOVER with headers: %s " % request.headers)
        method = request.headers.get('x-operation')
        trans_id = request.headers.get('x-request-trans-id')
        container_service_id = request.headers.get('x-server-id')
        host = '%(ip)s:%(port)s' % get_ip_port_by_id(container_service_id, self.logger)
        #exception handling for extenal interface
        obj_path = request.headers.get('x-object-path')
        map_version = request.headers.get('x-global-map-version')
        self.logger.info("recovering path: %s container_service_id: %s, host: %s " \
                % (obj_path, container_service_id, host)) 
        check_existense = self.__lock_obj.is_exists(obj_path)
        if check_existense:
            return HTTPOk(request=request)
        else:
            if method in ('GET', 'HEAD', 'POST'):
                data = HeaderKeyDict({'x-object-path' : obj_path, \
                    'x-request-trans-id' : trans_id,
                    'x-global-map-version': map_version})
                try:
                    self.__http_comm.release_lock(host, '', \
                        '', data)
                except(Exception, Timeout):
                    self.logger.exception(_(
                    'ERROR method FREE_LOCK failed with '
                    '%(host)s)'),
                    {'host': host})
            else:
                self.__recovery_handler_obj.triggered_recovery(obj_path, method, host)
        return HTTPOk(request=request)



    def __call__(self, env, start_response):
        """WSGI Application entry point for the OSD Object Server."""
        start_time = time.time()
        req = Request(env)
        self.logger.txn_id = req.headers.get('x-trans-id', None)

        if not check_utf8(req.path_info):
            res = HTTPPreconditionFailed(body='Invalid UTF8 or contains NULL')
        else:
            try:
                # disallow methods which have not been marked 'public'
                try:
                    method = getattr(self, req.method)
                    getattr(method, 'publicly_accessible')
                    #replication_method = getattr(method, 'replication', False)
                except AttributeError:
                    res = HTTPMethodNotAllowed()
                else:
                    if self.node_delete == True:
                        res = HTTPMovedPermanently()
                        self.logger.info("Node Deletion in progress")
                    else:
                        # check map_version of proxy
                        map_version = req.headers.get('X-GLOBAL-MAP-VERSION','None')
                        self.logger.debug("req headers: %s " % req.headers)
                        if map_version == 'None':
                            self.logger.error(_('Map version from header %s is not received'% map_version))
                            res = HTTPMovedPermanently()

                        elif map_version and float_comp(self.__map_version_object, float(map_version)) > 0:
                            self.logger.error(_('Map version of proxy is obsolete %s setting %s version now' % \
                                (map_version, self.__map_version_object)))
                            #In case map version is old then do not deny request as it is not
                            #handled at proxy level, so set local version.
                            req.headers['X-GLOBAL-MAP-VERSION'] = self.__map_version_object
                            res = method(req)
                        else:
                            res = method(req)

            except DiskFileCollision:
                res = HTTPForbidden(request=req)
            except HTTPException as error_response:
                res = error_response
            except (Exception, Timeout):
                self.logger.exception(_(
                    'ERROR __call__ error with %(method)s'
                    ' %(path)s '), {'method': req.method, 'path': req.path})
                res = HTTPInternalServerError(body=traceback.format_exc())
        trans_time = time.time() - start_time
        if self.log_requests:
            log_line = '%s - - [%s] "%s %s" %s %s "%s" "%s" "%s" %.4f' % (
                req.remote_addr,
                time.strftime('%d/%b/%Y:%H:%M:%S +0000',
                              time.gmtime()),
                req.method, req.path, res.status.split()[0],
                res.content_length or '-', req.referer or '-',
                req.headers.get('x-trans-id', '-'),
                req.user_agent or '-',
                trans_time)
            self.logger.info(log_line)
        if self.log_failure and not self.log_requests:
            if not is_success(res.status_int): 
                log_line = '%s - - [%s] "%s %s" %s %s "%s" "%s" "%s" %.4f' % (
                    req.remote_addr,
                    time.strftime('%d/%b/%Y:%H:%M:%S +0000',
                                  time.gmtime()),
                    req.method, req.path, res.status.split()[0],
                    res.content_length or '-', req.referer or '-',
                    req.headers.get('x-trans-id', '-'),
                    req.user_agent or '-',
                    trans_time)
                self.logger.error(log_line)
        if req.method in ('PUT', 'DELETE'):
            slow = self.slow - trans_time
            if slow > 0:
                sleep(slow)
        return res(env, start_response)

    #TODO:jai:-node delete flow : STOP_SERVICE to be called by GL.

    @public
    @timing_stats()
    @loggit('object-server')
    def STOP_SERVICE(self, request):
        """
        Set node delete paramater to be true
        :param http request 
        return HTTPOk: when all operations completed
        """
        self.logger.info("STOP SERVICE request received")
        self.__set_node_delete(True)
        request.headers.update({'Message-Type': typeEnums.BLOCK_NEW_REQUESTS_ACK})
        try:
            if self.__check_empty():
                return HTTPOk(request=request, headers = {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK})

        except Exception as e:
            self.logger.error(_(
                'ERROR check_empty trans id'
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': e, 'stack': ''.join(traceback.format_stack()),
                })
            return HTTPInternalServerError(request=request, headers = {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK})

    def __check_empty(self):
        """
        Check for In-Memory trans id's.
        return True: when operations completed and no trans_id 
        is left in memory
        """
        self.logger.info("Checking Dictionary size of trans id's")
        while 1:
            if self.__lock_obj.is_empty():
                return True
            #eventlet sleep...
            sleep(WAIT_TIME_FOR_TRANSID_CHECK)

    def __set_node_delete(self, node_delete = False):
        """
        set node delete_true
        :param node_delete=True when request for node_delete
        is received
        """
        self.node_delete = node_delete

    def callback_map_variables_update(self, global_map, global_map_version, conn_obj):
        """
        Update global variables from TransactionManager Class's local vars
        """
        self.__initial_map_load = global_map
        self.__map_version_object = global_map_version
        self.conn_obj = conn_obj
        self.logger.debug("Parameters: %s %s %s" % \
            (self.__initial_map_load, self.__map_version_object, self.conn_obj))
        return 0
        


    def service_unavailable_retry(self, request, filesystem, cont_dir, account, container, data):
        retry = 0
        while retry < self.retry_count:
            self.logger.debug("retrying %sst time on service unavailable" % retry)
            sleep(self.node_timeout)
            try:
                retry += 1
                #Modifications for single map req to GL are to be done.
                try:
                    self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger, request)
                except ValueError as err:
                    self.logger.error("Global map could not be loaded, exiting now")
                    return True, HTTPInternalServerError
                self.logger.debug("map: %s, version: %s, conn_obj: %s" % \
                    (self.__initial_map_load, self.__map_version_object, self.conn_obj))
                host = get_host_from_map(account, container, self.__initial_map_load, \
                                self.osd_dir, self.logger)
                if not host:
                    return True, HTTPServiceUnavailable

                map_version = self.__map_version_object
                self.logger.info("host: %s, map_version: %s"% (host, map_version))
                data.update({'x-global-map-version': map_version})

                resp = self.__http_comm.acquire_lock(host, filesystem, \
                    cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
                resp.read()
                if not is_success(resp.status):
                    self.logger.error((
                        'ERROR method %(method)s failed '
                        'response status: %(status)d '),
                        {'method': 'GET_LOCK', 'status': resp.status})
                    if resp.status in [HTTP_MOVED_PERMANENTLY, HTTP_TEMPORARY_REDIRECT, HTTP_CONFLICT]:
                        tries = 0
                        self.logger.debug("resp.status: %s" % resp.status)
                        while tries < self.retry_count:
                            tries += 1
                            if resp.status == HTTP_TEMPORARY_REDIRECT:
                                #eventlet sleep
                                sleep(self.wait_map_load)
                            if self.__map_loaded_failure_case == False:
                                try:
                                    self.__initial_map_load, self.__map_version_object, self.conn_obj = get_global_map(self.__serv_id, self.logger, request)
                                except ValueError as err:
                                    self.logger.error("Global map could not be loaded, exiting now")
                                    return True, HTTPInternalServerError

                                self.logger.debug("map: %s, version: %s, conn_obj: %s"% \
                                    (self.__initial_map_load, self.__map_version_object, self.conn_obj))
                                self.__map_loaded_failure_case = True

                            host = get_host_from_map(account, container, self.__initial_map_load, \
                                        self.osd_dir, self.logger)
                            if not host:
                                return True, HTTPServiceUnavailable

                            map_version = self.__map_version_object
                            self.logger.info("host: %s, map_version: %s"% (host, map_version))
                            data.update({'x-global-map-version': map_version})
                            resp = self.__http_comm.acquire_lock(host, filesystem, cont_dir, data, self.callback_map_variables_update, request, self.__serv_id, account, container, self.osd_dir)
                            resp.read()
                            if not is_success(resp.status):
                                self.__map_loaded_failure_case = False
                                if resp.status == HTTP_CONFLICT:
                                    sleep(self.conflict_timeout)
                                    continue
                                    #return True, HTTPConflict
                                    #return HTTPConflict(request=request)

                                elif resp.status in [HTTP_TEMPORARY_REDIRECT, HTTP_MOVED_PERMANENTLY]:
                                    continue
                                else:
                                    return True, HTTPServiceUnavailable
                                    #return HTTPServiceUnavailable(request=request)
                            else:
                                break

                    elif resp.status == HTTP_CONFLICT:
                        #return HTTPConflict(request=request)
                        return True, HTTPConflict
                    else:
                        #return HTTPServiceUnavailable(request=request)
                        return True, HTTPServiceUnavailable

                trans_id = self.__http_comm.get_transaction_id_from_resp(resp)
                if not trans_id:
                    self.logger.error(_(
                    'ERROR method acquire_lock failed with'
                    '%(host)s/%(fs)s/%(dir)s'),
                    {'host': host, 'fs': filesystem, 'dir' : account})
                    #return HTTPServiceUnavailable(request=request)
                    return True, HTTPServiceUnavailable
                break
            except(Exception, Timeout) as err:
                self.logger.exception(_(
                'ERROR method GET_LOCK failed with'
                '%(fs)s/%(dir)s : %(ex)s)'),
                {'fs': filesystem, 'dir' : account, 'ex': err})
                if isinstance(err.__class__, type(socket.error)) and ('ECONNREFUSED' in str(err)):
                    continue
                else:
                    return True, HTTPServiceUnavailable

        if retry == self.retry_count:
            return True, HTTPServiceUnavailable

        return False, trans_id



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
    """paste.deploy app factory for creating WSGI object server apps"""
    conf = global_conf.copy()
    conf.update(local_conf)
    return ObjectController(conf)

