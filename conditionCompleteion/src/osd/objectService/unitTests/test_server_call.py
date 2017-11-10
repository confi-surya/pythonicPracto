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
from osd.common.utils import mkdirs, normalize_timestamp, \
    NullLogger, storage_directory, public, replication
from osd.common import constraints
from osd.common.swob import Request, HeaderKeyDict
from osd.common.exceptions import DiskFileDeviceUnavailable
from osd.common.ring.ring import hash_path


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
            self.headers = {'x-request-trans-id': "123"}
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
@mock.patch("osd.objectService.objectutils.TranscationManager.acquire_lock", mock_acquire_lock(200, {'x-request-trans-id': "123"}))
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
        mkdirs(os.path.join(self.filesystems, self.filesystem,"a", "c", self.dir, "data"))
        mkdirs(os.path.join(self.filesystems, self.filesystem, "a", "c", self.dir, "meta"))
        conf = {'filesystems': self.filesystems, 'mount_check': 'false','llport' : 61014}
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


    
    def test_call_bad_request(self):
        inbuf = StringIO()
        errbuf = StringIO()
        outbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        self.object_controller.__call__({'REQUEST_METHOD': 'PUT',
                                         'SCRIPT_NAME': '',
                                         'PATH_INFO': '/fs1/a1/d1/o1/a/c/o',
                                         'SERVER_NAME': '127.0.0.1',
                                         'SERVER_PORT': '61005',
                                         'SERVER_PROTOCOL': 'HTTP/1.0',
                                         'CONTENT_LENGTH': '0',
                                         'wsgi.version': (1, 0),
                                         'wsgi.url_scheme': 'http',
                                         'wsgi.input': inbuf,
                                         'wsgi.errors': errbuf,
                                         'wsgi.multithread': False,
                                         'wsgi.multiprocess': False,
                                         'wsgi.run_once': False,
                                         'HTTP_X_GLOBAL_MAP_VERSION': 1.4},
                                         start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '400 ')


    def test_call_not_found(self):
        inbuf = StringIO()
        errbuf = StringIO()
        outbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        self.object_controller.__call__({'REQUEST_METHOD': 'GET',
                                         'SCRIPT_NAME': '',
                                         'PATH_INFO': '/fs1/a1/d1/o1/a/c/o',
                                         'SERVER_NAME': '127.0.0.1',
                                         'SERVER_PORT': '61005',
                                         'SERVER_PROTOCOL': 'HTTP/1.0',
                                         'CONTENT_LENGTH': '0',
                                         'HTTP_X_GLOBAL_MAP_VERSION': 1.4,
                                         'wsgi.version': (1, 0),
                                         'wsgi.url_scheme': 'http',
                                         'wsgi.input': inbuf,
                                         'wsgi.errors': errbuf,
                                         'wsgi.multithread': False,
                                         'wsgi.multiprocess': False,
                                         'wsgi.run_once': False},
                                        start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '404 ')


    def test_no_map(self):
        inbuf = StringIO()
        errbuf = StringIO()
        outbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        self.object_controller.__call__({'REQUEST_METHOD': 'GET',
                                         'SCRIPT_NAME': '',
                                         'PATH_INFO': '/fs1/a1/d1/o1/a/c/o',
                                         'SERVER_NAME': '127.0.0.1',
                                         'SERVER_PORT': '61005',
                                         'SERVER_PROTOCOL': 'HTTP/1.0',
                                         'CONTENT_LENGTH': '0',
                                         'wsgi.version': (1, 0),
                                         'wsgi.url_scheme': 'http',
                                         'wsgi.input': inbuf,
                                         'wsgi.errors': errbuf,
                                         'wsgi.multithread': False,
                                         'wsgi.multiprocess': False,
                                         'wsgi.run_once': False
                                         },
                                        start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '301 ')
    
    def test_node_delete(self):
        inbuf = StringIO()
        errbuf = StringIO()
        outbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)
        self.object_controller.node_delete = True
        self.object_controller.__call__({'REQUEST_METHOD': 'GET',
                                         'SCRIPT_NAME': '',
                                         'PATH_INFO': '/fs1/a1/d1/o1/a/c/o',
                                         'SERVER_NAME': '127.0.0.1',
                                         'SERVER_PORT': '61005',
                                         'SERVER_PROTOCOL': 'HTTP/1.0',
                                         'CONTENT_LENGTH': '0',
                                         'wsgi.version': (1, 0),
                                         'wsgi.url_scheme': 'http',
                                         'wsgi.input': inbuf,
                                         'wsgi.errors': errbuf,
                                         'wsgi.multithread': False,
                                         'wsgi.multiprocess': False,
                                         'wsgi.run_once': False
                                         },
                                        start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '301 ')


   
    def test_call_bad_method(self):
        inbuf = StringIO()
        errbuf = StringIO()
        outbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        self.object_controller.__call__({'REQUEST_METHOD': 'INVALID',
                                         'SCRIPT_NAME': '',
                                         'PATH_INFO': '/fs1/a1/d1/o1/a/c/o',
                                         'SERVER_NAME': '127.0.0.1',
                                         'SERVER_PORT': '61005',
                                         'SERVER_PROTOCOL': 'HTTP/1.0',
                                         'CONTENT_LENGTH': '0',
                                         'wsgi.version': (1, 0),
                                         'wsgi.url_scheme': 'http',
                                         'wsgi.input': inbuf,
                                         'wsgi.errors': errbuf,
                                         'wsgi.multithread': False,
                                         'wsgi.multiprocess': False,
                                         'wsgi.run_once': False,
                                         'HTTP_X_GLOBAL_MAP_VERSION': 1.4
                                         },
                                        start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '405 ')
    





    def test_invalid_method_doesnt_exist(self):
        errbuf = StringIO()
        outbuf = StringIO()

        def start_response(*args):
            outbuf.writelines(args)

        self.object_controller.__call__({
            'REQUEST_METHOD': 'method_doesnt_exist',
            'PATH_INFO': '/fs1/a1/d1/o1/a/c/o'},
           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '405 ')





    def test_invalid_method_is_not_public(self):
        errbuf = StringIO()
        outbuf = StringIO()

        def start_response(*args):
            outbuf.writelines(args)

        self.object_controller.__call__({'REQUEST_METHOD': '__init__',
                                         'PATH_INFO': '/fs1/a1/d1/o1/a/c/o',
                                         'HTTP_X_GLOBAL_MAP_VERSION': 1.4
                                         },
                                         start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '405 ')

if __name__ == '__main__':
    unittest.main()
