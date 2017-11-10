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

import mimetypes
import os
import socket
from osd import gettext_ as _
#from random import shuffle
import time
#import itertools

from eventlet import Timeout
from ConfigParser import ConfigParser, RawConfigParser
from osd.common.bulk_delete import get_objs_to_delete
from osd.common import constraints
from osd.common.ring.ring import ObjectRing, ContainerRing, AccountRing
from osd.common.utils import cache_from_env, get_logger, \
    config_true_value, generate_trans_id, list_from_csv, register_osd_info, \
    read_conf_dir, create_recovery_file, remove_recovery_file
from osd.common.helper_functions import split_path
from osd.common.constraints import check_utf8
from osd.proxyService.controllers import AccountController, ObjectController, \
    ContainerController, StopController
from osd.common.swob import HTTPBadRequest, HTTPForbidden, \
    HTTPMethodNotAllowed, HTTPNotFound, HTTPPreconditionFailed, \
    HTTPServerError, HTTPException, Request
from libHealthMonitoringLib import healthMonitoring
import subprocess
from osd.containerService.utils import SingletonType
import libraryUtils
from osd.glCommunicator.message_type_enum_pb2 import *

# List of entry points for mandatory middlewares.
#
# Fields:
#
# "name" (required) is the entry point name from setup.py.
#
# "after_fn" (optional) a function that takes a PipelineWrapper object as its
# single argument and returns a list of middlewares that this middleware
# should come after. Any middlewares in the returned list that are not present
# in the pipeline will be ignored, so you can safely name optional middlewares
# to come after. For example, ["catch_errors", "bulk"] would install this
# middleware after catch_errors and bulk if both were present, but if bulk
# were absent, would just install it after catch_errors.

required_filters = [
    {'name': 'catch_errors'},
    {'name': 'gatekeeper',
     'after_fn': lambda pipe: (['catch_errors']
                               if pipe.startswith("catch_errors")
                               else [])},
    #{'name': 'cache', 'after_fn': lambda _junk: ['catch_errors', 'gatekeeper']},
    {'name': 'name_check'}]
    #{'name': 'authtoken'}, {'name': 'keystoneauth'}]


class Application(object):
    """WSGI application for the proxy server."""
    __metaclass__ = SingletonType

    def __init__(self, conf, memcache=None, logger=None, account_ring=None,
                 container_ring=None, object_ring=None):
        if conf is None:
            conf = {}
        if logger is None:
            self.logger = get_logger(conf, log_route='proxy-server')
        else:
            self.logger = logger
        libraryUtils.OSDLoggerImpl("proxy-monitoring").initialize_logger()
        create_recovery_file('proxy-server')
        self.ongoing_operation_list = []
        self.stop_service_flag = False
        osd_dir = conf.get('osd_dir', '/export/.osd_meta_config')
        static_config_file = conf.get('static_config_file', \
            '/opt/HYDRAstor/objectStorage/configFiles/static_proxy-server.conf')
        #swift_dir = conf.get('swift_dir', '/etc/swift')
        static_conf = self.readconf(static_config_file)
        self.logger.debug("Static config parameters:%s" %(static_conf))
        self.node_timeout = int(conf.get('node_timeout', 10))
        self.recoverable_node_timeout = int(
            conf.get('recoverable_node_timeout', self.node_timeout))
        self.conn_timeout = float(conf.get('conn_timeout', 0.5))
        self.client_timeout = int(conf.get('client_timeout', 60))
        self.put_queue_depth = int(static_conf.get('put_queue_depth', 5))
        self.retry_count = int(static_conf.get('retry_count', 3))
        self.request_retry_time_service_unavailable = int(\
            static_conf.get('request_retry_time_service_unavailable', 100))
        self.request_retry_time_component_transfer = int(\
            static_conf.get('request_retry_time_component_transfer', 50))
        self.object_chunk_size = int(conf.get('object_chunk_size', 65536))
        self.client_chunk_size = int(conf.get('client_chunk_size', 65536))
        self.trans_id_suffix = conf.get('trans_id_suffix', '')
        self.post_quorum_timeout = float(conf.get('post_quorum_timeout', 0.5))
        #self.error_suppression_interval = \
        #    int(conf.get('error_suppression_interval', 60))
        #self.error_suppression_limit = \
        #    int(conf.get('error_suppression_limit', 10))
        self.recheck_container_existence = \
            int(conf.get('recheck_container_existence', 60))
        self.recheck_account_existence = \
            int(conf.get('recheck_account_existence', 60))
        self.allow_account_management = \
            config_true_value(static_conf.get('allow_account_management', 'no'))
        self.object_post_as_copy = \
            config_true_value(conf.get('object_post_as_copy', 'false'))
        self.object_ring = object_ring or ObjectRing(osd_dir, self.logger, \
                                          self.node_timeout)
        self.container_ring = container_ring or \
        ContainerRing(osd_dir, self.logger, self.node_timeout)
        self.account_ring = account_ring or AccountRing(osd_dir, self.logger, \
                                          self.node_timeout)
        self.memcache = memcache
        mimetypes.init(mimetypes.knownfiles +
                       [os.path.join(osd_dir, 'mime.types')])
        self.account_autocreate = \
            config_true_value(static_conf.get('account_autocreate', 'yes'))
        #self.expiring_objects_account = \
        #    (conf.get('auto_create_account_prefix') or '.') + \
        #    (conf.get('expiring_objects_account_name') or 'expiring_objects')
        #self.expiring_objects_container_divisor = \
        #    int(conf.get('expiring_objects_container_divisor') or 86400)
        self.max_containers_per_account = \
            int(conf.get('max_containers_per_account') or 10000000)
        self.max_containers_whitelist = [
            a.strip()
            for a in conf.get('max_containers_whitelist', '').split(',')
            if a.strip()]
        self.deny_host_headers = [
            host.strip() for host in
            conf.get('deny_host_headers', '').split(',') if host.strip()]
        #self.rate_limit_after_segment = \
        #    int(conf.get('rate_limit_after_segment', 10))
        #self.rate_limit_segments_per_sec = \
        #    int(conf.get('rate_limit_segments_per_sec', 1))
        #self.log_handoffs = config_true_value(conf.get('log_handoffs', 'true'))
        #self.cors_allow_origin = [
        #    a.strip()
        #    for a in conf.get('cors_allow_origin', '').split(',')
        #    if a.strip()]
        #self.strict_cors_mode = config_true_value(
        #    conf.get('strict_cors_mode', 't'))
        self.node_timings = {}
        self.timing_expiry = int(conf.get('timing_expiry', 300))
        #self.sorting_method = conf.get('sorting_method', 'shuffle').lower()
        #self.max_large_object_get_time = float(
        #    conf.get('max_large_object_get_time', '86400'))
        #value = conf.get('request_node_count', '2 * replicas').lower().split()
        #if len(value) == 1:
        #    value = int(value[0])
        #    self.request_node_count = lambda replicas: value
        #elif len(value) == 3 and value[1] == '*' and value[2] == 'replicas':
        #    value = int(value[0])
        #    self.request_node_count = lambda replicas: value * replicas
        #else:
        #    raise ValueError(
        #        'Invalid request_node_count value: %r' % ''.join(value))
        #try:
        #    self._read_affinity = read_affinity = conf.get('read_affinity', '')
        #    self.read_affinity_sort_key = affinity_key_function(read_affinity)
        #except ValueError as err:
        #    # make the message a little more useful
        #    raise ValueError("Invalid read_affinity value: %r (%s)" %
        #                     (read_affinity, err.message))
        #try:
        #    write_affinity = conf.get('write_affinity', '')
        #    self.write_affinity_is_local_fn \
        #        = affinity_locality_predicate(write_affinity)
        #except ValueError as err:
        #    # make the message a little more useful
        #    raise ValueError("Invalid write_affinity value: %r (%s)" %
        #                     (write_affinity, err.message))
        #value = conf.get('write_affinity_node_count',
        #                 '2 * replicas').lower().split()
        #if len(value) == 1:
        #    value = int(value[0])
        #    self.write_affinity_node_count = lambda replicas: value
        #elif len(value) == 3 and value[1] == '*' and value[2] == 'replicas':
        #    value = int(value[0])
        #    self.write_affinity_node_count = lambda replicas: value * replicas
        #else:
        #    raise ValueError(
        #        'Invalid write_affinity_node_count value: %r' % ''.join(value))
        # swift_owner_headers are stripped by the account and container
        # controllers; we should extend header stripping to object controller
        # when a privileged object header is implemented.
        swift_owner_headers = conf.get(
            'swift_owner_headers',
            'x-container-read, x-container-write, '
            'x-container-sync-key, x-container-sync-to, '
            'x-account-meta-temp-url-key, x-account-meta-temp-url-key-2, '
            'x-account-access-control')
        self.swift_owner_headers = [
            name.strip().title()
            for name in swift_owner_headers.split(',') if name.strip()]
        # Initialization was successful, so now apply the client chunk size
        # parameter as the default read / write buffer size for the network
        # sockets.
        #
        # NOTE WELL: This is a class setting, so until we get set this on a
        # per-connection basis, this affects reading and writing on ALL
        # sockets, those between the proxy servers and external clients, and
        # those between the proxy servers and the other internal servers.
        #
        # ** Because it affects the client as well, currently, we use the
        # client chunk size as the govenor and not the object chunk size.
        socket._fileobject.default_bufsize = self.client_chunk_size
        self.expose_info = config_true_value(
            conf.get('expose_info', 'yes'))
        self.disallowed_sections = list_from_csv(
            conf.get('disallowed_sections'))
        self.admin_key = conf.get('admin_key', None)
        register_osd_info(
            max_file_size=constraints.MAX_FILE_SIZE,
            max_meta_name_length=constraints.MAX_META_NAME_LENGTH,
            max_meta_value_length=constraints.MAX_META_VALUE_LENGTH,
            max_meta_count=constraints.MAX_META_COUNT,
            account_listing_limit=constraints.ACCOUNT_LISTING_LIMIT,
            container_listing_limit=constraints.CONTAINER_LISTING_LIMIT,
            max_account_name_length=constraints.MAX_ACCOUNT_NAME_LENGTH,
            max_container_name_length=constraints.MAX_CONTAINER_NAME_LENGTH,
            max_object_name_length=constraints.MAX_OBJECT_NAME_LENGTH,
            non_allowed_headers=constraints.NON_ALLOWED_HEADERS)

        self.proxy_port = int(static_conf.get('bind_port', 61005))
        self.__ll_port = int(conf.get('llport', 61014))

        self.max_bulk_delete_entries = int(conf.get(\
        'max_bulk_delete_entries', 1000))


        #unblock new requests which was blocked due to proxy service stop
        self.__request_unblock()

        hostname = socket.gethostname()
        self.__server_id = hostname + "_" + str(self.__ll_port) + "_proxy-server"

        # Start sending health to local leader
        self.logger.info("Loading health monitoring library")
        self.health_instance = healthMonitoring(self.__get_node_ip(hostname), \
            self.proxy_port, self.__ll_port, self.__server_id, True)
        self.logger.info("Loaded health monitoring library")
        remove_recovery_file('proxy-server')
    #def check_config(self):
    #    """
    #    Check the configuration for possible errors
    #    """
    #    if self._read_affinity and self.sorting_method != 'affinity':
    #        self.logger.warn("sorting_method is set to '%s', not 'affinity'; "
    #                         "read_affinity setting will have no effect." %
    #                         self.sorting_method)

    def readconf(self, conf_path, defaults=None, raw=False):
        """
        Read config file(s) and return config items as a dict

        :param conf_path: path to config file/directory, or a file-like object
                    (hasattr readline)
        :param defaults: dict of default values to pre-populate the config with
        :returns: dict of config items
        """
        conf = {}
        if defaults is None:
            defaults = {}
        if raw:
            c = RawConfigParser(defaults)
        else:
            c = ConfigParser(defaults)
        if hasattr(conf_path, 'readline'):
            c.readfp(conf_path)
        else:
            if os.path.isdir(conf_path):
                # read all configs in directory
                success = read_conf_dir(c, conf_path)
            else:
                success = c.read(conf_path)
            if not success:
                self.logger.debug("Unable to read config from %s" % conf_path)
                return conf
            for s in c.sections():
                conf.update(c.items(s))
        conf['__file__'] = conf_path
        return conf

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
            self.logger.exception(_('Error while getting ip of node:%s' %err))
            return ""

    def __request_unblock(self):
        """
        Unblock new requests which was blocked 
        due to proxy service stop 
        """
        command = "/sbin/iptables --list | grep " + str(self.proxy_port)
        std_out, std_err, ret_val = self.execute(command)
        if not ret_val:
            command = "/sbin/iptables -D OUTPUT -p tcp --dport " + \
                str(self.proxy_port) + " -m state --state NEW -j REJECT"
            std_out, std_err, ret_val = self.execute(command)
            if ret_val:
                self.logger.error("Error occured while executing command:%s, \
                    error:%s return value:%s" %(command, std_err, ret_val))
            command = "/sbin/iptables -D INPUT -p tcp --sport " + \
                str(self.proxy_port) + " -m state --state ESTABLISHED -j ACCEPT"
            std_out, std_err, ret_val = self.execute(command)
            if ret_val:
                self.logger.error("Error occured while executing command:%s, \
                    error:%s return value:%s" %(command, std_err, ret_val))

    def request_block(self):
        """
        Block new requests without hampering already 
        established requests as proxy service is going to stop 
        """
        command = "/sbin/iptables --list | grep " + str(self.proxy_port)
        std_out, std_err, ret_val = self.execute(command)
        if ret_val:
            command = "/sbin/iptables -A OUTPUT -p tcp --dport " + \
                str(self.proxy_port) + " -m state --state NEW -j REJECT"
            std_out, std_err, ret_val = self.execute(command)
            if ret_val:
                self.logger.error("Error occured while executing command:%s, \
                    error:%s return value:%s" %(command, std_err, ret_val))

            command = "/sbin/iptables -A INPUT -p tcp --sport " + \
                str(self.proxy_port) + " -m state --state ESTABLISHED -j ACCEPT"
            std_out, std_err, ret_val = self.execute(command)
            if ret_val:
                self.logger.error("Error occured while executing command:%s, \
                    error:%s return value:%s" %(command, std_err, ret_val))


    def execute(self, command):
        """
        Executing command
        """
        try:
            child = subprocess.Popen(command, stdout = subprocess.PIPE, \
                stderr = subprocess.PIPE, shell = True)
            std_out, std_err = child.communicate()
            ret_val = child.returncode
            return std_out, std_err, ret_val
        except Exception as err:
            self.logger.exception(_('While executing command:%s, error:%s' \
            %(command, err)))
            return '', '', ''

    def get_controller(self, path, method):
        """
        Get the controller to handle a request.

        :param path: path from request
        :returns: tuple of (controller class, path dictionary)

        :raises: ValueError (thrown by split_path) if given invalid path
        """

        version, account, container, obj = split_path(path, 1, 4, True)
        d = dict(version=version,
                 account_name=account,
                 container_name=container,
                 object_name=obj)
        if method == "STOP_SERVICE":
            return StopController, d
        if obj and container and account:
            return ObjectController, d
        elif container and account:
            return ContainerController, d
        elif account and not container and not obj:
            return AccountController, d
        return None, d

    def __call__(self, env, start_response):
        """
        WSGI entry point.
        Wraps env in swob.Request object and passes it down.

        :param env: WSGI environment dictionary
        :param start_response: WSGI callable
        """
        try:
            if self.memcache is None:
                self.memcache = cache_from_env(env)
            #req = self.update_request(Request(env))
            req = Request(env)
            return self.handle_request(req)(env, start_response)
        except UnicodeError:
            err = HTTPPreconditionFailed(
                request=Request(env), body='Invalid UTF8 or contains NULL')
            return err(env, start_response)
        except (Exception, Timeout):
            start_response('500 Server Error',
                           [('Content-Type', 'text/plain')])
            return ['Internal server error.\n']
        finally:
            if (not req.method == "STOP_SERVICE") and (req in self.ongoing_operation_list):
                self.ongoing_operation_list.remove(req)

    #def update_request(self, req):
    #    if 'x-storage-token' in req.headers and \
    #            'x-auth-token' not in req.headers:
    #        req.headers['x-auth-token'] = req.headers['x-storage-token']
    #    return req

    def handle_request(self, req):
        """
        Entry point for proxy server.
        Should return a WSGI-style callable (such as swob.Response).

        :param req: swob.Request object
        """
        try:
            if 'swift.trans_id' not in req.environ:
                # if this wasn't set by an earlier middleware, set it now
                trans_id = generate_trans_id(self.trans_id_suffix)
                req.environ['swift.trans_id'] = trans_id
                self.logger.txn_id = trans_id
            self.logger.info('Proxy service received request for %(method)s '
                             '%(path)s with %(headers)s (txn: %(trans_id)s)',
                             {'method': req.method, 'path': req.path,
                             'headers': req.headers, 'trans_id': self.logger.txn_id})
            req.headers['x-trans-id'] = req.environ['swift.trans_id']
            self.logger.set_statsd_prefix('proxy-server')
            #if self.stop_service_flag:
            #    self.logger.info('Proxy is going to stop therefore no more request')
            #    return HTTPForbidden(request=req, body='Proxy is going to stop\
            #        therefore no more request')		#TODO would it be HTTPForbidden or something else 
            if req.content_length and req.content_length < 0:
                self.logger.increment('errors')
                return HTTPBadRequest(request=req,
                                      body='Invalid Content-Length')

            try:
                if not check_utf8(req.path_info):
                    self.logger.increment('errors')
                    return HTTPPreconditionFailed(
                        request=req, body='Invalid UTF8 or contains NULL')
            except UnicodeError:
                self.logger.increment('errors')
                return HTTPPreconditionFailed(
                    request=req, body='Invalid UTF8 or contains NULL')

            try:
                error_response = constraints.check_non_allowed_headers(req)
                if error_response:
                    return error_response

                obj_list = ''
                if ('bulk-delete' in req.params or 'X-Bulk-Delete' in \
                req.headers) and req.method in ['POST', 'DELETE']:
                    self.logger.debug("*** Bulk delete request ***")
                    cont, obj_list = get_objs_to_delete(req, \
                    self.max_bulk_delete_entries, self.logger)
                    self.logger.debug("Container: %s, Obj list: %s" \
                    % (cont, obj_list))
                    version, account, container = \
                    split_path(req.path, 1, 3, True)
                    if container:
                        container = container.strip('/')
                        if cont != container:
                            self.logger.error("Container in path is different")
                            return HTTPBadRequest(request=req, \
                            body='Container name mismatch')
                        req.path_info = req.path.rstrip('/')
                    else:
                        req.path_info = os.path.join(req.path, cont)
                    req.method = 'BULK_DELETE'
                    req.headers['Content-Length'] = len(str(obj_list))
                    req.body = str(obj_list)

                controller, path_parts = self.get_controller(req.path, req.method)
                p = req.path_info
                if isinstance(p, unicode):
                    p = p.encode('utf-8')
            except ValueError:
                self.logger.increment('errors')
                return HTTPNotFound(request=req)
            if not controller:
                self.logger.increment('errors')
                return HTTPPreconditionFailed(request=req, body='Bad URL')
            if self.deny_host_headers and \
                    req.host.split(':')[0] in self.deny_host_headers:
                return HTTPForbidden(request=req, body='Invalid host header')

            self.logger.set_statsd_prefix('proxy-server.' +
                                          controller.server_type.lower())
            controller = controller(self, **path_parts)
            controller.trans_id = req.environ['swift.trans_id']
            #self.logger.client_ip = get_remote_client(req)
            try:
                handler = getattr(controller, req.method)
                getattr(handler, 'publicly_accessible')
            except AttributeError:
                allowed_methods = getattr(controller, 'allowed_methods', set())
                return HTTPMethodNotAllowed(
                    request=req, headers={'Allow': ', '.join(allowed_methods)})
            if 'swift.authorize' in req.environ:
                # We call authorize before the handler, always. If authorized,
                # we remove the swift.authorize hook so isn't ever called
                # again. If not authorized, we return the denial unless the
                # controller's method indicates it'd like to gather more
                # information and try again later.
                resp = req.environ['swift.authorize'](req)
                if not resp:
                    # No resp means authorized, no delayed recheck required.
                    del req.environ['swift.authorize']
                else:
                    # Response indicates denial, but we might delay the denial
                    # and recheck later. If not delayed, return the error now.
                    if not getattr(handler, 'delay_denial', None):
                        return resp
            # Save off original request method (GET, POST, etc.) in case it
            # gets mutated during handling.  This way logging can display the
            # method the client actually sent.
            req.environ['swift.orig_req_method'] = req.method
            if not req.method == "STOP_SERVICE":
                self.ongoing_operation_list.append(req)
            resp = handler(req)
            self.logger.info('Proxy returning response %(status)s for '
                             '%(method)s %(path)s with %(headers)s '
                             '(txn: %(trans_id)s)',
                             {'status': resp.status, 'method': req.method,
                              'path': req.path, 'headers': resp.headers,
                              'trans_id': req.headers['x-trans-id']})
            return resp
        except HTTPException as error_response:
            return error_response
        except (Exception, Timeout):
            self.logger.exception(_('ERROR Unhandled exception in request'))
            return HTTPServerError(request=req)


    def check_request_status(self, request):
        '''
        Set stop_service parameter to True 
        :param http request
        return HTTPOk when all ongoing operation completed
        '''
        if self.ongoing_operation_list:
            self.logger.info("Ongoing requests are:%s", \
            self.ongoing_operation_list)
        return self.ongoing_operation_list

    #def sort_nodes(self, nodes):
    #    '''
    #    Sorts nodes in-place (and returns the sorted list) according to
    #    the configured strategy. The default "sorting" is to randomly
    #    shuffle the nodes. If the "timing" strategy is chosen, the nodes
    #    are sorted according to the stored timing data.
    #    '''
    #    # In the case of timing sorting, shuffling ensures that close timings
    #    # (ie within the rounding resolution) won't prefer one over another.
    #    # Python's sort is stable (http://wiki.python.org/moin/HowTo/Sorting/)
    #    shuffle(nodes)
    #    if self.sorting_method == 'timing':
    #        now = time()
 
    #        def key_func(node):
    #            timing, expires = self.node_timings.get(node['ip'], (-1.0, 0))
    #            return timing if expires > now else -1.0
    #        nodes.sort(key=key_func)
    #    elif self.sorting_method == 'affinity':
    #        nodes.sort(key=self.read_affinity_sort_key)
    #    return nodes

    def set_node_timing(self, node, timing):
        #if self.sorting_method != 'timing':
        #    return
        now = time.time()
        timing = round(timing, 3)  # sort timings to the millisecond
        self.node_timings[node['ip']] = (timing, now + self.timing_expiry)
    
    #def error_limited(self, node):
    #    """
    #    Check if the node is currently error limited.

    #    :param node: dictionary of node to check
    #    :returns: True if error limited, False otherwise
    #    """
    #    now = time()
    #    if 'errors' not in node:
    #        return False
    #    if 'last_error' in node and node['last_error'] < \
    #            now - self.error_suppression_interval:
    #        del node['last_error']
    #        if 'errors' in node:
    #            del node['errors']
    #        return False
    #    limited = node['errors'] > self.error_suppression_limit
    #    if limited:
    #        self.logger.debug(
    #            _('Node error limited %(ip)s:%(port)s (%(device)s)'), node)
    #    return limited

    #def error_limit(self, node, msg):
    #    """
    #    Mark a node as error limited. This immediately pretends the
    #    node received enough errors to trigger error suppression. Use
    #    this for errors like Insufficient Storage. For other errors
    #    use :func:`error_occurred`.

    #    :param node: dictionary of node to error limit
    #    :param msg: error message
    #    """
    #    node['errors'] = self.error_suppression_limit + 1
    #    node['last_error'] = time()
    #    self.logger.error(_('%(msg)s %(ip)s:%(port)s/%(device)s'),
    #                      {'msg': msg, 'ip': node['ip'],
    #                      'port': node['port'], 'device': node['device']})

    #def error_occurred(self, node, msg):
    #    """
    #    Handle logging, and handling of errors.

    #    :param node: dictionary of node to handle errors for
    #    :param msg: error message
    #    """
    #    node['errors'] = node.get('errors', 0) + 1
    #    node['last_error'] = time()
    #    self.logger.error(_('%(msg)s %(ip)s:%(port)s/%(device)s'),
    #                      {'msg': msg, 'ip': node['ip'],
    #                      'port': node['port'], 'device': node['device']})

    #def iter_nodes(self, ring, partition, node_iter=None):
    #    """
    #    Yields nodes for a ring partition, skipping over error
    #    limited nodes and stopping at the configurable number of
    #    nodes. If a node yielded subsequently gets error limited, an
    #    extra node will be yielded to take its place.

    #    Note that if you're going to iterate over this concurrently from
    #    multiple greenthreads, you'll want to use a
    #    osd.common.utils.GreenthreadSafeIterator to serialize access.
    #    Otherwise, you may get ValueErrors from concurrent access. (You also
    #    may not, depending on how logging is configured, the vagaries of
    #    socket IO and eventlet, and the phase of the moon.)

    #    :param ring: ring to get yield nodes from
    #    :param partition: ring partition to yield nodes for
    #    :param node_iter: optional iterable of nodes to try. Useful if you
    #        want to filter or reorder the nodes.
    #    """
    #    part_nodes = ring.get_part_nodes(partition)
    #    if node_iter is None:
    #        node_iter = itertools.chain(part_nodes,
    #                                    ring.get_more_nodes(partition))
    #    num_primary_nodes = len(part_nodes)

    #    # Use of list() here forcibly yanks the first N nodes (the primary
    #    # nodes) from node_iter, so the rest of its values are handoffs.
    #    primary_nodes = self.sort_nodes(
    #        list(itertools.islice(node_iter, num_primary_nodes)))
    #    handoff_nodes = node_iter
    #    nodes_left = self.request_node_count(len(primary_nodes))

    #    for node in primary_nodes:
    #        if not self.error_limited(node):
    #            yield node
    #            if not self.error_limited(node):
    #                nodes_left -= 1
    #                if nodes_left <= 0:
    #                    return
    #    handoffs = 0
    #    for node in handoff_nodes:
    #        if not self.error_limited(node):
    #            handoffs += 1
    #            if self.log_handoffs:
    #                self.logger.increment('handoff_count')
    #                self.logger.warning(
    #                    'Handoff requested (%d)' % handoffs)
    #                if handoffs == len(primary_nodes):
    #                    self.logger.increment('handoff_all_count')
    #            yield node
    #            if not self.error_limited(node):
    #                nodes_left -= 1
    #                if nodes_left <= 0:
    #                    return

    def exception_occurred(self, node, typ, additional_info):
        """
        Handle logging of generic exceptions.

        :param node: dictionary of node to log the error for
        :param typ: server type
        :param additional_info: additional information to log
        """
        self.logger.exception(
            _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
              '%(info)s'),
            {'type': typ, 'ip': node['ip'], 'port': node['port'],
             'info': additional_info})

    def modify_wsgi_pipeline(self, pipe):
        """
        Called during WSGI pipeline creation. Modifies the WSGI pipeline
        context to ensure that mandatory middleware is present in the pipeline.

        :param pipe: A PipelineWrapper object
        """
        pipeline_was_modified = False
        for filter_spec in reversed(required_filters):
            filter_name = filter_spec['name']
            if filter_name not in pipe:
                afters = filter_spec.get('after_fn', lambda _junk: [])(pipe)
                insert_at = 0
                for after in afters:
                    try:
                        insert_at = max(insert_at, pipe.index(after) + 1)
                    except ValueError:  # not in pipeline; ignore it
                        pass
                self.logger.info(
                    'Adding required filter %s to pipeline at position %d' %
                    (filter_name, insert_at))
                ctx = pipe.create_filter(filter_name)
                pipe.insert_filter(ctx, index=insert_at)
                pipeline_was_modified = True

        if pipeline_was_modified:
            self.logger.info("Pipeline was modified. New pipeline is \"%s\".",
                             pipe)
        else:
            self.logger.debug("Pipeline is \"%s\"", pipe)


def app_factory(global_conf, **local_conf):
    """paste.deploy app factory for creating WSGI proxy apps."""
    conf = global_conf.copy()
    conf.update(local_conf)
    app = Application(conf)
    return app
