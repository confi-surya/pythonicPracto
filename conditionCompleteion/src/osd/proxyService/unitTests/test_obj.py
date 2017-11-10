#!/usr/bin/env python
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

import unittest
from contextlib import contextmanager

import mock

import osd
from osd.proxyService import server as proxy_server
from osd.common.swob import HTTPException
from osd.proxyService.unitTests import FakeMemcache, fake_http_connect, debug_logger
from osd.objectService.unitTests import FakeObjectRing, FakeContainerRing, FakeAccountRing, FakeLogger
from osd.proxyService.controllers import obj
from osd.common.swob import HTTPForbidden, HTTPBadRequest, HTTPException, HTTPPreconditionFailed, HTTPOk
from osd.common.http import HTTP_INSUFFICIENT_STORAGE
from osd.common.exceptions import ChunkReadTimeout

@contextmanager
def set_http_connect(*args, **kwargs):
    old_connect = osd.proxyService.controllers.base.http_connect
    new_connect = fake_http_connect(*args, **kwargs)
    osd.proxyService.controllers.base.http_connect = new_connect
    osd.proxyService.controllers.obj.http_connect = new_connect
    osd.proxyService.controllers.account.http_connect = new_connect
    osd.proxyService.controllers.container.http_connect = new_connect
    yield new_connect
    osd.proxyService.controllers.base.http_connect = old_connect
    osd.proxyService.controllers.obj.http_connect = old_connect
    osd.proxyService.controllers.account.http_connect = old_connect
    osd.proxyService.controllers.container.http_connect = old_connect


class TestObjControllerWriteAffinity(unittest.TestCase):
    def setUp(self):
        self.app = proxy_server.Application(
            None, FakeMemcache(), account_ring=FakeAccountRing(''),
            container_ring=FakeContainerRing(''), object_ring=FakeObjectRing(''), logger=FakeLogger())

    def test_connect_put_node_timeout(self):
        controller = proxy_server.ObjectController(self.app, 'a', 'c', 'o')
        self.app.conn_timeout = 0.1
        with set_http_connect(200, slow_connect=True):
            nodes = [dict(ip='', port='', device='')]
            res = controller._connect_put_node(nodes, '', '', '', {}, ('', ''))
        self.assertTrue(res is None)


class TestObjController(unittest.TestCase):
    def setUp(self):
        logger = debug_logger('proxy-server')
        logger.thread_locals = ('txn1', '127.0.0.2')
        self.app = proxy_server.Application(
            None, FakeMemcache(), account_ring=FakeAccountRing(''),
            container_ring=FakeContainerRing(''), object_ring=FakeObjectRing(''),
            logger=logger)
        self.controller = proxy_server.ObjectController(self.app,
                                                        'a', 'c', 'o')
        self.controller.container_info = mock.MagicMock(return_value={
            'node': [
                {'ip': '127.0.0.1', 'port': '1'},
            ],
            'filesystem': 'fs0',
            'directory': 'd0',
            'sync_key': None,
            'write_acl': None,
            'versions': None,
            'read_acl': None})

    #def test_PUT_simple(self):
    #    req = osd.common.swob.Request.blank('/v1/a/c/o')
    #    req.headers['content-length'] = '0'
    #    with set_http_connect(201, 201, 201):
    #        resp = self.controller.PUT(req)
    #    self.assertEquals(resp.status_int, 201)

    #def test_PUT_if_none_match(self):
    #    req = osd.common.swob.Request.blank('/v1/a/c/o')
    #    req.headers['if-none-match'] = '*'
    #    req.headers['content-length'] = '0'
    #    with set_http_connect(201, 201, 201):
    #        resp = self.controller.PUT(req)
    #    self.assertEquals(resp.status_int, 201)


    def test_PUT_if_none_match_not_star(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        req.headers['if-none-match'] = 'somethingelse'
        req.headers['content-length'] = '0'
        with set_http_connect(201, 201, 201):
            resp = self.controller.PUT(req)
        self.assertEquals(resp.status_int, 400)

    def test_GET_simple(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with set_http_connect(200):
            resp = self.controller.GET(req)
        self.assertEquals(resp.status_int, 200)
    
    def test_GET_when_account_not_exist(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with set_http_connect(404, 404, 204):
            resp = self.controller.GET(req)
            self.assertEquals(resp.status_int, 404)

    def test_HEAD_when_account_not_exist(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with set_http_connect(404, 404, 204):
            resp = self.controller.HEAD(req)
            self.assertEquals(resp.status_int, 404)

    def test_DELETE_simple(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with set_http_connect(204, 204, 204):
            resp = self.controller.DELETE(req)
        self.assertEquals(resp.status_int, 204)

    def test_HEAD_simple(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with set_http_connect(200, 200, 200, 201, 201, 201):
            resp = self.controller.HEAD(req)
        self.assertEquals(resp.status_int, 200)

    def test_PUT_log_info(self):
        # mock out enough to get to the area of the code we want to test
        with mock.patch('osd.proxyService.controllers.obj.check_object_creation',
                        mock.MagicMock(return_value=None)):
            req = osd.common.swob.Request.blank('/v1/a/c/o')
            req.headers['x-copy-from'] = 'somewhere'
            try:
                self.controller.PUT(req)
            except HTTPException:
                pass
            self.assertEquals(
                req.environ.get('swift.log_info'), ['x-copy-from:somewhere'])
            # and then check that we don't do that for originating POSTs
            req = osd.common.swob.Request.blank('/v1/a/c/o')
            req.method = 'POST'
            req.headers['x-copy-from'] = 'elsewhere'
            try:
                self.controller.PUT(req)
            except HTTPException:
                pass
            self.assertEquals(req.environ.get('swift.log_info'), None)

    def test_check_content_type(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        self.assertEqual(obj.check_content_type(req), None)
        req = osd.common.swob.Request.blank('/v1/a/c/o', environ = {})
        req.headers['content-type'] = ';swift_*'
        self.assertTrue(obj.check_content_type(req) is not None)

    def test_GETorHEAD(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o', environ={'swift.authorize':mock.MagicMock(side_effect=HTTPForbidden)})
        resp = self.controller.GETorHEAD(req)
        self.assertEqual(resp.status_int, 403)

        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(404)):
            resp = self.controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 404)

        with mock.patch('osd.proxyService.controllers.base.Controller.container_info'):
            resp = self.controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 503)        
        
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200)):
            resp = self.controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 200)
       
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, headers={'content-type': 'text;swift_bytes=1'})):
            resp = self.controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 200)
            self.assertEqual(resp.headers['content-type'], 'text')

        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, headers={'content-type': 'json'})):
            resp = self.controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 200)
            self.assertEqual(resp.headers['content-type'], 'json')

    def test_POST(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with mock.patch('osd.proxyService.controllers.obj.check_metadata', return_value=HTTPBadRequest(request=req)):
            resp = self.controller.POST(req)
            self.assertEqual(resp.status_int, 400)

        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200)):
            resp = self.controller.POST(req)
            self.assertEqual(resp.status_int, 200)
 
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(503)):
            resp = self.controller.POST(req)
            self.assertEqual(resp.status_int, 503)
        
        self.controller.container_info = mock.MagicMock(return_value={'filesystem': None, 'directory': None, 'node': None, 'write_acl': None})
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        resp = self.controller.POST(req)
        self.assertEqual(resp.status_int, 404)      
        
        req = osd.common.swob.Request.blank('/v1/a/c/o', environ={'swift.authorize':mock.MagicMock(side_effect=HTTPForbidden)})
        resp = self.controller.POST(req)
        self.assertEqual(resp.status_int, 403)

    def test_DELETE(self):
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200)):
            resp = self.controller.DELETE(req)
            self.assertEqual(resp.status_int, 200)
 
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(503)):
            resp = self.controller.DELETE(req)
        req = osd.common.swob.Request.blank('/v1/a/c/o', environ={'swift.authorize':mock.MagicMock(side_effect=HTTPForbidden)})
        resp = self.controller.DELETE(req)
        self.assertEqual(resp.status_int, 403)

        self.controller.container_info = mock.MagicMock(return_value={'filesystem': None, 'directory': None, 'node': None, 'write_acl': None, 'sync_key': None, 'versions': None})
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        resp = self.controller.DELETE(req)
        self.assertEqual(resp.status_int, 404)

    def test_PUT(self):
        with mock.patch('osd.proxyService.controllers.obj.http_connect',
                        fake_http_connect(200)):
            req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': '0'})
            resp = self.controller.PUT(req)
            self.assertEqual(resp.status_int, 200)
        
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        req.if_none_match = 'test'
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 400)
    
        req = osd.common.swob.Request.blank('/v1/a/c/o', query_string="multipart-manifest=test")
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 400)
        self.assertEqual(resp.body, 'Unsupported query parameter multipart manifest')
        
        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200)):
            resp = self.controller.PUT(req)
            self.assertEqual(resp.status_int, 411)

        req = osd.common.swob.Request.blank('/v1/a/c/o', environ={'swift.authorize':mock.MagicMock(side_effect=HTTPForbidden)})
        resp = self.controller.DELETE(req)
        self.assertEqual(resp.status_int, 403)
        
        req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'transfer-encoding': 'test1, test2'})
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 501)

        req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'transfer-encoding': 'test'})
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 400)

        osd.proxyService.controllers.obj.MAX_FILE_SIZE = 5
        req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': 'test'})
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 400)

        req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': '6'})
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 413)

        req = osd.common.swob.Request.blank('/v1/a/c/o')
        with mock.patch('osd.proxyService.controllers.obj.check_object_creation', return_value=HTTPBadRequest(request=req)):
            resp = self.controller.PUT(req)
            self.assertEqual(resp.status_int, 400)

        #with mock.patch('osd.proxyService.controllers.obj.http_connect',
        #                fake_http_connect(200)):
        #    req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': '0'})
        #    obj.ObjectController._send_file = mock.MagicMock(side_effect=Exception)
        #    resp = self.controller.PUT(req)
        #    self.assertEqual(resp.status_int, 499)
        
        # with mock.patch('osd.proxyService.controllers.obj.http_connect',
        #                fake_http_connect(200)):
        #    with mock.patch('osd.proxyService.controllers.obj.ObjectController._send_file', side_effect=ChunkReadTimeout):
        #        req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': '0'})
        #        #    obj.ObjectController._send_file = mock.MagicMock(side_effect=ChunkReadTimeout)
        #        resp = self.controller.PUT(req)
        #        self.assertEqual(resp.status_int, 499)
        
        with mock.patch('osd.proxyService.controllers.obj.http_connect',
                        fake_http_connect(200)):
            req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': '0'})
            obj.ObjectController._get_put_responses = mock.MagicMock(return_value=([], [], [], ['etag1', 'etag2']))
            resp = self.controller.PUT(req)
            self.assertEqual(resp.status_int, 500)
        
        
        self.controller.container_info = mock.MagicMock(return_value={'filesystem': None, 'directory': None, 'node': None, 'write_acl': None, 'sync_key': None, 'versions': None})
        req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': '6'})
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 404)
 
        
    def test__backend_requests(self):
        req = osd.common.swob.Request.blank('/v1/a/c', {})
        headers = self.controller._backend_requests(req, 1, 'fs1', 'd1', [{'ip': '1', 'port': '1'}], 5.0, 0)
        self.assertTrue(headers[0]['X-Container-Filesystem'], 'fs1')
        self.assertTrue(headers[0]['X-Container-Directory'], 'd1')
        self.assertTrue(headers[0]['X-Container-Host'], '1:1')
        self.assertEqual(len(headers), 1)
        self.assertRaises(ZeroDivisionError, self.controller._backend_requests, req, 0, 'fs1', 'd1', [{'ip': '1', 'port': '1'}], 5.0, 0)

    def test_connect_put_node(self):
        with mock.patch('osd.proxyService.controllers.obj.http_connect',
                        fake_http_connect(200)):
            conn = self.controller._connect_put_node([{'ip': '1', 'port': '1'}], 'fs1', 'd1', \
                '/a/c/o', {'If-None-Match': None}, self.app.logger.thread_locals)
            self.assertEqual(conn.node, {'ip': '1', 'port': '1'})
            self.assertEqual(conn.resp, None)

        with mock.patch('osd.proxyService.controllers.obj.http_connect',
                        fake_http_connect((201, -4))):
            conn = self.controller._connect_put_node([{'ip': '1', 'port': '1'}], 'fs1', 'd1', \
                '/a/c/o', {'If-None-Match': None}, self.app.logger.thread_locals)
            self.assertEqual(conn.node, {'ip': '1', 'port': '1'})
            self.assertEqual(conn.resp.status, 201)

        with mock.patch('osd.proxyService.controllers.obj.http_connect',
                        fake_http_connect((200, 412))):
            self.controller._connect_put_node([{'ip': '1', 'port': '1'}], 'fs1', 'd1', '/a/c/o', {'If-None-Match': 'test'}, \
                self.app.logger.thread_locals)
            self.assertEqual(conn.node, {'ip': '1', 'port': '1'})
            self.assertEqual(conn.resp.status, 201)

    def test_ZeroDivisionError_in_backend_requests(self):
        req = osd.common.swob.Request.blank('/v1/a/c')
        self.controller._backend_requests = mock.MagicMock(side_effect=ZeroDivisionError)
        resp = self.controller.POST(req)
        self.assertEqual(resp.status_int, 500)

        resp = self.controller.DELETE(req)
        self.assertEqual(resp.status_int, 500)
        
        req = osd.common.swob.Request.blank('/v1/a/c/o', headers={'content-length': '0'})
        resp = self.controller.PUT(req)
        self.assertEqual(resp.status_int, 500)

    def test_get_node_returning_empty_value(self):
        req = osd.common.swob.Request.blank('/v1/a/c', {})
        self.app.object_ring.get_node = mock.MagicMock(return_value=([], '', '', '', ''))
        resp = self.controller.GET(req)
        self.assertEqual(resp.status_int, 500)
 
        resp = self.controller.HEAD(req)
        self.assertEqual(resp.status_int, 500)
        

        
if __name__ == '__main__':
    unittest.main()
