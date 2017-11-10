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
from osd.common.ring.ring import hash_path
from osd.common.utils import mkdirs, normalize_timestamp, \
    NullLogger, storage_directory, public, replication
from osd.common import constraints
from osd.common.swob import Request, HeaderKeyDict
from osd.common.exceptions import DiskFileNotExist
from osd.objectService.objectutils import LockManager, TranscationManager


def mock_time(*args, **kwargs):
    return 5000.0

def mock_container_update(*args, **kwargs):
    return

def mock_http_connection(*args, **kwargs):
    return 

class FakeResponse():
    def __init__(self):
        self.status = 200
  
def mock_add(*args, **kwargs):
    return
 
def mock_release_lock(*args, **kwargs):
    return FakeResponse()

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
            #self.headers = {'x-request-trans-id': "123"}
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
    return 1234

 
#@mock.patch("osd.common.ring.ring.ObjectRingData",mock_load)        
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
    def create_instance(self,conf,  RecoveryHandler):
        return object_server.ObjectController(
            conf, logger=debug_logger())
    

    def setUp(self):
        """Set up for testing osd.object.server.ObjectController"""
        utils.HASH_PATH_SUFFIX = 'endcap'
        utils.HASH_PATH_PREFIX = 'startcap'
        self.testdir = \
            os.path.join(mkdtemp(), 'tmp_test_object_server_ObjectController')
        self.filesystems = os.path.join(os.path.join(self.testdir, "export"))
        self.filesystem = "fs1"
        self.dir = "d1"
        mkdirs(os.path.join(self.filesystems, self.filesystem))
        mkdirs(os.path.join(self.filesystems, self.filesystem))
        conf = {'filesystems': self.filesystems, 'mount_check': 'false'}
        self.object_controller = self.create_instance(conf)
        self.object_controller.bytes_per_sync = 1
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
        yield
        diskfile.DiskFile.read_metadata = read_metadata_orig
 
      

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_update_meta(self):
        # Test osd.objectService.server.ObjectController.POST
        original_headers = self.object_controller.allowed_headers
        test_headers = 'content-encoding foo bar'.split()
        self.object_controller.allowed_headers = set(test_headers)
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Type': 'application/x-test',
                                     'Foo': 'fooheader',
                                     'Baz': 'bazheader',
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-Two': 'Two',
				     'X-Container-Host': '1.2.3:0000'})
        req.body = 'VERIFY'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 201)

        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-3': 'Three',
                                     'X-Object-Meta-4': 'Four',
                                     'Content-Encoding': 'gzip',
                                     'Foo': 'fooheader',
                                     'Bar': 'barheader',
                                     'Content-Type': 'application/x-test',
				     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 202)

        req = Request.blank('/fs1/a1/d1/o1/a/c/o', headers={'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assert_("X-Object-Meta-1" not in resp.headers and
                     "X-Object-Meta-Two" not in resp.headers and
                     "X-Object-Meta-3" in resp.headers and
                     "X-Object-Meta-4" in resp.headers and
                     "Foo" in resp.headers and
                     "Bar" in resp.headers and
                     "Baz" not in resp.headers and
                     "Content-Encoding" in resp.headers)
        self.assertEquals(resp.headers['Content-Type'], 'application/x-test')

        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'HEAD', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0}, headers={'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assert_("X-Object-Meta-1" not in resp.headers and
                     "X-Object-Meta-Two" not in resp.headers and
                     "X-Object-Meta-3" in resp.headers and
                     "X-Object-Meta-4" in resp.headers and
                     "Foo" in resp.headers and
                     "Bar" in resp.headers and
                     "Baz" not in resp.headers and
                     "Content-Encoding" in resp.headers)
        self.assertEquals(resp.headers['Content-Type'], 'application/x-test')

        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Type': 'application/x-test',
				     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 202)
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', headers={'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assert_("X-Object-Meta-3" not in resp.headers and
                     "X-Object-Meta-4" not in resp.headers and
                     "Foo" not in resp.headers and
                     "Bar" not in resp.headers and
                     "Content-Encoding" not in resp.headers)
        self.assertEquals(resp.headers['Content-Type'], 'application/x-test')

        # test defaults
        self.object_controller.allowed_headers = original_headers
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Type': 'application/x-test',
                                     'Foo': 'fooheader',
                                     'X-Object-Meta-1': 'One',
                                     'Content-Encoding': 'gzip',
                                     'Content-Disposition': 'bar',
                                     'X-Container-Host': '1.2.3:0000'})
        req.body = 'VERIFY'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 201)
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',  headers={'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assert_("X-Object-Meta-1" in resp.headers and
                     "Foo" not in resp.headers and
                     "Content-Encoding" in resp.headers and
                     "Content-Disposition" in resp.headers)
        self.assertEquals(resp.headers['Content-Type'], 'application/x-test')

        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-3': 'Three',
                                     'Foo': 'fooheader',
                                     'Content-Type': 'application/x-test',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 202)
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', headers={'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assert_("X-Object-Meta-1" not in resp.headers and
                     "Foo" not in resp.headers and
                     "Content-Encoding" not in resp.headers and
                     "X-Object-Manifest" not in resp.headers and
                     "Content-Disposition" not in resp.headers and
                     "X-Object-Meta-3" in resp.headers)
        self.assertEquals(resp.headers['Content-Type'], 'application/x-test')

        # Test for empty metadata
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Type': 'application/x-test',
                                     'X-Object-Meta-3': '',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 202)
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', headers={'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.headers["x-object-meta-3"], '')

      
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_old_timestamp(self):
        ts = time()
        timestamp = normalize_timestamp(ts)
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Type': 'application/x-test',
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-Two': 'Two',
				     'X-Container-Host': '1.2.3:0000'})
        req.body = 'VERIFY'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 201)

        # Same timestamp should result in 409
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-3': 'Three',
                                     'X-Object-Meta-4': 'Four',
                                     'Content-Encoding': 'gzip',
                                     'Content-Type': 'application/x-test',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 409)

        # Earlier timestamp should result in 409
        timestamp = normalize_timestamp(ts - 1)
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-5': 'Five',
                                     'X-Object-Meta-6': 'Six',
                                     'Content-Encoding': 'gzip',
                                     'Content-Type': 'application/x-test',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 409)



    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_not_exist(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/fail',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 404)

    

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_invalid_path(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c', environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 400)



    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_no_timestamp(self):
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST ', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 400)




    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_bad_timestamp(self):
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': 'bad',
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 400)

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(409, {}))
    def test_POST_acquire_lock_fail_409(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 409)

 
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(503, {}))
    def test_POST_acquire_lock_fail_503(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 503)
    
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(400, {}))
    def test_POST_acquire_lock_fail_400(self):
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 503)

    def test_POST_acquire_lock_exception(self):
        acquire_lock_orig = TranscationManager.acquire_lock
        TranscationManager.acquire_lock = mock.MagicMock(side_effect=Exception)
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 503)
	TranscationManager.acquire_lock = acquire_lock_orig

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_add_exception(self):
        LockManager.add = mock.MagicMock(side_effect=Exception)
        object_server.ObjectController.release_lock = mock.MagicMock() 
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                            environ={'REQUEST_METHOD': 'POST', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 500)
        self.assertTrue(object_server.ObjectController.release_lock.called)
        self.assertFalse(self.lock_mgr_obj.is_exists('fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d'))
    
    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_read_metadata_exception(self):
        with self.read_meta_manage():
            diskfile.DiskFile.read_metadata = mock.MagicMock(side_effect=DiskFileNotExist)
            object_server.ObjectController.remove_trans_id = mock.MagicMock() 
            object_server.ObjectController.release_lock = mock.MagicMock() 
            timestamp = normalize_timestamp(time())
            req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                                environ={'REQUEST_METHOD': 'POST ', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                                headers={'X-Timestamp': timestamp,
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-2': 'Two',
                                     'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
            resp = req.get_response(self.object_controller)
            self.assertEqual(resp.status_int, 404)
            object_server.ObjectController.remove_trans_id.assert_called_once_with("fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d", "123")
            self.assertTrue(object_server.ObjectController.release_lock.called)
            self.assertFalse(self.lock_mgr_obj.is_exists('fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d'))

    @mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    def test_POST_read_metadata_wrong_timestamp(self):
    
        timestamp = normalize_timestamp(time())
        req = Request.blank('/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                            headers={'X-Timestamp': timestamp,
                                     'Content-Type': 'application/x-test',
                                     'X-Object-Meta-1': 'One',
                                     'X-Object-Meta-Two': 'Two',
                                     'X-Container-Host': '1.2.3:0000'})
        req.body = 'VERIFY'
        resp = req.get_response(self.object_controller)
        self.assertEquals(resp.status_int, 201)
        object_server.ObjectController.remove_trans_id = mock.MagicMock() 
        object_server.ObjectController.release_lock = mock.MagicMock() 
        with self.read_meta_manage():
            diskfile.DiskFile.read_metadata = mock.MagicMock(return_value={'X-Timestamp': str(float(timestamp)+1)})
            req = Request.blank('/fs1/a1/d1/o1/a/c/o',
                               environ={'REQUEST_METHOD': 'POST ', 'HTTP_X_GLOBAL_MAP_VERSION': 4.0},
                                headers={'X-Timestamp': timestamp,
                                         'X-Object-Meta-1': 'One',
                                         'X-Object-Meta-2': 'Two',
                                         'Content-Type': 'text/plain',
                                     'X-Container-Host': '1.2.3:0000'})
            resp = req.get_response(self.object_controller)
            self.assertEqual(resp.status_int, 409)
            object_server.ObjectController.remove_trans_id.assert_called_once_with("fd2d8dafa0570422b397941531342ca4/6eff33b3ff33852f4cfb514aaa7f5199/d1c91d91068a170a6bcf220efe77711d", "123")
            self.assertTrue(object_server.ObjectController.release_lock.called)



if __name__ == '__main__':
    unittest.main()
