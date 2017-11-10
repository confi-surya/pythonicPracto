#-*- coding:utf-8 -*-
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

"""Tests for osd.obj.server"""
#pylint: disable=E1120
import cPickle as pickle
import datetime
import operator
import os
import mock
import unittest
import math
from shutil import rmtree
from StringIO import StringIO
from time import gmtime, strftime, time, struct_time
from tempfile import mkdtemp
from hashlib import md5
from contextlib import contextmanager

from eventlet import sleep, spawn, wsgi, listen, Timeout, tpool

#from nose import SkipTest

from osd.objectService.unitTests import FakeLogger, debug_logger, \
    FakeObjectRing, FakeContainerRing
from osd.objectService.unitTests import connect_tcp, readuntil2crlfs
from osd.objectService import server as object_server
from osd.objectService import diskfile
from osd.common import utils
from osd.common.utils import mkdirs, normalize_timestamp, \
    NullLogger, storage_directory, public, replication
from osd.common import constraints
from osd.common.swob import Request, HeaderKeyDict
from osd.common.exceptions import DiskFileNotExist, ChunkReadTimeout, DiskFileNoSpace
from osd.objectService.objectutils import LockManager, TranscationManager
from osd.common.ring.ring import hash_path


def mock_time(*args, **kwargs):
    return 5000.0

def mock_container_update(*args, **kwargs):
    return

def read_metadata(objfile):
    return diskfile.read_metadata(open(objfile))

def mock_http_connection(*args, **kwargs):
    return
class FakeResponse():
    def __init__(self):
        self.status = 200
        
def mock_add(*args, **kwargs):
    return 

def mock_release_lock(*args, **kwargs):
    return FakeResponse()

def mock_get_host_from_map(*args, **kwargs):
    return '1.2.3.4:0'

def mock_acquire_lock(response, headers):
    """ mock function for release_lock """
    class FakeConn(object):
        """ Fake class for HTTP Connection """
        def __init__(self, status, headers):
            """ initialize function """
            self.status = status
            self.reason = 'Fake'
            self.host = '1.2.3.4'
            self.port = '1234'
            self.headers = headers
            self.status_int = status

        def getresponse(self):
            """ return response """
            return self

        def read(self):
            """ read response """
            return ''

        def getheaders(self):
            return self.headers.items()

    return lambda *args, **kwargs: FakeConn(response, headers)

def mock_delete(*args, **kwargs):
    return 

def mock_load(*args, **kwargs):
    return True


def mock_get_global_map(*args, **kwargs):
    global_map = {'x1':'1,2,3'}
    global_version = float(1.1)
    conn_obj = 1
    return global_map, global_version, conn_obj

def mock_healthMonitoring(*args, **kwargs):
    print "2"
    return 1234

test_dir = os.path.join(mkdtemp(), 'tmp_test_object_server_ObjectController')

@mock.patch("osd.objectService.objectutils.LockManager.add", mock_add)
@mock.patch("osd.objectService.objectutils.LockManager.delete", mock_delete)
@mock.patch("osd.objectService.objectutils.TranscationManager.release_lock", mock_release_lock)
@mock.patch("osd.objectService.server.ObjectController.container_update", mock_container_update)
class TestObjectController(unittest.TestCase):
    """Test osd.obj.server.ObjectController"""

    @mock.patch("osd.objectService.server.get_global_map", mock_get_global_map)
    @mock.patch("osd.objectService.recovery_handler.ObjectRing", FakeObjectRing) 
    @mock.patch("osd.objectService.recovery_handler.ContainerRing", FakeContainerRing) 
    @mock.patch("osd.objectService.recovery_handler.RecoveryHandler")        
    def create_instance(self, conf,  RecoveryHandler):
        return object_server.ObjectController(
            conf, logger=debug_logger())
     
    def setUp(self):
        """Set up for testing osd.object.server.ObjectController"""
        utils.HASH_PATH_SUFFIX = 'endcap'
        utils.HASH_PATH_PREFIX = 'startcap'

        self.testdir = test_dir
        #self.testdir = \
        #    os.path.join(mkdtemp(), 'tmp_test_object_server_ObjectController')
        #self.filesystems = os.path.join(os.path.join(self.testdir, "export"))
        self.filesystems = os.path.join(os.path.join(self.testdir, "export"))
        self.filesystem = "fs1"
        self.dir = "o1"
        conf = {'filesystems': self.filesystems, 'mount_check': 'false'}
        self.object_controller = self.create_instance(conf)
        mkdirs(os.path.join(self.filesystems, self.filesystem))
        #mkdirs(os.path.join(self.filesystems, self.filesystem))
        self._orig_tpool_exc = tpool.execute
        tpool.execute = lambda f, *args, **kwargs: f(*args, **kwargs)
        self.df_mgr = diskfile.DiskFileManager(conf,
                                               self.object_controller.logger)
        self.lock_mgr_obj = LockManager(FakeLogger(''))

    def tearDown(self):
        """Tear down for testing osd.object.server.ObjectController"""
        rmtree(os.path.dirname(self.testdir))
        tpool.execute = self._orig_tpool_exc
    
    @contextmanager
    def read_meta_manage(self):
        read_metadata_orig = diskfile.DiskFile.read_metadata
        write_orig = diskfile.DiskFileWriter.write
        yield
        diskfile.DiskFile.read_metadata = read_metadata_orig
        diskfile.DiskFileWriter.write = write_orig

    
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_invalid_path(self):
        req = Request.blank('/fs1/a1/d1/o1/a/c', environ={'REQUEST_METHOD': 'PUT'}, headers={'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 400)
   
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_no_timestamp(self):
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT',
                                                      'CONTENT_LENGTH': '0'}, headers={'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 400)
    
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_no_content_type(self):
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Length': '6', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 400)
    
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_invalid_content_type(self):
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Length': '6',
                     'Content-Type': '\xff\xff', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 400)
        self.assert_('Content-Type' in resp.body)
     
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_no_content_length(self):
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Type': 'application/octet-stream', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY'
        del req.headers['Content-Length']
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 411)
    
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_zero_content_length(self):
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Type': 'application/octet-stream', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = ''
        self.assertEquals(req.headers['Content-Length'], '0')
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 201)
    
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_bad_transfer_encoding(self):
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Type': 'application/octet-stream', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY'
        req.headers['Transfer-Encoding'] = 'bad'
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 400)
     
    

    
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_old_timestamp(self):
        ts = time()
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(ts),
                     'Content-Length': '6',
                     'Content-Type': 'application/octet-stream', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 201)

        req = Request.blank('/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
                            headers={'X-Timestamp': normalize_timestamp(ts),
                                     'Content-Type': 'text/plain',
                                     'Content-Encoding': 'gzip', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY TWO'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 409)

        req = Request.blank('/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
                            headers={
                                'X-Timestamp': normalize_timestamp(ts - 1),
                                'Content-Type': 'text/plain',
                                'Content-Encoding': 'gzip', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY THREE'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 409)

    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_no_etag(self):
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Type': 'text/plain', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'test'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 201)

    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_invalid_etag(self):
        object_server.ObjectController.release_lock = mock.MagicMock()
        object_server.ObjectController.remove_trans_id = mock.MagicMock()
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Type': 'text/plain',
                     'ETag': 'invalid', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'test'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 422)
        self.assertTrue(object_server.ObjectController.release_lock.called)
        self.assertTrue(object_server.ObjectController.remove_trans_id.called)
    
    
    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_client_timeout(self):
        class FakeTimeout(BaseException):
            def __enter__(self):
                raise self

            def __exit__(self, typ, value, tb):
                pass
        # This is just so the test fails when run on older object server code
        # instead of exploding.
        if not hasattr(object_server, 'ChunkReadTimeout'):
            object_server.ChunkReadTimeout = None
        with mock.patch.object(object_server, 'ChunkReadTimeout', FakeTimeout):
            timestamp = normalize_timestamp(time())
            req = Request.blank(
                '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
                headers={'X-Timestamp': timestamp,
                         'Content-Type': 'text/plain',
                         'Content-Length': '6', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})
            req.environ['wsgi.input'] = StringIO('VERIFY')
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 408)

    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_container_connection(self):

        def mock_http_connect(response, with_exc=False):

            class FakeConn(object):

                def __init__(self, status, with_exc):
                    self.status = status
                    self.reason = 'Fake'
                    self.host = '1.2.3.4'
                    self.port = '1234'
                    self.with_exc = with_exc

                def getresponse(self):
                    if self.with_exc:
                        raise Exception('test')
                    return self

                def read(self, amt=None):
                    return ''

            return lambda *args, **kwargs: FakeConn(response, with_exc)

        old_http_connect = object_server.http_connect
        try:
            timestamp = normalize_timestamp(time())
            req = Request.blank(
                '/fs1/a1/d1/o1/a/c/o',
                environ={'REQUEST_METHOD': 'PUT'},
                headers={'X-Timestamp': timestamp,
                         'X-Container-Host': '1.2.3.4:0',
                         'X-Container-Partition': '3',
                         'X-Container-Device': 'sda1',
                         'X-Container-Timestamp': '1',
                         'Content-Type': 'application/new1',
                         'Content-Length': '0','X-GLOBAL-MAP-VERSION': 4.0})
            object_server.http_connect = mock_http_connect(201)
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 201)
            timestamp = normalize_timestamp(time())
            req = Request.blank(
                '/fs1/a1/d1/o1/a/c/o',
                environ={'REQUEST_METHOD': 'PUT'},
                headers={'X-Timestamp': timestamp,
                         'X-Container-Host': '1.2.3.4:0',
                         'X-Container-Partition': '3',
                         'X-Container-Device': 'sda1',
                         'X-Container-Timestamp': '1',
                         'Content-Type': 'application/new1',
                         'Content-Length': '0',
                         'X-GLOBAL-MAP-VERSION': 4.0})
            object_server.http_connect = mock_http_connect(500)
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 201)
            timestamp = normalize_timestamp(time())
            req = Request.blank(
                '/fs1/a1/d1/o1/a/c/o',
                environ={'REQUEST_METHOD': 'PUT'},
                headers={'X-Timestamp': timestamp,
                         'X-Container-Host': '1.2.3.4:0',
                         'X-Container-Partition': '3',
                         'X-Container-Device': 'sda1',
                         'X-Container-Timestamp': '1',
                         'Content-Type': 'application/new1',
                         'Content-Length': '0',
                         'X-GLOBAL-MAP-VERSION': 4.0})
            object_server.http_connect = mock_http_connect(500, with_exc=True)
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 201)
        finally:
            object_server.http_connect = old_http_connect

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(409, {}))
    @mock.patch("osd.objectService.server.get_global_map", mock_get_global_map)
    @mock.patch("osd.objectService.server.get_host_from_map", mock_get_host_from_map)
    def test_PUT_acquire_lock_fail_409(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'PUT'},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Length': '0',
                                     'Content-Type': 'text/plain',
				     'X-Container-Host': '1.2.3:0000',
                                     'X-GLOBAL-MAP-VERSION': 4.0})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 409)


    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(503, {}))
    def test_PUT_acquire_lock_fail_503(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'PUT'},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Length': '0',
                                     'Content-Type': 'text/plain',
				     'X-Container-Host': '1.2.3:0000',
                                     'X-GLOBAL-MAP-VERSION': 4.0})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 503)

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(400, {}))
    def test_PUT_acquire_lock_fail_400(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'PUT'},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Length': '0',
                                     'Content-Type': 'text/plain',
				     'X-Container-Host': '1.2.3:0000',
                                     'X-GLOBAL-MAP-VERSION': 4.0})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 503)

    def test_PUT_acquire_lock_exception(self):
        acquire_lock_orig = TranscationManager.acquire_lock
        TranscationManager.acquire_lock = mock.MagicMock(side_effect=Exception)
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'PUT'},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Length': '0',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000',
                                     'X-GLOBAL-MAP-VERSION': 4.0})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 503)
        TranscationManager.acquire_lock = acquire_lock_orig

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_add_exception(self):
        LockManager.add = mock.MagicMock(side_effect=Exception)
        object_server.ObjectController.release_lock = mock.MagicMock()
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'PUT'},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Length': '0',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000',
                                     'X-GLOBAL-MAP-VERSION': 4.0})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 500)
        self.assertTrue(object_server.ObjectController.release_lock.called)
        self.assertFalse(self.lock_mgr_obj.is_exists('fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d'))

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_read_metadata_exception(self):
        with self.read_meta_manage():
            diskfile.DiskFile.read_metadata = mock.MagicMock(side_effect=DiskFileNotExist)
            timestamp = normalize_timestamp(time())
            req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                                environ={'REQUEST_METHOD': 'PUT'},
                                headers={'X-Timestamp': timestamp,
				     'Content-Length': '0',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000',
                                     'X-GLOBAL-MAP-VERSION': 4.0})
            resp = req.get_response(self.object_controller)
            self.assertEqual(resp.status_int, 201)
            self.assertFalse(self.lock_mgr_obj.is_exists('fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d'))

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_read_metadata_wrong_timestamp(self):
        timestamp = normalize_timestamp(time())
        with self.read_meta_manage():
            diskfile.DiskFile.read_metadata = mock.MagicMock(return_value={'X-Timestamp': str(float(timestamp)+1)})
            object_server.ObjectController.release_lock = mock.MagicMock()
            object_server.ObjectController.remove_trans_id = mock.MagicMock()
            req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                                environ={'REQUEST_METHOD': 'PUT'},
                                headers={'X-Timestamp': timestamp,
				         'Content-Length': '0',
                                         'Content-Type': 'text/plain',
                                         'X-Container-Host': '1.2.3:0000',
                                         'X-GLOBAL-MAP-VERSION': 4.0})
            resp = req.get_response(self.object_controller)
            self.assertEqual(resp.status_int, 409)
            object_server.ObjectController.remove_trans_id.assert_called_once_with("fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d", "123")
            self.assertTrue(object_server.ObjectController.release_lock.called)
            self.assertTrue(object_server.ObjectController.remove_trans_id.called)

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_put_exception(self):
        timestamp = normalize_timestamp(time())
        with self.read_meta_manage():
            diskfile.DiskFileWriter.put = mock.MagicMock(side_effect=DiskFileNoSpace)
            object_server.ObjectController.release_lock = mock.MagicMock()
            object_server.ObjectController.remove_trans_id = mock.MagicMock()
            req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                                environ={'REQUEST_METHOD': 'PUT'},
                                headers={'X-Timestamp': timestamp,
				         'Content-Length': '0',
                                         'Content-Type': 'text/plain',
                                         'X-Container-Host': '1.2.3:0000',
                                         'X-GLOBAL-MAP-VERSION': 4.0})
            resp = req.get_response(self.object_controller)
            self.assertEqual(resp.status_int, 507)
            object_server.ObjectController.remove_trans_id.assert_called_once_with("fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d", "123")
            self.assertTrue(object_server.ObjectController.release_lock.called)
            self.assertTrue(object_server.ObjectController.remove_trans_id.called)




    ##changed
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_PUT_common(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': timestamp,
                     'Content-Length': '6',
                     'Content-Type': 'application/octet-stream', 'X-Container-Host': '1.2.3:0000','X-GLOBAL-MAP-VERSION': 4.0})

        req.body = 'VERIFY'
        print req,"-------------------"
        resp = req.get_response(self.object_controller)
        print "-------status",resp.status
        self.assertEquals(resp.status_int, 201)
        print "----hello", self.filesystems
        print "----hello2", self.filesystems
        objfile = os.path.join(self.filesystems, "fs1", "a1", hash_path("a"), "d1", hash_path("a", "c"), "o1", "data",
                                                        hash_path('a', 'c', 'o')) + ".data"
        metafile = os.path.join(self.filesystems, "fs1", "a1", hash_path("a"), "d1", hash_path("a", "c"), "o1", "meta",
                                                         hash_path('a', 'c', 'o')) + ".meta"
        print objfile, metafile
        try:
            self.assert_(os.path.isfile(objfile))
        except :
            print os.path.isfile(objfile)
        try:
            self.assert_(os.path.isfile(metafile))
        except:
            print os.path.isfile(metafile), "------------"
        self.assertEquals(open(objfile,'r').read(), 'VERIFY')
        self.assertEquals(read_metadata(metafile),
                          {'X-Timestamp': timestamp,
                           'Content-Length': '6',
                           'ETag': '0b4c12d7e0a73840c1c4f148fda3b037',
                           'Content-Type': 'application/octet-stream',
                           'name': '/a/c/o'})

    

        

    '''
    def test_write_exception(self):
        chunk_read_timeout_orig = object_server.ChunkReadTimeout
        @contextmanager
        def mock_chunkreadtimeout(self):
            try:
                yield
                raise chunk_read_timeout_orig()
            except:
                return
                raise
                #raise chunk_read_timeout_orig()
        with self.read_meta_manage():
            object_server.ChunkReadTimeout = mock_chunkreadtimeout
            timestamp = normalize_timestamp(time())
            object_server.ObjectController.remove_trans_id = mock.MagicMock()
            object_server.ObjectController.release_lock = mock.MagicMock()
            #diskfile.DiskFileWriter.write = mock.MagicMock(side_effect=ChunkReadTimeout)
            req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                                environ={'REQUEST_METHOD': 'PUT', 'wsgi.input': StringIO('a')},
                                headers={'X-Timestamp': timestamp,
				         'Content-Length': '1',
                                         'Content-Type': 'text/plain'})
            resp = req.get_response(self.object_controller)
            self.assertEqual(resp.status_int, 408)
            object_server.ObjectController.remove_trans_id.assert_called_once_with("fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d", "123")
            self.assertTrue(object_server.ObjectController.release_lock.called)
    '''




 
if __name__ == '__main__':
    unittest.main()

