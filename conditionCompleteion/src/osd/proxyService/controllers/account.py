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

from osd.accountService.utils import account_listing_response
from osd.common.request_helpers import get_listing_content_type
from osd.common.middleware.acl import parse_acl, format_acl
from osd.common.utils import public, loggit
from osd.common.constraints import check_metadata, MAX_ACCOUNT_NAME_LENGTH
from osd.common.http import HTTP_NOT_FOUND, HTTP_GONE
from osd.proxyService.controllers.base import Controller, clear_info_cache
from osd.common.swob import HTTPBadRequest, HTTPMethodNotAllowed, \
    HTTPPreconditionFailed, HTTPInternalServerError, HTTPTemporaryRedirect, \
    HTTPMovedPermanently
from osd.common.request_helpers import get_sys_meta_prefix, get_param


class AccountController(Controller):
    """WSGI controller for account requests"""
    server_type = 'Account'

    def __init__(self, app, account_name, **kwargs):
        Controller.__init__(self, app)
        self.account_name = unquote(account_name)
        if not self.app.allow_account_management:
            self.allowed_methods.remove('PUT')
            self.allowed_methods.remove('DELETE')

    def add_acls_from_sys_metadata(self, resp):
        if resp.environ['REQUEST_METHOD'] in ('HEAD', 'GET', 'PUT', 'POST'):
            prefix = get_sys_meta_prefix('account') + 'core-'
            name = 'access-control'
            (extname, intname) = ('x-account-' + name, prefix + name)
            acl_dict = parse_acl(version=2, data=resp.headers.pop(intname))
            if acl_dict:  # treat empty dict as empty header
                resp.headers[extname] = format_acl(
                    version=2, acl_dict=acl_dict)

    def GETorHEAD(self, req):
        """Handler for HTTP GET/HEAD requests."""
        if len(self.account_name) > MAX_ACCOUNT_NAME_LENGTH:
            resp = HTTPBadRequest(request=req)
            resp.body = 'Account name length of %d longer than %d' % \
                        (len(self.account_name), MAX_ACCOUNT_NAME_LENGTH)
            return resp
        path = get_param(req, 'path')
        #delimiter = get_param(req, 'delimiter')
        #prefix = get_param(req, 'prefix')
        limit = get_param(req, 'limit')
        if limit and not limit.isdigit():
            return HTTPPreconditionFailed(request=req,
                                          body='Value of limit must '
                                               'be a positive integer')
        if path:
            return HTTPBadRequest(request=req, body='Unsupported query '
                                  'parameter path')

#        partition, nodes = self.app.account_ring.get_nodes(self.account_name)
        node, filesystem, directory, global_map_version, component_number = \
                             self.app.account_ring.get_node(self.account_name)
        if not len(node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                              {'msg': _('ERROR Wrong ring file content'),
                              'method': 'GET/HEAD', 'path': req.path})
            return HTTPInternalServerError(request=req)
	#TODO Currently same component request is blocked until previous same 
        #component request's map version is not updated, need to check other provision
        if not self.is_req_blocked(_('Account'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #   'movement is in progress')
#        resp = self.GETorHEAD_base(
#            req, _('Account'), self.app.account_ring, partition,
#            req.swift_entity_path.rstrip('/'))
            resp = self.GETorHEAD_base(
                req, _('Account'), self.app.account_ring, node, filesystem, 
                directory, req.swift_entity_path.rstrip('/'), 
                global_map_version, component_number)
            resp = self.account_req_for_comp_distribution(req, component_number,\
                global_map_version, self.account_name, resp)
            if int(resp.status.split()[0]) in (301, 307):
                resp = HTTPInternalServerError(request=req)
            if resp.status_int == HTTP_NOT_FOUND:
                if resp.headers.get('X-Account-Status', '').lower() == 'deleted':
                    resp.status = HTTP_GONE
                if self.app.account_autocreate:
                    resp = account_listing_response(self.account_name, req,
                                                get_listing_content_type(req))
            if req.environ.get('swift_owner'):
                self.add_acls_from_sys_metadata(resp)
            else:
                for header in self.app.swift_owner_headers:
                    resp.headers.pop(header, None)
            return resp
    
    @public
    @loggit('proxy-server')
    def PUT(self, req):
        """HTTP PUT request handler."""
        if not self.app.allow_account_management:
            return HTTPMethodNotAllowed(
                request=req,
                headers={'Allow': ', '.join(self.allowed_methods)})
        error_response = check_metadata(req, 'account')
        if error_response:
            return error_response
        if len(self.account_name) > MAX_ACCOUNT_NAME_LENGTH:
            resp = HTTPBadRequest(request=req)
            resp.body = 'Account name length of %d longer than %d' % \
                        (len(self.account_name), MAX_ACCOUNT_NAME_LENGTH)
            return resp
#        account_partition, accounts = \
#            self.app.account_ring.get_nodes(self.account_name)
        node, filesystem, directory, global_map_version, component_number = \
            self.app.account_ring.get_node(self.account_name)
        if not len(node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                              {'msg': _('ERROR Wrong ring file content'),
                              'method': 'PUT', 'path': req.path})
            return HTTPInternalServerError(request=req)
        headers = self.generate_request_headers(global_map_version,
                                component_number, req, transfer=True)
        clear_info_cache(self.app, req.environ, self.account_name)
        if not self.is_req_blocked(_('Account'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #    'movement is in progress')
#        resp = self.make_requests(
#            req, self.app.account_ring, account_partition, 'PUT',
#            req.swift_entity_path, [headers] * len(accounts))
            resp = self.make_requests(
                req, self.app.account_ring, node, filesystem, directory, 'PUT',
                req.swift_entity_path, [headers] * len(node))
            resp = self.account_req_for_comp_distribution(req, component_number,\
                global_map_version, self.account_name, resp)
            if int(resp.status.split()[0]) in (301, 307):
                resp = HTTPInternalServerError(request=req)
            self.add_acls_from_sys_metadata(resp)
            return resp

    @public
    @loggit('proxy-server')
    def POST(self, req):
        """HTTP POST request handler."""
        if len(self.account_name) > MAX_ACCOUNT_NAME_LENGTH:
            resp = HTTPBadRequest(request=req)
            resp.body = 'Account name length of %d longer than %d' % \
                        (len(self.account_name), MAX_ACCOUNT_NAME_LENGTH)
            return resp
        error_response = check_metadata(req, 'account')
        if error_response:
            return error_response
#        account_partition, accounts = \
#            self.app.account_ring.get_nodes(self.account_name)
        node, filesystem, directory, global_map_version, component_number = \
            self.app.account_ring.get_node(self.account_name)
        if not len(node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                              {'msg': _('ERROR Wrong ring file content'),
                              'method': 'POST', 'path': req.path})
            return HTTPInternalServerError(request=req)
        headers = self.generate_request_headers(global_map_version,
                                component_number, req, transfer=True)
        clear_info_cache(self.app, req.environ, self.account_name)
        if not self.is_req_blocked(_('Account'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #       'movement is in progress')
#        resp = self.make_requests(
#            req, self.app.account_ring, account_partition, 'POST',
#            req.swift_entity_path, [headers] * len(accounts))
            resp = self.make_requests(
                req, self.app.account_ring, node, filesystem, directory, 'POST',
                req.swift_entity_path, [headers] * len(node))
            resp = self.account_req_for_comp_distribution(req, component_number,\
                global_map_version, self.account_name, resp)
            if resp.status_int == HTTP_NOT_FOUND and self.app.account_autocreate:
                self.autocreate_account(req.environ, self.account_name)
#           resp = self.make_requests(
#               req, self.app.account_ring, account_partition, 'POST',
#               req.swift_entity_path, [headers] * len(accounts))
                resp = self.make_requests(
                    req, self.app.account_ring, node, filesystem, directory, 
                    'POST', req.swift_entity_path, [headers] * len(node))
                resp = self.account_req_for_comp_distribution(req, component_number,\
                global_map_version, self.account_name, resp)
                if int(resp.status.split()[0]) in (301, 307):
                    resp = HTTPInternalServerError(request=req)
            self.add_acls_from_sys_metadata(resp)
            return resp

    @public
    @loggit('proxy-server')
    def DELETE(self, req):
        """HTTP DELETE request handler."""
        # Extra safety in case someone typos a query string for an
        # account-level DELETE request that was really meant to be caught by
        # some middleware.
        if req.query_string:
            return HTTPBadRequest(request=req)
        if not self.app.allow_account_management:
            return HTTPMethodNotAllowed(
                request=req,
                headers={'Allow': ', '.join(self.allowed_methods)})
#        account_partition, accounts = \
#            self.app.account_ring.get_nodes(self.account_name)
        node, filesystem, directory, global_map_version, component_number = \
            self.app.account_ring.get_node(self.account_name)
        if not len(node):
            self.app.logger.error(_('%(msg)s %(method)s %(path)s'),
                              {'msg': _('ERROR Wrong ring file content'),
                              'method': 'DELETE', 'path': req.path})
            return HTTPInternalServerError(request=req)
        headers = self.generate_request_headers(global_map_version,
                                component_number, req)
        clear_info_cache(self.app, req.environ, self.account_name)
        if not self.is_req_blocked(_('Account'), component_number):
            #return HTTPTemporaryRedirect(request=req, body = 'Component'
            #       'movement is in progress')
#        resp = self.make_requests(
#            req, self.app.account_ring, account_partition, 'DELETE',
#            req.swift_entity_path, [headers] * len(accounts))
            resp = self.make_requests(
                req, self.app.account_ring, node, filesystem, directory, 
                'DELETE', req.swift_entity_path, [headers] * len(node))
            resp = self.account_req_for_comp_distribution(req, component_number,\
                global_map_version, self.account_name, resp)
            if int(resp.status.split()[0]) in (301, 307):
                resp = HTTPInternalServerError(request=req)
            return resp
