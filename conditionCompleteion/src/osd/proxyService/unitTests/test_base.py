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
from mock import patch, MagicMock
from osd.proxyService.controllers.base import headers_to_container_info, \
    headers_to_account_info, headers_to_object_info, get_container_info, \
    get_container_memcache_key, get_account_info, get_account_memcache_key, \
    get_object_env_key, _get_cache_key, get_info, get_object_info, \
    Controller, GetOrHeadHandler, source_key, _prep_headers_to_info, \
    _set_info_cache, _set_object_info_cache, _get_info_cache, clear_info_cache
from osd.common.swob import Request, HTTPException, HeaderKeyDict
from osd.common.helper_functions import split_path
from osd.objectService.unitTests import fake_http_connect, FakeObjectRing, \
    FakeMemcache, FakeContainerRing, FakeAccountRing, FakeLogger
from osd.proxyService import server as proxy_server
from osd.common.request_helpers import get_sys_meta_prefix


FakeResponse_status_int = 201


class FakeResponse(object):
    def __init__(self, headers, env, account, container, obj, status=None):
        self.headers = headers
        self.status_int = FakeResponse_status_int
        self.environ = env
        self.status = status
        if obj:
            env_key = get_object_env_key(account, container, obj)
        else:
            cache_key, env_key = _get_cache_key(account, container)

        if account and container and obj:
            info = headers_to_object_info(headers, FakeResponse_status_int)
        elif account and container:
            info = headers_to_container_info(headers, FakeResponse_status_int)
        else:
            info = headers_to_account_info(headers, FakeResponse_status_int)
        env[env_key] = info

    def getheader(self, name):
        for key, value in self.headers.items():
            if name == key:
                return value


class FakeRequest(object):
    def __init__(self, env, path, swift_source=None):
        self.environ = env
        (version, account, container, obj) = split_path(path, 2, 4, True)
        self.account = account
        self.container = container
        self.obj = obj
        if obj:
            stype = 'object'
            self.headers = {'content-length': 5555,
                            'content-type': 'text/plain'}
        else:
            stype = container and 'container' or 'account'
            self.headers = {'x-%s-object-count' % (stype): 1000,
                            'x-%s-bytes-used' % (stype): 6666}
        if swift_source:
            meta = 'x-%s-meta-fakerequest-swift-source' % stype
            self.headers[meta] = swift_source

    def get_response(self, app):
        return FakeResponse(self.headers, self.environ, self.account,
                            self.container, self.obj)


class FakeCache(object):
    def __init__(self, val):
        self.val = val

    def get(self, *args):
        return self.val


class TestFuncs(unittest.TestCase):
    def setUp(self):
        self.app = proxy_server.Application(None, FakeMemcache(),
                                            account_ring=FakeAccountRing(''),
                                            container_ring=FakeContainerRing(''),
                                            object_ring=FakeObjectRing(''),
                                            logger=FakeLogger())

    def test_GETorHEAD_base(self):
        base = Controller(self.app)
        req = Request.blank('/v1/a/c/o/with/slashes')
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
            resp = base.GETorHEAD_base(req, 'object', FakeObjectRing(''), [{'ip': '1.2.3', 'port': '0000'}], \
                                        'fs0', 'd0', '/a/c/o/with/slashes', 5.0, 0)
        self.assertTrue('swift.object/a/c/o/with/slashes' in resp.environ)
        self.assertEqual(
            resp.environ['swift.object/a/c/o/with/slashes']['status'], 200)
        req = Request.blank('/v1/a/c/o')
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
            resp = base.GETorHEAD_base(req, 'object', FakeObjectRing(''), [{'ip': '1.2.3', 'port': '0000'}], \
                                        'fs0', 'd0', '/a/c/o', 5.0, 0)
        self.assertTrue('swift.object/a/c/o' in resp.environ)
        self.assertEqual(resp.environ['swift.object/a/c/o']['status'], 200)
        req = Request.blank('/v1/a/c')
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
            resp = base.GETorHEAD_base(req, 'container', FakeContainerRing(''), [{'ip': '1.2.3', 'port': '0000'}], \
                                        'fs0', 'd0', '/a/c', 5.0, 0)
        self.assertTrue('swift.container/a/c' in resp.environ)
        self.assertEqual(resp.environ['swift.container/a/c']['status'], 200)

        req = Request.blank('/v1/a')
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
            resp = base.GETorHEAD_base(req, 'account', FakeAccountRing(''), [{'ip': '1.2.3', 'port': '0000'}], \
                                        'fs0', 'd0', '/a', 5.0, 0)
        self.assertTrue('swift.account/a' in resp.environ)
        self.assertEqual(resp.environ['swift.account/a']['status'], 200)

    def test_test_account_infoget_info(self):
        global FakeResponse_status_int
        # Do a non cached call to account
        env = {}
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            info_a = get_info(None, env, 'a')
        # Check that you got proper info
        self.assertEquals(info_a['status'], 201)
        self.assertEquals(info_a['bytes'], 6666)
        self.assertEquals(info_a['total_object_count'], 1000)
        # Make sure the env cache is set
        self.assertEquals(env.get('swift.account/a'), info_a)

        # Do an env cached call to account
        info_a = get_info(None, env, 'a')
        # Check that you got proper info
        self.assertEquals(info_a['status'], 201)
        self.assertEquals(info_a['bytes'], 6666)
        self.assertEquals(info_a['total_object_count'], 1000)
        # Make sure the env cache is set
        self.assertEquals(env.get('swift.account/a'), info_a)

        # This time do env cached call to account and non cached to container
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            info_c = get_info(None, env, 'a', 'c')
        # Check that you got proper info
        self.assertEquals(info_a['status'], 201)
        self.assertEquals(info_c['bytes'], 6666)
        self.assertEquals(info_c['object_count'], 1000)
        # Make sure the env cache is set
        self.assertEquals(env.get('swift.account/a'), info_a)
        self.assertEquals(env.get('swift.container/a/c'), info_c)

        # This time do a non cached call to account than non cached to
        # container
        env = {}  # abandon previous call to env
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            info_c = get_info(None, env, 'a', 'c')
        # Check that you got proper info
        self.assertEquals(info_a['status'], 201)
        self.assertEquals(info_c['bytes'], 6666)
        self.assertEquals(info_c['object_count'], 1000)
        # Make sure the env cache is set
        self.assertEquals(env.get('swift.account/a'), info_a)
        self.assertEquals(env.get('swift.container/a/c'), info_c)

        # This time do an env cached call to container while account is not
        # cached
        del(env['swift.account/a'])
        info_c = get_info(None, env, 'a', 'c')
        # Check that you got proper info
        self.assertEquals(info_a['status'], 201)
        self.assertEquals(info_c['bytes'], 6666)
        self.assertEquals(info_c['object_count'], 1000)
        # Make sure the env cache is set and account still not cached
        self.assertEquals(env.get('swift.container/a/c'), info_c)

        # Do a non cached call to account not found with ret_not_found
        env = {}
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            try:
                FakeResponse_status_int = 404
                info_a = get_info(None, env, 'a', ret_not_found=True)
            finally:
                FakeResponse_status_int = 201
        # Check that you got proper info
        self.assertEquals(info_a['status'], 404)
        self.assertEquals(info_a['bytes'], 6666)
        self.assertEquals(info_a['total_object_count'], 1000)
        # Make sure the env cache is set
        self.assertEquals(env.get('swift.account/a'), info_a)

        # Do a cached call to account not found with ret_not_found
        info_a = get_info(None, env, 'a', ret_not_found=True)
        # Check that you got proper info
        self.assertEquals(info_a['status'], 404)
        self.assertEquals(info_a['bytes'], 6666)
        self.assertEquals(info_a['total_object_count'], 1000)
        # Make sure the env cache is set
        self.assertEquals(env.get('swift.account/a'), info_a)

        # Do a non cached call to account not found without ret_not_found
        env = {}
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            try:
                FakeResponse_status_int = 404
                info_a = get_info(None, env, 'a')
            finally:
                FakeResponse_status_int = 201
        # Check that you got proper info
        self.assertEquals(info_a, None)
        self.assertEquals(env['swift.account/a']['status'], 404)

        # Do a cached call to account not found without ret_not_found
        info_a = get_info(None, env, 'a')
        # Check that you got proper info
        self.assertEquals(info_a, None)
        self.assertEquals(env['swift.account/a']['status'], 404)

    def test_get_container_info_swift_source(self):
        req = Request.blank("/v1/a/c", environ={'swift.cache': FakeCache({})})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_container_info(req.environ, 'app', swift_source='MC')
        self.assertEquals(resp['meta']['fakerequest-swift-source'], 'MC')

    def test_get_object_info_swift_source(self):
        req = Request.blank("/v1/a/c/o",
                            environ={'swift.cache': FakeCache({})})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_object_info(req.environ, 'app', swift_source='LU')
        self.assertEquals(resp['meta']['fakerequest-swift-source'], 'LU')

    def test_get_container_info_no_cache(self):
        req = Request.blank("/v1/AUTH_account/cont",
                            environ={'swift.cache': FakeCache({})})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_container_info(req.environ, 'xxx')
        self.assertEquals(resp['bytes'], 6666)
        self.assertEquals(resp['object_count'], 1000)

    def test_get_container_info_cache(self):
        cached = {'status': 404,
                  'bytes': 3333,
                  'object_count': 10}
                  # simplejson sometimes hands back strings, sometimes unicodes
                  #'versions': u"\u1F4A9"}
        req = Request.blank("/v1/account/cont",
                            environ={'swift.cache': FakeCache(cached)})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_container_info(req.environ, 'xxx')
        self.assertEquals(resp['bytes'], 3333)
        self.assertEquals(resp['object_count'], 10)
        self.assertEquals(resp['status'], 404)
        #self.assertEquals(resp['versions'], "\xe1\xbd\x8a\x39")

    def test_get_container_info_env(self):
        cache_key = get_container_memcache_key("account", "cont")
        env_key = 'swift.%s' % cache_key
        req = Request.blank("/v1/account/cont",
                            environ={env_key: {'bytes': 3867},
                                     'swift.cache': FakeCache({})})
        resp = get_container_info(req.environ, 'xxx')
        self.assertEquals(resp['bytes'], 3867)

    def test_get_account_info_swift_source(self):
        req = Request.blank("/v1/a", environ={'swift.cache': FakeCache({})})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_account_info(req.environ, 'a', swift_source='MC')
        self.assertEquals(resp['meta']['fakerequest-swift-source'], 'MC')

    def test_get_account_info_no_cache(self):
        req = Request.blank("/v1/AUTH_account",
                            environ={'swift.cache': FakeCache({})})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_account_info(req.environ, 'xxx')
        self.assertEquals(resp['bytes'], 6666)
        self.assertEquals(resp['total_object_count'], 1000)

    def test_get_account_info_cache(self):
        # The original test that we prefer to preserve
        cached = {'status': 404,
                  'bytes': 3333,
                  'total_object_count': 10}
        req = Request.blank("/v1/account/cont",
                            environ={'swift.cache': FakeCache(cached)})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_account_info(req.environ, 'xxx')
        self.assertEquals(resp['bytes'], 3333)
        self.assertEquals(resp['total_object_count'], 10)
        self.assertEquals(resp['status'], 404)

        # Here is a more realistic test
        cached = {'status': 404,
                  'bytes': '3333',
                  'container_count': '234',
                  'total_object_count': '10',
                  'meta': {}}
        req = Request.blank("/v1/account/cont",
                            environ={'swift.cache': FakeCache(cached)})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_account_info(req.environ, 'xxx')
        self.assertEquals(resp['status'], 404)
        self.assertEquals(resp['bytes'], '3333')
        self.assertEquals(resp['container_count'], 234)
        self.assertEquals(resp['meta'], {})
        self.assertEquals(resp['total_object_count'], '10')

    def test_get_account_info_env(self):
        cache_key = get_account_memcache_key("account")
        env_key = 'swift.%s' % cache_key
        req = Request.blank("/v1/account",
                            environ={env_key: {'bytes': 3867},
                                     'swift.cache': FakeCache({})})
        resp = get_account_info(req.environ, 'xxx')
        self.assertEquals(resp['bytes'], 3867)

    def test_get_object_info_env(self):
        cached = {'status': 200,
                  'length': 3333,
                  'type': 'application/json',
                  'meta': {}}
        env_key = get_object_env_key("account", "cont", "obj")
        req = Request.blank("/v1/account/cont/obj",
                            environ={env_key: cached,
                                     'swift.cache': FakeCache({})})
        resp = get_object_info(req.environ, 'xxx')
        self.assertEquals(resp['length'], 3333)
        self.assertEquals(resp['type'], 'application/json')

    def test_get_object_info_no_env(self):
        req = Request.blank("/v1/account/cont/obj",
                            environ={'swift.cache': FakeCache({})})
        with patch('osd.proxyService.controllers.base.'
                   '_prepare_pre_auth_info_request', FakeRequest):
            resp = get_object_info(req.environ, 'xxx')
        self.assertEquals(resp['length'], 5555)
        self.assertEquals(resp['type'], 'text/plain')

    def test_headers_to_container_info_missing(self):
        resp = headers_to_container_info({}, 404)
        self.assertEquals(resp['status'], 404)
        self.assertEquals(resp['read_acl'], None)
        self.assertEquals(resp['write_acl'], None)

    def test_headers_to_container_info_meta(self):
        headers = {'X-Container-Meta-Whatevs': 14,
                   'x-container-meta-somethingelse': 0}
        resp = headers_to_container_info(headers.items(), 200)
        self.assertEquals(len(resp['meta']), 2)
        self.assertEquals(resp['meta']['whatevs'], 14)
        self.assertEquals(resp['meta']['somethingelse'], 0)

    def test_headers_to_container_info_sys_meta(self):
        prefix = get_sys_meta_prefix('container')
        headers = {'%sWhatevs' % prefix: 14,
                   '%ssomethingelse' % prefix: 0}
        resp = headers_to_container_info(headers.items(), 200)
        self.assertEquals(len(resp['sysmeta']), 2)
        self.assertEquals(resp['sysmeta']['whatevs'], 14)
        self.assertEquals(resp['sysmeta']['somethingelse'], 0)

    def test_headers_to_container_info_values(self):
        headers = {
            'x-container-read': 'readvalue',
            'x-container-write': 'writevalue',
            'x-versions-location': 'versionvalue'
        }
        resp = headers_to_container_info(headers.items(), 200)
        self.assertEquals(resp['read_acl'], 'readvalue')
        self.assertEquals(resp['write_acl'], 'writevalue')
        self.assertTrue('versions' in resp)

        headers['x-unused-header'] = 'blahblahblah'
        self.assertEquals(
            resp,
            headers_to_container_info(headers.items(), 200))

    def test_headers_to_account_info_missing(self):
        resp = headers_to_account_info({}, 404)
        self.assertEquals(resp['status'], 404)
        self.assertEquals(resp['bytes'], None)
        self.assertEquals(resp['container_count'], None)

    def test_headers_to_account_info_meta(self):
        headers = {'X-Account-Meta-Whatevs': 14,
                   'x-account-meta-somethingelse': 0}
        resp = headers_to_account_info(headers.items(), 200)
        self.assertEquals(len(resp['meta']), 2)
        self.assertEquals(resp['meta']['whatevs'], 14)
        self.assertEquals(resp['meta']['somethingelse'], 0)

    def test_headers_to_account_info_sys_meta(self):
        prefix = get_sys_meta_prefix('account')
        headers = {'%sWhatevs' % prefix: 14,
                   '%ssomethingelse' % prefix: 0}
        resp = headers_to_account_info(headers.items(), 200)
        self.assertEquals(len(resp['sysmeta']), 2)
        self.assertEquals(resp['sysmeta']['whatevs'], 14)
        self.assertEquals(resp['sysmeta']['somethingelse'], 0)

    def test_headers_to_account_info_values(self):
        headers = {
            'x-account-object-count': '10',
            'x-account-container-count': '20',
        }
        resp = headers_to_account_info(headers.items(), 200)
        self.assertEquals(resp['total_object_count'], '10')
        self.assertEquals(resp['container_count'], '20')

        headers['x-unused-header'] = 'blahblahblah'
        self.assertEquals(
            resp,
            headers_to_account_info(headers.items(), 200))

    def test_headers_to_object_info_missing(self):
        resp = headers_to_object_info({}, 404)
        self.assertEquals(resp['status'], 404)
        self.assertEquals(resp['length'], None)
        self.assertEquals(resp['etag'], None)

    def test_headers_to_object_info_meta(self):
        headers = {'X-Object-Meta-Whatevs': 14,
                   'x-object-meta-somethingelse': 0}
        resp = headers_to_object_info(headers.items(), 200)
        self.assertEquals(len(resp['meta']), 2)
        self.assertEquals(resp['meta']['whatevs'], 14)
        self.assertEquals(resp['meta']['somethingelse'], 0)

    def test_headers_to_object_info_values(self):
        headers = {
            'content-length': '1024',
            'content-type': 'application/json',
        }
        resp = headers_to_object_info(headers.items(), 200)
        self.assertEquals(resp['length'], '1024')
        self.assertEquals(resp['type'], 'application/json')

        headers['x-unused-header'] = 'blahblahblah'
        self.assertEquals(
            resp,
            headers_to_object_info(headers.items(), 200))

    def test_have_quorum(self):
        base = Controller(self.app)
        # just throw a bunch of test cases at it
        self.assertEqual(base.have_quorum([201, 404], 3), False)
        self.assertEqual(base.have_quorum([201, 201], 4), False)
        self.assertEqual(base.have_quorum([201, 201, 404, 404], 4), False)
        self.assertEqual(base.have_quorum([201, 503, 503, 201], 4), False)
        self.assertEqual(base.have_quorum([201, 201], 3), True)
        self.assertEqual(base.have_quorum([404, 404], 3), True)
        self.assertEqual(base.have_quorum([201, 201], 2), True)
        self.assertEqual(base.have_quorum([404, 404], 2), True)
        self.assertEqual(base.have_quorum([201, 404, 201, 201], 4), True)

    def test_range_fast_forward(self):
        req = Request.blank('/')
        handler = GetOrHeadHandler(None, req, None, None, None, None, None, None, {})
        handler.fast_forward(50)
        self.assertEquals(handler.backend_headers['Range'], 'bytes=50-')

        handler = GetOrHeadHandler(None, req, None, None, None, None, None, None,
                                   {'Range': 'bytes=23-50'})
        handler.fast_forward(20)
        self.assertEquals(handler.backend_headers['Range'], 'bytes=43-50')
        self.assertRaises(HTTPException,
                          handler.fast_forward, 80)

        handler = GetOrHeadHandler(None, req, None, None, None, None, None, None,
                                   {'Range': 'bytes=23-'})
        handler.fast_forward(20)
        self.assertEquals(handler.backend_headers['Range'], 'bytes=43-')

        handler = GetOrHeadHandler(None, req, None, None, None, None, None, None,
                                   {'Range': 'bytes=-100'})
        handler.fast_forward(20)
        self.assertEquals(handler.backend_headers['Range'], 'bytes=-80')

    def test_transfer_headers_with_sysmeta(self):
        base = Controller(self.app)
        good_hdrs = {'x-base-sysmeta-foo': 'ok',
                     'X-Base-sysmeta-Bar': 'also ok'}
        bad_hdrs = {'x-base-sysmeta-': 'too short'}
        hdrs = dict(good_hdrs)
        hdrs.update(bad_hdrs)
        dst_hdrs = HeaderKeyDict()
        base.transfer_headers(hdrs, dst_hdrs)
        self.assertEqual(HeaderKeyDict(good_hdrs), dst_hdrs)
    
    def test_container_info(self):
        base = Controller(self.app)
        info = base.container_info('a', 'c')
        self.assertEquals(info['filesystem'], None)
        self.assertEquals(info['node'], None)
        self.assertEquals(info['directory'], None)

        info = {'status': '201', 'bytes': '2000', 'total_object_count': '1000'}
        with patch('osd.proxyService.controllers.base.get_info', return_value=info):
            info = base.container_info('a', 'c')
            self.assertEquals(info['filesystem'], 'fs1')
            self.assertEquals(info['node'], [{'ip': '1.2.3.4', 'port': '0000'}])
            self.assertEquals(info['directory'], 'a1')
            
    
    def test_account_info(self):
        base = Controller(self.app)
        info = base.account_info('a')
   
        info = {'status': '201', 'bytes': '2000', 'container_count': '1000'}
        with patch('osd.proxyService.controllers.base.get_info', return_value=info):
            info = base.account_info('a')
            self.assertEquals(info, ([{'ip': '1.2.3.4', 'port': '0000'}], 'fs1', 'a', 1000))
        
        info = {'status': '201', 'bytes': '2000', 'container_count': None}
        with patch('osd.proxyService.controllers.base.get_info', return_value=info):
            info = base.account_info('a')
            self.assertEquals(info, ([{'ip': '1.2.3.4', 'port': '0000'}], 'fs1', 'a', 0))

    def test_generate_request_headers(self):
        base = Controller(self.app)
        src_headers = {'x-remove-base-meta-owner': 'x',
                       'x-base-meta-size': '151M',
                       'new-owner': 'Kun'}
        req = Request.blank('/v1/a/c/o', headers=src_headers)
        dst_headers = base.generate_request_headers(5.0, 0, req, transfer=True)
        expected_headers = {'x-base-meta-owner': '',
                            'x-base-meta-size': '151M'}
        for k, v in expected_headers.iteritems():
            self.assertTrue(k in dst_headers)
            self.assertEqual(v, dst_headers[k])
        self.assertFalse('new-owner' in dst_headers)

    def test_generate_request_headers_with_sysmeta(self):
        base = Controller(self.app)
        good_hdrs = {'x-base-sysmeta-foo': 'ok',
                     'X-Base-sysmeta-Bar': 'also ok'}
        bad_hdrs = {'x-base-sysmeta-': 'too short'}
        hdrs = dict(good_hdrs)
        hdrs.update(bad_hdrs)
        req = Request.blank('/v1/a/c/o', headers=hdrs)
        dst_headers = base.generate_request_headers(5.0, 0, req, transfer=True)
        for k, v in good_hdrs.iteritems():
            self.assertTrue(k.lower() in dst_headers)
            self.assertEqual(v, dst_headers[k.lower()])
        for k, v in bad_hdrs.iteritems():
            self.assertFalse(k.lower() in dst_headers)

class TestGlobalMethod(unittest.TestCase):
    def setUp(self):
        self.app = proxy_server.Application(None, FakeMemcache(),
                                            account_ring=FakeAccountRing(''),
                                            container_ring=FakeContainerRing(''),
                                            object_ring=FakeObjectRing(''),
                                            logger=FakeLogger())
        self.base = Controller(self.app)

    def tearDown(self):
        pass

    def test_source_key(self):
        self.assertEquals(source_key(FakeResponse({'test': 'value'}, {}, 'a', 'c', 'o')), 0.0)
        self.assertEquals(source_key(FakeResponse({'x-put-timestamp': '123'}, {}, 'a', 'c', 'o')), 123.0) 
        self.assertEquals(source_key(FakeResponse({'x-timestamp': '1231.0'}, {}, 'a', 'c', 'o')), 1231.0)

    def test_get_account_memcache_key(self):
        self.assertEquals(get_account_memcache_key('test'), 'account/test')
        self.assertEquals(get_account_memcache_key(''), 'account/')
  
    def test_get_container_memcache_key(self):
        self.assertEquals(get_container_memcache_key('test', 'test_cont'), 'container/test/test_cont')
        self.assertRaises(ValueError, get_container_memcache_key, '', '')
        self.assertEquals(get_container_memcache_key('', 'test_cont'), 'container//test_cont')

    def test__prep_headers_to_info(self):
        self.assertEquals(_prep_headers_to_info({'X-Container-Meta-Test': 'Value'}, 'container'), ({}, {'test': 'Value'}, {}))
        self.assertEquals(_prep_headers_to_info({'X-Timestamp': '123'}, 'container'), ({'x-timestamp': '123'}, {}, {}))
        self.assertEquals(_prep_headers_to_info({'X-Timestamp': '123', 'X-Content-Length': 12}, 'object'), \
            ({'x-timestamp': '123', 'x-content-length': 12}, {}, {}))
        self.assertEquals(_prep_headers_to_info({'X-Container-Sysmeta-test': 'value'}, 'container'), \
           ({}, {}, {'test': 'value'}))
        self.assertEquals(_prep_headers_to_info({'X-Container-Sysmeta-test': 'value', 'X-Container-Meta-Test': 'Value', 'X-Content-Length': 12}, \
           'container'), ({'x-content-length': 12}, {'test': 'Value'}, {'test': 'value'}))

    def test__get_cache_key(self):
        self.assertEquals(_get_cache_key('acc', 'cont'), ('container/acc/cont', 'swift.container/acc/cont'))
        self.assertEquals(_get_cache_key('acc', None), ('account/acc', 'swift.account/acc'))

    def test_get_object_env_key(self):
        self.assertEquals(get_object_env_key('a', 'c', 'o'), 'swift.object/a/c/o')

    def test__set_info_cache(self):
        env = {}
        _set_info_cache(self.app, env, 'a', 'c', FakeResponse({'x-container-meta-test': 'value'}, {}, 'a', 'c', None))
        env_expected = {'swift.container/a/c': {'status': 201, 'sync_key': None, 'meta': {'test': 'value'}, 'write_acl': None, 'object_count': None, 'sysmeta': {}, 'bytes': None, 'read_acl': None, 'versions': None}}
        self.assertEquals(env, env_expected)
        env = {}
        _set_info_cache(self.app, env, 'a', None, FakeResponse({'x-account-meta-test': 'value'}, {}, 'a', None, None))
        env_expected = {'swift.account/a': {'status': 201, 'container_count': None, 'bytes': None, 'total_object_count': None, 'meta': {'test': 'value'}, 'sysmeta': {}}}
        self.assertEquals(env, env_expected)

    def test__set_object_info_cache(self):
        env = {}
        _set_object_info_cache(self.app, env, 'a', 'c', 'o', FakeResponse({}, {}, 'a', 'c', 'o'))
        env_expected = {'swift.object/a/c/o': {'status': 201, 'length': None, 'etag': None, 'type': None, 'meta': {}}}
        self.assertEquals(env, env_expected)

    def test__get_info_cache(self):
       env = {'swift.account/test_Account': {'just_a_test': 'empty'}}
       self.assertEquals(_get_info_cache(self.app, env, 'test_Account', None), {'just_a_test': 'empty'})
       env = {'swift.container/a/c': {'just_a_test': 'empty'}}
       self.assertEquals(_get_info_cache(self.app, env, 'a', 'c'), {'just_a_test': 'empty'})
       self.assertEquals(_get_info_cache(self.app, {}, 'test_Account', None), None)

    def test_is_good_source(self):
        req = Request.blank('/')
        handler = GetOrHeadHandler(None, req, 'Object', None, None, None, None, None,
                                   {'Range': 'bytes=-100'})
        self.assertEquals(handler.is_good_source(FakeResponse({}, {}, 'a', 'c', 'o', 416)), True)
        handler = GetOrHeadHandler(None, req, 'Container', None, None, None, None, None,
                                   {'Range': 'bytes=-100'})
        self.assertEquals(handler.is_good_source(FakeResponse({}, {}, 'a', 'c', 'o', 416)), False)
        self.assertEquals(handler.is_good_source(FakeResponse({}, {}, 'a', 'c', 'o', 200)), True)
        self.assertEquals(handler.is_good_source(FakeResponse({}, {}, 'a', 'c', 'o', 311)), True)

    def test__get_source_and_node(self):
        req = Request.blank('/')
        handler = GetOrHeadHandler(self.app, req, 'Object', None, [{'ip': '1.2.3', 'port': '0000'}], None, None, None,
                                   {'Range': 'bytes=-100'})
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(side_effect=Exception)):    
            self.assertEquals(handler._get_source_and_node(), (None, None))
        
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
            self.assertEquals(handler._get_source_and_node()[1], {'ip': '1.2.3', 'port': '0000'})
        
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(507)):
            self.assertEquals(handler._get_source_and_node(), (None, None))

    def test_get_working_response(self):
        req = Request.blank('/')
        handler = GetOrHeadHandler(self.app, req, 'Object', None, [{'ip': '1.2.3', 'port': '0000'}], None, None, None,
                                   {'Range': 'bytes=-100'})
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(side_effect=Exception)):
            self.assertEquals(handler.get_working_response(req), None)
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
            self.assertTrue(handler.get_working_response(req) is not None)
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(503)):
            self.assertEquals(handler.get_working_response(req), None)

    def test__x_remove_headers(self):
        self.assertEquals(self.base._x_remove_headers(), [])

    def test_transfer_headers(self):
        dst_headers = {}
        src_headers = {'x-remove-base-meta-key': 'value'}
        self.base.transfer_headers(src_headers, dst_headers)
        self.assertEquals(dst_headers, {'x-base-meta-key': ''})
        dst_headers = {}
        src_headers = {'x-base-sysmeta-key': 'value'}
        self.base.transfer_headers(src_headers, dst_headers)
        self.assertEquals(dst_headers, {'x-base-sysmeta-key': 'value'})

    def test_account_info(self):
        with patch('osd.proxyService.controllers.base.get_info',
                   return_value={'container_count': 2}):
            self.assertEquals(self.base.account_info('a'), ([{'ip': '1.2.3.4', 'port': '0000'}], 'fs1', 'a', 2))
        with patch('osd.proxyService.controllers.base.get_info',
                   return_value={'container_count': None}):
            self.assertEquals(self.base.account_info('a'), ([{'ip': '1.2.3.4', 'port': '0000'}], 'fs1', 'a', 0))
        self.assertEquals(self.base.account_info('a'), (None, None, None, None))

    def test_container_info(self):
       with patch('osd.proxyService.controllers.base.get_info',
                   return_value={'object_count': 2}):
            self.assertEquals(self.base.container_info('a', 'c'), {'node': [{'ip': '1.2.3.4', 'port': '0000'}], 'filesystem': 'fs1', \
                'directory': 'a1', 'object_count': 2})
       with patch('osd.proxyService.controllers.base.get_info',
                   return_value=None):
            self.assertEquals(self.base.container_info('a', 'c'), {'status': 0, 'sync_key': None, 'node': None, 'meta': {}, \
               'sysmeta': {}, 'read_acl': None, 'object_count': None, 'write_acl': None, 'bytes': None, 'filesystem': None, 'directory': None, 'versions': None})
    
    def test_make_requests(self):
        req = Request.blank('/v1/a/c/o') 
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', side_effect=Exception):
            resp = self.base.make_requests(req, FakeObjectRing(''), [{'ip': '1.2.3.4', 'port': '0000'}], 'fs1', \
                'd1', 'HEAD', '/a/c/o', {'test': 'value'})
            self.assertEqual(resp.status, '503 Internal Server Error')
        with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
            resp = self.base.make_requests(req, FakeObjectRing(''), [{'ip': '1.2.3.4', 'port': '0000'}], 'fs1', \
                'd1', 'HEAD', '/a/c/o', {'test': 'value'})
            self.assertEqual(resp.status.split()[0], '200')

    def test_best_response(self):
        req = Request.blank('/v1/a/c/o')
        resp = self.base.best_response(req, [], [], [], 'object')
        self.assertEqual(resp.status, '503 Internal Server Error')
  
        req = Request.blank('/v1/a/c/o')
        resp = self.base.best_response(req, [200], ['Fake'], [''], 'object', {})
        self.assertEqual(resp.status, '200 Fake')
    
        req = Request.blank('/v1/a/c/o')
        resp = self.base.best_response(req, [412], ['Fake'], [''], 'object', {})
        self.assertEqual(resp.status, '412 Fake')

        req = Request.blank('/v1/a/c/o')
        resp = self.base.best_response(req, [500], ['Fake'], [''], 'object', {})
        self.assertEqual(resp.status, '503 Internal Server Error')

    def test_autocreate_account(self):
       with patch('osd.proxyService.controllers.base.'
                   'http_connect', side_effect=Exception):
           clear_info_cache = MagicMock()
           self.base.autocreate_account({}, 'a')
           self.assertFalse(clear_info_cache.called)
       
       with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(503)):
           clear_info_cache = MagicMock()
           self.base.autocreate_account({}, 'a')
           self.assertFalse(clear_info_cache.called)

    def test_autocreate_account_success_case(self):
       with patch('osd.proxyService.controllers.base.'
                   'http_connect', fake_http_connect(200)):
           clear_info_cache = MagicMock()
           self.base.autocreate_account({}, 'a')
           #self.assertTrue(clear_info_cache.called)
           #clear_info_cache.assert_called_once_with(self.app, {}, 'a')

    def test_add_component_in_block_req(self):
        base = Controller(self.app)
        self.base.add_component_in_block_req('Account', 'X1')
        self.assertEqual(self.base.block_req_dict, {('Account', 'X1'): True})
        self.base.add_component_in_block_req('Account', 'X2')
        self.assertEqual(self.base.block_req_dict, {('Account', 'X1'): True, ('Account', 'X2'): True})

    def test_delete_component_from_block_req(self):
        base = Controller(self.app)
        #self.base.block_req_dict = MagicMock({('1.2.3.4', '0000'): True, ('1.2.3.4', '1111'): False})
        self.base.add_component_in_block_req('Account', 'X1')
        self.base.add_component_in_block_req('Account', 'X2')
        self.base.delete_component_from_block_req('Account', 'X1')
        self.assertEqual(self.base.block_req_dict, {('Account', 'X2'): True})

    def test_is_req_blocked_without_component(self):
        base = Controller(self.app)
        self.assertFalse(self.base.is_req_blocked('Account', 'X1'))

    def test_is_req_blocked_with_component(self):
        self.base.add_component_in_block_req('Account', 'X1')
        self.assertTrue(self.base.is_req_blocked('Account', 'X1'))
        self.assertFalse(self.base.is_req_blocked('', ''))
        self.base.add_component_in_block_req('Account', 'X2')
        self.assertTrue(self.base.is_req_blocked('Account', 'X2'))
        self.base.add_component_in_block_req('Account', 'X3')
        self.assertTrue(self.base.is_req_blocked('Account', 'X3'))
        self.assertFalse(self.base.is_req_blocked('Account', 'X5'))

    def test_update_map_for_account(self):
        pass


if __name__ == '__main__':
    unittest.main()
