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
from osd.common.exceptions import DiskFileDeviceUnavailable
import osd


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
 
def mock_acquire_lock(*args, **kwargs):
    return "123"
 
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

 
@mock.patch("osd.objectService.objectutils.LockManager.add", mock_add)
@mock.patch("osd.objectService.objectutils.LockManager.delete", mock_delete)
@mock.patch("osd.objectService.objectutils.TranscationManager.acquire_lock", mock_acquire_lock)
@mock.patch("osd.objectService.objectutils.TranscationManager.release_lock", mock_release_lock)
@mock.patch("osd.objectService.server.ObjectController.container_update", mock_container_update)

class TestObjectController(unittest.TestCase):
    """Test osd.obj.server.ObjectController"""

    @mock.patch("osd.objectService.server.get_global_map", mock_get_global_map)
    @mock.patch("osd.objectService.server.healthMonitoring", mock_healthMonitoring)
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
   
    def tearDown(self):
        """Tear down for testing osd.object.server.ObjectController"""
        rmtree(os.path.dirname(self.testdir))
        tpool.execute = self._orig_tpool_exc

    @mock.patch("time.time", mock_time)
    def test_RECOVER(self):
        with mock.patch("osd.objectService.server.get_ip_port_by_id", return_value={'ip': '1.2.3', 'port': '000'}):
        # Test osd.obj.server.ObjectController.RECOVER
            # In memory case
            osd.objectService.server.RecoveryHandler.triggered_recovery = mock.MagicMock()
            osd.objectService.server.LockManager.is_exists = mock.MagicMock(return_value=True)
            req = Request.blank('/fs1/a1/d1/a/c',
                                environ={'REQUEST_METHOD': 'RECOVER'}, headers={"x-operation": 'GET', "x-request-trans-id": 'xyz', \
                                "x-server-id": 'xxx', "x-object-path":'a/c/o'})
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 200)
            self.assertFalse(osd.objectService.server.RecoveryHandler.triggered_recovery.called)
            
            #not in memory case successfulcase
            osd.objectService.server.LockManager.is_exists = mock.MagicMock(return_value=False)
            req = Request.blank('/fs1/a1/d1/a/c/o',
                                environ={'REQUEST_METHOD': 'RECOVER'}, headers={"x-operation": 'GET', "x-request-trans-id": 'xyz', \
                                "x-server-id": 'xxx', "x-object-path":'a/c/o'})
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 200)

            
            
            #not in memory case unsuccessful case
            osd.objectService.server.TranscationManager.release_lock = mock.MagicMock(side_effect=Exception)
            req = Request.blank('/fs1/a1/d1/a/c/o', environ={'REQUEST_METHOD': 'RECOVER'}, headers={"x-operation": 'GET', "x-request-trans-id": 'xyz', \
                                "x-server-id": 'xxx', "x-object-path":'a/c/o'})
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 200)
            self.assertFalse(osd.objectService.server.RecoveryHandler.triggered_recovery.called)

            #not in memory DELETE PUT case
            req = Request.blank('/a/c/o', {'REQUEST_METHOD': 'RECOVER'}, \
                                headers={"x-operation": 'PUT', "x-request-trans-id": 'xyz', \
                                "x-server-id": 'xxx', "x-object-path":'a/c/o'})
            resp = req.get_response(self.object_controller)
            self.assertEquals(resp.status_int, 200)
            self.assertTrue(osd.objectService.server.RecoveryHandler.triggered_recovery.called) 
            osd.objectService.server.RecoveryHandler.triggered_recovery.assert_called_once_with('a/c/o', 'PUT', '1.2.3:000')

if __name__ == '__main__':
    unittest.main()
