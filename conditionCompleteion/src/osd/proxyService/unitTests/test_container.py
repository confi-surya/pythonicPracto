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

import mock
import unittest

from osd.common.swob import Request
from osd.proxyService import server as proxy_server
from osd.proxyService.controllers.base import headers_to_container_info, Controller
from osd.objectService.unitTests import FakeObjectRing, FakeContainerRing, FakeAccountRing, FakeLogger
from osd.proxyService.unitTests import fake_http_connect, FakeMemcache
from osd.common.request_helpers import get_sys_meta_prefix
from osd.common.swob import HTTPNotFound, HTTPBadRequest, HTTPException
from osd.proxyService.controllers.base import Controller

class TestContainerController(unittest.TestCase):
    def setUp(self):
        self.app = proxy_server.Application(None, FakeMemcache(),
                                            account_ring=FakeAccountRing(''),
                                            container_ring=FakeContainerRing(''),
                                            object_ring=FakeObjectRing(''),
                                            logger=FakeLogger())

    def test_container_info_in_response_env(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, 200, body='')):
            req = Request.blank('/v1/a/c', {'PATH_INFO': '/v1/a/c'})
            resp = controller.HEAD(req)
        self.assertEqual(2, resp.status_int // 100)
        self.assertTrue("swift.container/a/c" in resp.environ)
        self.assertEqual(headers_to_container_info(resp.headers),
                         resp.environ['swift.container/a/c'])

    def test_swift_owner(self):
        owner_headers = {
            'x-container-read': 'value', 'x-container-write': 'value',
            'x-container-sync-key': 'value', 'x-container-sync-to': 'value'}
        controller = proxy_server.ContainerController(self.app, 'a', 'c')

        req = Request.blank('/v1/a/c')
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, 200, headers=owner_headers)):
            resp = controller.HEAD(req)
        self.assertEquals(2, resp.status_int // 100)
        for key in owner_headers:
            self.assertTrue(key not in resp.headers)

        req = Request.blank('/v1/a/c', environ={'swift_owner': True})
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, 200, headers=owner_headers)):
            resp = controller.HEAD(req)
        self.assertEquals(2, resp.status_int // 100)
        for key in owner_headers:
            self.assertTrue(key in resp.headers)

    def _make_callback_func(self, context):
        def callback(ipaddr, port, device, partition, method, path,
                     headers=None, query_string=None, ssl=False):
            context['method'] = method
            context['path'] = path
            context['headers'] = headers or {}
        return callback

    def test_sys_meta_headers_PUT(self):
        # check that headers in sys meta namespace make it through
        # the container controller
        sys_meta_key = '%stest' % get_sys_meta_prefix('container')
        sys_meta_key = sys_meta_key.title()
        user_meta_key = 'X-Container-Meta-Test'
        controller = proxy_server.ContainerController(self.app, 'a', 'c')

        context = {}
        callback = self._make_callback_func(context)
        hdrs_in = {sys_meta_key: 'foo',
                   user_meta_key: 'bar',
                   'x-timestamp': '1.0'}
        req = Request.blank('/v1/a/c', headers=hdrs_in)
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, 200, give_connect=callback)):
            controller.PUT(req)
        self.assertEqual(context['method'], 'PUT')
        self.assertTrue(sys_meta_key in context['headers'])
        self.assertEqual(context['headers'][sys_meta_key], 'foo')
        self.assertTrue(user_meta_key in context['headers'])
        self.assertEqual(context['headers'][user_meta_key], 'bar')
        self.assertNotEqual(context['headers']['x-timestamp'], '1.0')

    def test_sys_meta_headers_POST(self):
        # check that headers in sys meta namespace make it through
        # the container controller
        sys_meta_key = '%stest' % get_sys_meta_prefix('container')
        sys_meta_key = sys_meta_key.title()
        user_meta_key = 'X-Container-Meta-Test'
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        context = {}
        callback = self._make_callback_func(context)
        hdrs_in = {sys_meta_key: 'foo',
                   user_meta_key: 'bar',
                   'x-timestamp': '1.0'}
        req = Request.blank('/v1/a/c', headers=hdrs_in)
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, 200, give_connect=callback)):
            controller.POST(req)
        self.assertEqual(context['method'], 'POST')
        self.assertTrue(sys_meta_key in context['headers'])
        self.assertEqual(context['headers'][sys_meta_key], 'foo')
        self.assertTrue(user_meta_key in context['headers'])
        self.assertEqual(context['headers'][user_meta_key], 'bar')
        self.assertNotEqual(context['headers']['x-timestamp'], '1.0')

    def test_x_remove_headers(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        self.assertEqual(len(controller._x_remove_headers()), 3)

    def test_clean_acls(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        req = Request.blank('/v1/a/c', environ={'swift.clean_acl': 'test'})
        self.assertEqual(controller.clean_acls(req), None)
        req = Request.blank('/v1/a/c', environ={'swift.clean_acl': mock.MagicMock()}, headers={'x-container-read':'', 'x-container-write':''})
        self.assertEqual(controller.clean_acls(req), None)
        environ = {'swift.clean_acl': mock.MagicMock(side_effect=ValueError)}
        req = Request.blank('/v1/a/c', environ, headers={'x-container-read':'', 'x-container-write':''})
        self.assertTrue(controller.clean_acls(req) is not None)

    def test_GETorHEAD(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        req = Request.blank('/v1/a/c', {})
        with mock.patch('osd.proxyService.controllers.base.http_connect', fake_http_connect(200)):
            with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, 'test')):
                resp = controller.GETorHEAD(req)
                self.assertEqual(resp.status_int, 200)
        with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, None)):
            req = Request.blank('/v1/a/c', {})
            resp = controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 404)

        with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, 'test')):
            controller = proxy_server.ContainerController(self.app, 'a', 'c')
            req = Request.blank('/v1/a/c', {}, query_string='path=2')
            resp = controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 400)
            """
            self.assertEqual(resp.body, 'Unsupported query parameters path or delimiter or prefix')
        
            req = Request.blank('/v1/AUTH_bob', {}, query_string='prefix=a')
            resp = controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 400)
            self.assertEqual(resp.body, 'Unsupported query parameters path or delimiter or prefix')
    
            req = Request.blank('/v1/AUTH_bob', {}, query_string='delimiter=a')
            resp = controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 400)
            self.assertEqual(resp.body, 'Unsupported query parameters path or delimiter or prefix')
            """

        with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, 'test')):
            controller = proxy_server.ContainerController(self.app, 'a', 'c')
            req = Request.blank('/v1/a/c', {}, query_string='limit=-1')
            resp = controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 412)
            self.assertEqual(resp.body, 'Value of limit must be a positive integer')

        with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, 'test')):
            controller = proxy_server.ContainerController(self.app, 'a', 'c')
            req = Request.blank('/v1/a/c', {}, query_string='limit=abc')
            resp = controller.GETorHEAD(req)
            self.assertEqual(resp.status_int, 412)
            self.assertEqual(resp.body, 'Value of limit must be a positive integer')


    def test_PUT(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        req = Request.blank('/v1/a/c', {})
        with mock.patch('osd.proxyService.controllers.container.check_metadata', return_value=HTTPBadRequest(request=req)):
            resp = controller.PUT(req)
            self.assertEqual(resp.status_int, 400)
        
        controller = proxy_server.ContainerController(self.app, 'a', 'container'*40)
        resp = controller.PUT(req)
        self.assertEqual(resp.status_int, 400)

        with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, None, None, None)):
            controller = proxy_server.ContainerController(self.app, 'a', 'c')
            self.app.account_autocreate = False
            Controller.autocreate_account =  mock.MagicMock()
            req = Request.blank('/v1/a/c', {})
            resp = controller.PUT(req)
            self.assertEqual(resp.status_int, 404)
            self.assertFalse(Controller.autocreate_account.called)

        with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 10)):
            self.app.max_containers_per_account = 5
            req = Request.blank('/v1/a/c', {})
            resp = controller.PUT(req)
            self.assertEqual(resp.status_int, 403)
           


    def test_PUT_success_cases(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        self.app.allow_account_management = True
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, body='')):
            with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 1)):
                self.app.account_autocreate = True
                Controller.autocreate_account =  mock.MagicMock()
                req = Request.blank('/v1/a/c', {})
                resp = controller.PUT(req)
                self.assertEqual(resp.status_int, 200)
                self.assertFalse(Controller.autocreate_account.called)

    
    def test_DELETE(self):
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, body='')):
            with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 1)):
                controller = proxy_server.ContainerController(self.app, 'a', 'c')
                req = Request.blank('/v1/a/c', {})
                resp = controller.DELETE(req)
                self.assertEqual(resp.status_int, 200)

        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200, body='')):
            with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, None, None, None)):
                controller = proxy_server.ContainerController(self.app, 'a', 'c')
                req = Request.blank('/v1/a/c', {})
                resp = controller.DELETE(req)
                self.assertEqual(resp.status_int, 404)
        
        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(202)):
            with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 1)):
                controller = proxy_server.ContainerController(self.app, 'a', 'c')
                req = Request.blank('/v1/a/c', {})
                resp = controller.DELETE(req)
                self.assertEqual(resp.status_int, 404)

    def test_POST(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        req = Request.blank('/v1/a/c', {})
        with mock.patch('osd.proxyService.controllers.container.check_metadata', return_value=HTTPBadRequest(request=req)):
            resp = controller.POST(req)
            self.assertEqual(resp.status_int, 400)    
        
        with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=(None, None, None, None)):
            req = Request.blank('/v1/a/c', {})
            resp = controller.POST(req)
            self.assertEqual(resp.status_int, 404)

        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(200)):
            with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 1)):
                controller = proxy_server.ContainerController(self.app, 'a', 'c')
                req = Request.blank('/v1/a/c', {})
                resp = controller.POST(req)
                self.assertEqual(resp.status_int, 200)

        with mock.patch('osd.proxyService.controllers.base.http_connect',
                        fake_http_connect(503)):
            with mock.patch('osd.proxyService.controllers.base.Controller.account_info', return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 1)):
                controller = proxy_server.ContainerController(self.app, 'a', 'c')
                req = Request.blank('/v1/a/c', {})
                resp = controller.POST(req)
                self.assertEqual(resp.status_int, 503)

    def test_backend_requests(self):
        controller = proxy_server.ContainerController(self.app, 'a', 'c')
        req = Request.blank('/v1/a/c', {})
        headers = controller._backend_requests(req, 1, [{'ip': '1', 'port': '1'}], 'fs1', 'd1', 5.0, 0)
        self.assertTrue(headers[0]['X-Account-FileSystem'], 'fs1')
        self.assertTrue(headers[0]['X-Account-Directory'], 'd1')
        self.assertTrue(headers[0]['X-Account-Host'], '1:1')
        self.assertEqual(len(headers), 1)
        self.assertRaises(ZeroDivisionError, controller._backend_requests, req, 0, 'fs1', 'd1', [{'ip': '1', 'port': '1'}], 5.0, 0)

    def test_get_node_returning_empty_value(self):
        controller = proxy_server.ContainerController(self.app, 'AUTH_bob', 'test')
        Controller.account_info = mock.MagicMock(return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 1))
        req = Request.blank('/v1/a/c', {})
        self.app.container_ring.get_node = mock.MagicMock(return_value=([], '', '', 5.0, 0))
        resp = controller.GET(req)
        self.assertEqual(resp.status_int, 500)

        resp = controller.HEAD(req)
        self.assertEqual(resp.status_int, 500)

        resp = controller.PUT(req)
        self.assertEqual(resp.status_int, 500)

        resp = controller.DELETE(req)
        self.assertEqual(resp.status_int, 500)

        resp = controller.POST(req)
        self.assertEqual(resp.status_int, 500)

    def test_ZeroDivisionError_in_backend_requests(self):
        controller = proxy_server.ContainerController(self.app, 'AUTH_bob', 'test')
        Controller.account_info = mock.MagicMock(return_value=([{'ip': '1', 'port': '1'}], 'fs1', 'd1', 1))
        req = Request.blank('/v1/a/c')
        controller._backend_requests = mock.MagicMock(side_effect=ZeroDivisionError)

        resp = controller.DELETE(req)
        self.assertEqual(resp.status_int, 500)

        req = Request.blank('/v1/a/c/o', headers={'content-length': '0'})
        resp = controller.PUT(req)
        self.assertEqual(resp.status_int, 500)


if __name__ == '__main__':
    unittest.main()
