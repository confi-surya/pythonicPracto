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

from osd import gettext_ as _
from urllib import unquote
import time
from osd.common.utils import public, normalize_timestamp, loggit, \
    GreenthreadSafeIterator, GreenAsyncPile
from osd.common.constraints import check_metadata, MAX_CONTAINER_NAME_LENGTH, \
    REQ_BODY_CHUNK_SIZE
from osd.common.http import HTTP_ACCEPTED
from osd.proxyService.controllers.base import Controller, delay_denial, \
    clear_info_cache
from osd.common.swob import HTTPBadRequest, HTTPForbidden, \
    HTTPNotFound, HTTPPreconditionFailed, HTTPInternalServerError, \
    HTTPServiceUnavailable
from osd.common.request_helpers import get_param
from osd.common.bufferedhttp import http_connect
from osd.common.exceptions import ConnectionTimeout
from eventlet.timeout import Timeout
from osd.common.http import is_informational, is_server_error, \
    HTTP_INSUFFICIENT_STORAGE, HTTP_SERVICE_UNAVAILABLE

#pylint: disable=C0103

class ContainerController(Controller):
    """WSGI controller for container requests"""
    server_type = 'Container'

    # Ensure these are all lowercase
#    pass_through_headers = ['x-container-read', 'x-container-write',
#                            'x-container-sync-key', 'x-container-sync-to',
#                            'x-versions-location']
    pass_through_headers = ['x-container-read', 'x-container-write', 'accept']

    def __init__(self, app, account_name, container_name, **kwargs):
        Controller.__init__(self, app)
        self.account_name = unquote(account_name)
        self.container_name = unquote(container_name)

    def _x_remove_headers(self):
        st = self.server_type.lower()
        return ['x-remove-%s-read' % st,
                'x-remove-%s-write' % st,
                'x-remove-versions-location']

    def clean_acls(self, req):
        if 'swift.clean_acl' in req.environ:
            for header in ('x-container-read', 'x-container-write'):
                if header in req.headers:
                    try:
                        req.headers[header] = \
                            req.environ['swift.clean_acl'](header,
                                                           req.headers[header])
                    except ValueError as err:
                        return HTTPBadRequest(request=req, body=str(err))
        return None

    @public
    @loggit('proxy-server')
    def BULK_DELETE(self, req):
        """HTTP BULK_DELETE request handler."""
        self.app.logger.debug("In BULK_DELETE____")
        account_node, account_filesystem, account_directory, container_count, \
        account_component_number, head_status = self.account_info(\
                                               self.account_name, req)
        if not account_node and not head_status:
            return HTTPInternalServerError(request=req)
        if head_status and int(str(head_status).split()[0]) == 503:
            #TODO need to check why head_status is int or sometimes str
            self.app.logger.info("account HEAD returning 503 service " \
                "unavailable error due to which this request got failed")
            return HTTPServiceUnavailable(request=req)
        if not account_node:
            return HTTPNotFound(request=req)
        container_node, container_filesystem, container_directory, \
        global_map_version, component_number = \
            self.app.container_ring.get_node( \
            self.account_name, self.container_name)
        if not len(container_node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'BULK_DELETE', 'path': req.path})
            return HTTPInternalServerError(request=req)
        self.app.logger.debug("Going for backend call______")
        try:
            headers = self._backend_requests(req, len(container_node),
                account_node, account_filesystem, account_directory, \
                global_map_version, component_number, account_component_number)
        except ZeroDivisionError:
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'BULK_DELETE', 'path': req.path})
            return HTTPInternalServerError(request=req)

        clear_info_cache(self.app, req.environ,
                         self.account_name, self.container_name)
        self.app.logger.debug("Now going to make request_____")
        resp = self.make_request_for_bulk_delete(
            req, self.app.container_ring, container_node, container_filesystem,
            container_directory, 'BULK_DELETE',
            req.swift_entity_path, headers)
        return resp

    def _make_request_for_bulk_delete(self, nodes, filesystem, directory, \
        method, path, headers, query, body_data, logger_thread_locals):
        self.app.logger.thread_locals = logger_thread_locals
        self.app.logger.debug("in _make_request_for_bulk_delete___")
        for node in nodes:
            try:
                start_node_timing = time.time()
                headers['Content-Length'] = len(body_data)
                with ConnectionTimeout(self.app.conn_timeout):
                    conn = http_connect(node['ip'], node['port'],
                                        filesystem, directory, method, path,
                                        headers=headers, query_string=query)
                    if conn:
                        try:
                            bytes_transferred = 0
                            data_size = len(body_data)
                            while bytes_transferred < data_size:
                                chunk = body_data[:REQ_BODY_CHUNK_SIZE]
                                self.app.logger.debug("Sending chunk: %s" \
                                % chunk)
                                conn.send(chunk)
                                bytes_transferred += len(chunk)
                                body_data = body_data[REQ_BODY_CHUNK_SIZE:]
                            self.app.logger.debug("Total sent bytes: %s" \
                            % bytes_transferred)
                        except Exception, ex:
                            self.app.logger.error(\
                            "Error while sending body: %s" % ex)
                    conn.node = node
                self.app.set_node_timing(node, time.time() - start_node_timing)
                with Timeout(self.app.node_timeout):
                    resp = conn.getresponse()
                    if not is_informational(resp.status) and \
                            not is_server_error(resp.status):
                        return resp.status, resp.reason, resp.getheaders(), \
                            resp.read()
                    elif resp.status == HTTP_INSUFFICIENT_STORAGE:
                        self.app.logger.error(_('%(msg)s %(ip)s:%(port)s'),
                            {'msg': _('ERROR Insufficient Storage'), \
                            'ip': node['ip'], 'port': node['port']})
            except (Exception, Timeout):
                self.app.exception_occurred(
                    node, self.server_type,
                    _('Trying to %(method)s %(path)s') %
                    {'method': method, 'path': path})

    def make_request_for_bulk_delete(self, req, ring, node, filesystem, \
        directory, method, path, headers, query_string=''):
        self.app.logger.debug("In make_request_for_bulk_delete")
        nodes = GreenthreadSafeIterator(node)
        pile = GreenAsyncPile(len(node))
        for head in headers:
            pile.spawn(self._make_request_for_bulk_delete, nodes, filesystem, \
            directory, method, path, head, query_string, req.body, \
            self.app.logger.thread_locals)
        response = []
        statuses = []
        for resp in pile:
            if not resp:
                continue
            response.append(resp)
            statuses.append(resp[0])
            if self.have_quorum(statuses, len(node)):
                break
        pile.waitall(self.app.post_quorum_timeout)
        while len(response) < len(node):
            response.append((HTTP_SERVICE_UNAVAILABLE, '', '', ''))
        statuses, reasons, resp_headers, bodies = zip(*response)
        return self.best_response(req, statuses, reasons, bodies,
                                  '%s %s' % (self.server_type, req.method),
                                  headers=resp_headers)


    def GETorHEAD(self, req):
        """Handler for HTTP GET/HEAD requests."""
        account_node, account_filesystem, account_directory, container_count, \
             account_component_number, head_status = self.account_info(\
                                               self.account_name, req)
        if not account_node and not head_status:
            return HTTPInternalServerError(request=req)
        if head_status and int(str(head_status).split()[0]) == 503:
            #TODO need to check why head_status is int or sometimes str
            self.app.logger.info("account HEAD returning 503 service " \
                "unavailable error due to which this request got failed")
            return HTTPServiceUnavailable(request=req)
        if head_status and int(str(head_status).split()[0]) == 500:
            return HTTPInternalServerError(request=req)
        if not account_node:
            return HTTPNotFound(request=req)
        path = get_param(req, 'path')
        #delimiter = get_param(req, 'delimiter')
        #prefix = get_param(req, 'prefix')
        limit = get_param(req, 'limit')
        if limit and not limit.isdigit():
            return HTTPPreconditionFailed(request=req,
                                          body='Value of limit must '
                                               'be a positive integer')
        if path:
            return HTTPBadRequest(request=req, body='Unsupported query parameter '
                                                    'path')
#        part = self.app.container_ring.get_part(
#            self.account_name, self.container_name)
        node, filesystem, directory, global_map_version, component_number = \
            self.app.container_ring.get_node(self.account_name, \
            self.container_name)
        if not len(node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'GET/HEAD', 'path': req.path})
            return HTTPInternalServerError(request=req)
        #TODO Currently same component request is blocked until previous same 
        #component request's map version is not updated, need to check other provision
        if not self.is_req_blocked(_('Container'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #    'movement is in progress') 
#        resp = self.GETorHEAD_base(
#            req, _('Container'), self.app.container_ring, part,
#            req.swift_entity_path)
            resp = self.GETorHEAD_base(req, _('Container'), \
                self.app.container_ring, node, filesystem, directory, \
                req.swift_entity_path, global_map_version, component_number)
            resp = self.container_req_for_comp_distribution(req, \
                component_number, global_map_version, self.account_name, \
                self.container_name, resp)
            if int(resp.status.split()[0]) in (301, 307):
                resp = HTTPInternalServerError(request=req)
            if 'swift.authorize' in req.environ:
                req.acl = resp.headers.get('x-container-read')
                aresp = req.environ['swift.authorize'](req)
                if aresp:
                    return aresp
            if not req.environ.get('swift_owner', False):
                for key in self.app.swift_owner_headers:
                    if key in resp.headers:
                        del resp.headers[key]
            return resp

    @public
    @delay_denial
#    @cors_validation
    @loggit('proxy-server')
    def GET(self, req):
        """Handler for HTTP GET requests."""
        return self.GETorHEAD(req)

    @public
    @delay_denial
#    @cors_validation
    @loggit('proxy-server')
    def HEAD(self, req):
        """Handler for HTTP HEAD requests."""
        return self.GETorHEAD(req)

    @public
#    @cors_validation
    @loggit('proxy-server')
    def PUT(self, req):
        """HTTP PUT request handler."""
        error_response = \
            self.clean_acls(req) or check_metadata(req, 'container')
        if error_response:
            return error_response
        if not req.environ.get('swift_owner'):
            for key in self.app.swift_owner_headers:
                req.headers.pop(key, None)
        if len(self.container_name) > MAX_CONTAINER_NAME_LENGTH:
            resp = HTTPBadRequest(request=req)
            resp.body = 'Container name length of %d longer than %d' % \
                        (len(self.container_name), MAX_CONTAINER_NAME_LENGTH)
            return resp
#        account_partition, accounts, container_count = \
#            self.account_info(self.account_name, req)
        account_node, account_filesystem, account_directory, container_count, \
             account_component_number, head_status = self.account_info(\
                                               self.account_name, req)
#        if not accounts and self.app.account_autocreate:
#            self.autocreate_account(req.environ, self.account_name)
#            account_partition, accounts, container_count = \
#                self.account_info(self.account_name, req)
#        if not accounts:
        if not account_node and not head_status:
            return HTTPInternalServerError(request=req)
        if head_status and int(str(head_status).split()[0]) == 503:
            #TODO need to check why head_status is int or sometimes str
            self.app.logger.info("account HEAD returning 503 service " \
                "unavailable error due to which this request got failed")
            return HTTPServiceUnavailable(request=req)
        if head_status and int(str(head_status).split()[0]) == 500:
            return HTTPInternalServerError(request=req)

        if not account_node and self.app.account_autocreate:
            self.autocreate_account(req.environ, self.account_name)
            account_node, account_filesystem, account_directory, \
            container_count, account_component_number, head_status = \
            self.account_info(self.account_name, req)
            if not account_node and not head_status:
                return HTTPInternalServerError(request=req)
            if head_status and int(str(head_status).split()[0]) == 503:
                self.app.logger.info("account HEAD returning 503 service" \
                    "unavailable error due to which this request got failed")
                return HTTPServiceUnavailable(request=req)
            if head_status and int(str(head_status).split()[0]) == 500:
                return HTTPInternalServerError(request=req)

        if not account_node:
            return HTTPNotFound(request=req)
        if self.app.max_containers_per_account > 0 and \
                container_count >= self.app.max_containers_per_account and \
                self.account_name not in self.app.max_containers_whitelist:
            resp = HTTPForbidden(request=req)
            resp.body = 'Reached container limit of %s' % \
                        self.app.max_containers_per_account
            return resp
#        container_partition, containers = self.app.container_ring.get_nodes(
#            self.account_name, self.container_name)
        container_node, container_filesystem, container_directory, \
            global_map_version, component_number = \
            self.app.container_ring.get_node(self.account_name,
                                             self.container_name)
        if not len(container_node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'PUT', 'path': req.path})
            return HTTPInternalServerError(request=req)
#        headers = self._backend_requests(req, len(containers),
#                                         account_partition, accounts)
        try:
            headers = self._backend_requests(req, len(container_node),
                                             account_node, account_filesystem,
                                             account_directory,
                                             global_map_version,
                                             component_number, 
                                             account_component_number)
        except ZeroDivisionError:
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'PUT', 'path': req.path})
            return HTTPInternalServerError(request=req)

        clear_info_cache(self.app, req.environ,
                         self.account_name, self.container_name)
        if not self.is_req_blocked(_('Container'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #    'movement is in progress')
#        resp = self.make_requests(
#
#
#            container_partition, 'PUT', req.swift_entity_path, headers)
            resp = self.make_requests(
                req, self.app.container_ring,
                container_node, container_filesystem, container_directory,
                'PUT', req.swift_entity_path, headers)
            resp = self.container_req_for_comp_distribution(req, \
                component_number, global_map_version, self.account_name, \
                self.container_name, resp)
            if int(resp.status.split()[0]) in (301, 307):
                resp = HTTPInternalServerError(request=req)
            return resp

    @public
#    @cors_validation
    @loggit('proxy-server')
    def POST(self, req):
        """HTTP POST request handler."""
        error_response = \
            self.clean_acls(req) or check_metadata(req, 'container')
        if error_response:
            return error_response
        if not req.environ.get('swift_owner'):
            for key in self.app.swift_owner_headers:
                req.headers.pop(key, None)
#        account_partition, accounts, container_count = \
#            self.account_info(self.account_name, req)
#        if not accounts:
        account_node, account_filesystem, account_directory, container_count, \
            account_component_number, head_status = self.account_info(\
            self.account_name, req)
        if not account_node and not head_status:
            return HTTPInternalServerError(request=req)
        if head_status and int(str(head_status).split()[0]) == 503:
            self.app.logger.info("account HEAD returning 503 service" \
                "unavailable error due to which this request got failed")
            return HTTPServiceUnavailable(request=req)
        if head_status and int(str(head_status).split()[0]) == 500:
            return HTTPInternalServerError(request=req)
        if not account_node:
            return HTTPNotFound(request=req)
#        container_partition, containers = self.app.container_ring.get_nodes(
#
#            self.account_name, self.container_name)
        container_node, container_filesystem, container_directory, \
            global_map_version, component_number = \
            self.app.container_ring.get_node( \
            self.account_name, self.container_name)
        if not len(container_node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'POST', 'path': req.path})
            return HTTPInternalServerError(request=req)
        headers = self.generate_request_headers(global_map_version,
                                component_number, req, transfer=True)
        clear_info_cache(self.app, req.environ,
                         self.account_name, self.container_name)
        if not self.is_req_blocked(_('Container'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #    'movement is in progress')
#        resp = self.make_requests(
#            req, self.app.container_ring, container_partition, 'POST',
#
#            req.swift_entity_path, [headers] * len(containers))
            resp = self.make_requests(
                req, self.app.container_ring, container_node,
                container_filesystem, container_directory, 'POST',
                req.swift_entity_path, [headers] * len(container_node))
            resp = self.container_req_for_comp_distribution(req, \
                component_number, global_map_version, self.account_name, \
                self.container_name, resp)
            if int(resp.status.split()[0]) in (301, 307):
                resp = HTTPInternalServerError(request=req)
            return resp

    @public
#    @cors_validation
    @loggit('proxy-server')
    def DELETE(self, req):
        """HTTP DELETE request handler."""
#        account_partition, accounts, container_count = \
#            self.account_info(self.account_name, req)
#        if not accounts:
        account_node, account_filesystem, account_directory, container_count, \
            account_component_number, head_status = self.account_info(\
            self.account_name, req)
        if not account_node and not head_status:
            return HTTPInternalServerError(request=req)
        if head_status and int(str(head_status).split()[0]) == 503:
            self.app.logger.info("account HEAD returning 503 service" \
                "unavailable error due to which this request got failed")
            return HTTPServiceUnavailable(request=req)
        if head_status and int(str(head_status).split()[0]) == 500:
            return HTTPInternalServerError(request=req)
        if not account_node:
            return HTTPNotFound(request=req)
#        container_partition, containers = self.app.container_ring.get_nodes(
#
#            self.account_name, self.container_name)
        container_node, container_filesystem, container_directory, \
            global_map_version, component_number = \
            self.app.container_ring.get_node( \
            self.account_name, self.container_name)
        if not len(container_node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'DELETE', 'path': req.path})
            return HTTPInternalServerError(request=req)
#        headers = self._backend_requests(req, len(containers),
#                                         account_partition, accounts)
        try:
            headers = self._backend_requests(req, len(container_node),
                account_node, account_filesystem, account_directory,
                global_map_version, component_number, account_component_number)
        except ZeroDivisionError:
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                {'msg': _('ERROR Wrong ring file content'),
                'method': 'DELETE', 'path': req.path})
            return HTTPInternalServerError(request=req)

        clear_info_cache(self.app, req.environ,
                         self.account_name, self.container_name)
#        resp = self.make_requests(
#            req, self.app.container_ring, container_partition, 'DELETE',
#
#            req.swift_entity_path, headers)
        if not self.is_req_blocked(_('Container'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #    'movement is in progress')
            resp = self.make_requests(
                req, self.app.container_ring, container_node, container_filesystem,
                container_directory, 'DELETE',
                req.swift_entity_path, headers)
            resp = self.container_req_for_comp_distribution(req, \
                component_number, global_map_version, self.account_name, \
                self.container_name, resp)
            if int(resp.status.split()[0]) in (301, 307):
                resp = HTTPInternalServerError(request=req)
            # Indicates no server had the container
            if resp.status_int == HTTP_ACCEPTED:
                return HTTPNotFound(request=req)
            return resp

#    def _backend_requests(self, req, n_outgoing,
#                          account_partition, accounts):
    def _backend_requests(self, req, n_outgoing,
                          account_node, account_filesystem, account_directory,
                          global_map_version, component_number, 
                          account_component_number):
        additional = {'X-Timestamp': normalize_timestamp(time.time())}
        headers = [self.generate_request_headers(global_map_version,
                   component_number, req, transfer=True, additional=additional)
                   for _junk in range(n_outgoing)]

#        for i, account in enumerate(accounts):
        for i, account in enumerate(account_node):
            try:
                i = i % len(headers)
            except ZeroDivisionError:
                raise

#            headers[i]['X-Account-Partition'] = account_partition
#            headers[i]['X-Account-Host'] = csv_append(
#                headers[i].get('X-Account-Host'),
#                '%(ip)s:%(port)s' % account)
#            headers[i]['X-Account-Device'] = csv_append(
#                headers[i].get('X-Account-Device'),
#                account['device'])
            headers[i]['X-Account-FileSystem'] = account_filesystem
            headers[i]['X-Account-Directory'] = account_directory
            headers[i]['X-Account-Host'] = '%(ip)s:%(port)s' % account
            headers[i]['X-Account-Component-Number'] = account_component_number
        return headers
