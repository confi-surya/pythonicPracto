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
    @mock.patch("osd.objectService.server.healthMonitoring", mock_healthMonitoring) 
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
    #@mock.patch("osd.objectService.objectutils.http_connect", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
    @mock.patch("osd.objectService.server.get_global_map", mock_get_global_map)
    @mock.patch("osd.objectService.server.healthMonitoring", mock_healthMonitoring)
    @mock.patch("osd.objectService.recovery_handler.ObjectRing", FakeObjectRing)
    @mock.patch("osd.objectService.recovery_handler.ContainerRing", FakeContainerRing)
    def test_PUT_object(self):
        req = Request.blank(
            '/fs1/a1/d1/o1/a/c/o', environ={'REQUEST_METHOD': 'PUT'},
            headers={'X-Timestamp': normalize_timestamp(time()),
                     'Content-Type': 'application/octet-stream', 'X-Container-Host': '0.0.0.0:0000','X-GLOBAL-MAP-VERSION': 4.0})
        req.body = 'VERIFY'
        #req.headers['Transfer-Encoding'] = 'bad'
        resp = req.get_response(self.object_controller)
        self.assertEqual(resp.status_int, 201)


 
if __name__ == '__main__':
    unittest.main()

