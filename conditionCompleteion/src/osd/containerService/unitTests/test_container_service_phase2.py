"""Tests for src.containerService.server"""
import sys
sys.path.append("../../../../bin/release_prod_64")
sys.path.append("../../../../src")
sys.path.append("../../../../scripts")
import os
import pickle
homePath = os.environ["HOME"]
os.environ["OBJECT_STORAGE_CONFIG"] = homePath + "/osd_config"
from StringIO import StringIO
import shutil
import contextlib
import time
import unittest
import mock
import httplib
from osd.containerService.utils import AsyncLibraryHelper
from osd.common.swob import Request, HTTPException, status_map
from osd.containerService.unitTests import FakeObjectRing, \
    FakeContainerRing, FakeAccountRing, debug_logger
from osd.containerService.server import ContainerController
from eventlet import greenpool
#from osd.containerService import server as container_server

from osd.containerService.unitTests import FakeTransferCompMessage, \
    FakeComponentList, FakeReq, FakeConnectTargetNode, FakeSendData, \
    FakeGetConnResp

def mock_get_global_map(*args, **kwargs):
    obj = mock.Mock()
    obj.get_id.return_value = 'HN0101_61014_container-server'
    global_map = {'x1':'1,2,3', 'container-server': (obj,1.4)}
    global_version = float(1.1)
    conn_obj = 1
    return global_map, global_version, conn_obj

def my_components_from_map_mock(*args, **kwargs):
    return [1,2,3]




EXPORT = os.getcwd() + '/export'
cont_jour = EXPORT + '/hydra042_61014_container_journal'
trans_jour = EXPORT +'/hydra042_61014_transaction_journal'
if not os.path.exists(cont_jour):
    os.makedirs(cont_jour)
if not os.path.exists(trans_jour):
    os.makedirs(trans_jour)
conf = {'bind_port' : 6001, 'mount_check': 'False', 'filesystems' : EXPORT,
        'child_port' : 6101,    'journal_path_cont' : cont_jour,
        'journal_path_trans' : trans_jour, 'PID_PATH' : EXPORT}
logger = debug_logger()

def fs_mount_fake(*args,**kwargs):
    return 0

def fake_ownership(*args,**kwargs):
    return [1,2,4]

def mock_get_global_map(*args, **kwargs):
    global_map = {'x1':'1,2,3', 'container-server': '9'}
    global_version = float(1.1)
    conn_obj = 1
    return global_map, global_version, conn_obj


def health_monitor_fake(*args,**kwargs):
    pass

def create_proc(*args, **kwargs):
    pass


def trans_mock(*args):
    pass

def fake_object_gl_version(*args,**kwargs):
    return 1

with mock.patch("osd.containerService.server.osd_utils.mount_filesystem", fs_mount_fake):
        with mock.patch("osd.containerService.server.get_global_map", mock_get_global_map):
            with mock.patch("osd.containerService.server.healthMonitoring", health_monitor_fake):
                with mock.patch("osd.containerService.server.ContainerController.get_my_ownership", fake_ownership):
                    with mock.patch("osd.containerService.server.ContainerController.create_process", create_proc):
                        with mock.patch("osd.containerService.server.ContainerController.get_object_gl_version", fake_object_gl_version):
                            cont_obj = ContainerController(conf, logger)


def mock_http_release(response):
    """ mock function for release_lock """
    class FakeConn(object):
        """ Fake class for HTTP Connection """
        def __init__(self, status):
            """ initialize function """
            self.status = status
            self.reason = 'Fake'
            self.host = '1.2.3.4'
            self.port = '1234'

        def getresponse(self):
            """ return response """
            return self

        def read(self):
            """ read response """
            return ''

    return lambda *args, **kwargs: FakeConn(response)


class TestContainerOperationManager(unittest.TestCase):
    """ Test class for ContainerController """

    @mock.patch("osd.containerService.recovery_handler.ObjectRing", FakeObjectRing)
    @mock.patch("osd.containerService.recovery_handler.ContainerRing", FakeContainerRing)
    def setUp(self):
        sys.path.append("../../../../bin/release_prod_64")
        self.__pool = greenpool.GreenPool(2)
        self.__pool.spawn_n(cont_obj.add_green_thread)
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        self.req = Request.blank(
                '/fs0/accdir/contdir/pramita/container1',
                environ={'REQUEST_METHOD': 'PUT', 'HTTP_X_COMPONENT_NUMBER': 4},
                headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                         'Content-Length': '6', 'X-Size' : 97, 'X-GLOBAL-MAP-VERSION': 4.0 , 'x-component-number': 4,
                         'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu', 'X-OBJECT-GL-VERSION-ID': 0})


    def tearDown(self):
        if os.path.exists(EXPORT):
            shutil.rmtree(EXPORT)
        UPDATER = os.getcwd() + '/updater'
        if os.path.exists(UPDATER):
            shutil.rmtree(UPDATER)

    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_01_accept_component_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        #accept_cont_data_mock = mock.Mock(spec = ContainerOperationManager)
        #accept_cont_data_mock.accept_components_cont_data.return_value = 9
        with mock.patch('osd.containerService.server.ContainerOperationManager.accept_components_cont_data') as mock_ins:
            mock_ins.return_value = 400
            resp = cont_obj.ACCEPT_COMPONENT_CONT_DATA(self.req)
            self.assertEqual(resp.status_int, 200)


    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_02_in_EOF_Error_exception_accept_component_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        #self.req.environ['wsgi.input'] = StringIO(string_pickle)
        #accept_cont_data_mock = mock.Mock(spec = ContainerOperationManager)
        #accept_cont_data_mock.accept_components_cont_data.return_value = 9
        with mock.patch('osd.containerService.server.ContainerOperationManager.accept_components_cont_data') as mock_ins:
            mock_ins.return_value = 400
            #mock_ins.side_effect = TypeError()
            #mock_ins.side_effect = ['Exception']
            #resp = cont_obj.ACCEPT_COMPONENT_CONT_DATA(self.req)
            self.assertRaises(Exception, cont_obj.ACCEPT_COMPONENT_CONT_DATA(self.req))




    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_03_accept_component_trans_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        #accept_cont_data_mock = mock.Mock(spec = ContainerOperationManager)
        #accept_cont_data_mock.accept_components_cont_data.return_value = 9
        with mock.patch('osd.containerService.server.TransactionManager.accept_components_trans_data') as mock_ins:
            mock_ins.return_value = 302
            resp = cont_obj.ACCEPT_COMPONENT_TRANS_DATA(self.req)
            self.assertEqual(resp.status_int, 200)


    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_04_in_EOF_Error_exception_accept_component_trans_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        #self.req.environ['wsgi.input'] = StringIO(string_pickle)
        #accept_cont_data_mock = mock.Mock(spec = ContainerOperationManager)
        #accept_cont_data_mock.accept_components_cont_data.return_value = 9
        with mock.patch('osd.containerService.server.TransactionManager.accept_components_trans_data') as mock_ins:
            mock_ins.return_value = 400
            #mock_ins.side_effect = TypeError()
            #mock_ins.side_effect = ['Exception']
            #resp = cont_obj.ACCEPT_COMPONENT_CONT_DATA(self.req)
            self.assertRaises(Exception, cont_obj.ACCEPT_COMPONENT_TRANS_DATA(self.req))


    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_05_in_EOF_Error_exception_accept_component_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO('VERIFY')
        with mock.patch('osd.containerService.server.ContainerOperationManager.accept_components_cont_data') as mock_ins:
            mock_ins.return_value = 400
            self.assertRaises(Exception, cont_obj.ACCEPT_COMPONENT_CONT_DATA(self.req))


    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_06_exception_accept_component_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = string_pickle
        with mock.patch('osd.containerService.server.ContainerOperationManager.accept_components_cont_data') as mock_ins:
            mock_ins.return_value = 400
            self.assertRaises(Exception, cont_obj.ACCEPT_COMPONENT_CONT_DATA(self.req))



    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_07_in_EOF_Error_exception_accept_component_trans(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO('VERIFY')
        with mock.patch('osd.containerService.server.TransactionManager.accept_components_trans_data') as mock_ins:
            mock_ins.return_value = 400
            self.assertRaises(Exception, cont_obj.ACCEPT_COMPONENT_TRANS_DATA(self.req))






    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_08_exception_accept_component_trans_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = string_pickle
        with mock.patch('osd.containerService.server.TransactionManager.accept_components_trans_data') as mock_ins:
            mock_ins.return_value = 400
            self.assertRaises(Exception, cont_obj.ACCEPT_COMPONENT_TRANS_DATA(self.req))








    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_09_accept_component_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        with mock.patch('osd.containerService.server.ContainerOperationManager.accept_components_cont_data') as mock_ins:
            mock_ins.return_value = 500
            resp = cont_obj.ACCEPT_COMPONENT_CONT_DATA(self.req)
            self.assertEqual(resp.status_int, 500)



    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_10_accept_component_trans_data(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        with mock.patch('osd.containerService.server.TransactionManager.accept_components_trans_data') as mock_ins:
            mock_ins.return_value = 500
            resp = cont_obj.ACCEPT_COMPONENT_TRANS_DATA(self.req)
            self.assertEqual(resp.status_int, 500)




    def test_11_check_status(self):
        #response  = cont_obj.__check_status(library_status, req)
        response  = cont_obj._ContainerController__check_status(10, self.req)
        self.assertEqual(None , response)

        response  = cont_obj._ContainerController__check_status(20, self.req)
        self.assertEqual(status_map[404]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(30, self.req)
        self.assertEqual(None, response)

        response  = cont_obj._ContainerController__check_status(40, self.req)
        self.assertEqual(status_map[500]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(50, self.req)
        self.assertEqual(status_map[500]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(60, self.req)
        self.assertEqual(None, response)

        response  = cont_obj._ContainerController__check_status(70, self.req)
        self.assertEqual(status_map[400]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(71, self.req)
        self.assertEqual(status_map[400]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(73, self.req)
        self.assertEqual(status_map[400]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(75, self.req)
        self.assertEqual(status_map[400]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(77, self.req)
        self.assertEqual(status_map[500]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(100, self.req)
        self.assertEqual(None, response)

        response  = cont_obj._ContainerController__check_status(200, self.req)
        self.assertEqual(status_map[202]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(202, self.req)
        self.assertEqual(status_map[200]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(402, self.req)
        self.assertEqual(status_map[200]().status_int, response.status_int)

        response  = cont_obj._ContainerController__check_status(92, self.req)
        self.assertEqual(status_map[500]().status_int, response.status_int)

    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_12_stop_service(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        with mock.patch('osd.containerService.server.ContainerOperationManager.flush_all_data') as mock_ins:
            mock_ins.return_value = 202
            resp = cont_obj.STOP_SERVICE(self.req)
            self.assertEqual(resp.status_int, 200)

    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_13_exception_stop_service(self):
        self.req.body = 'VERIFY'
        string_pickle = pickle.dumps('VERIFY')
        self.req.headers['Content-Length'] = 14
        self.req.environ['wsgi.input'] = StringIO(string_pickle)
        with mock.patch('osd.containerService.server.ContainerOperationManager.flush_all_data') as mock_ins:
            mock_ins.side_effect = TypeError
            #resp = cont_obj.STOP_SERVICE(self.req)
            #self.assertEqual(resp.status_int, 200)
            self.assertRaises(TypeError, cont_obj.STOP_SERVICE(self.req))
            self.assertEqual(cont_obj.STOP_SERVICE(self.req).status_int, 500)



    @mock.patch('osd.containerService.server.ContainerOperationManager.commit_recovered_data', trans_mock)
    @mock.patch('osd.containerService.server.TransactionManager.commit_recovered_data', trans_mock)
    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    @mock.patch('osd.containerService.server.ContainerController._ContainerController__get_my_components_from_map', my_components_from_map_mock)
    def test_14_call_method(self):
        outbuf = StringIO()
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)
        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'GET',
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
                          'HTTP_X_GLOBAL_MAP_VERSION': 1.4,
                          'HTTP_X_COMPONENT_NUMBER': 5
                           },
                           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '404 ')


    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    def test_15_call_method_precondition_fail(self):
        outbuf = StringIO()
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)
        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'GET',
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
                          'HTTP_X_COMPONENT_NUMBER': 5},
                           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '412 ')



    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    def test_16_call_method_precondition_fail_empty_path(self):
        outbuf = StringIO()
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)
        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'GET',
                          'SCRIPT_NAME': '',
                          'PATH_INFO': '',
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
                          'HTTP_X_GLOBAL_MAP_VERSION': 1.4,
                          'HTTP_X_COMPONENT_NUMBER': 5},
                           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '412 ')



    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    def test_17_call_method_precondition_fail(self):
        outbuf = StringIO()
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)
        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'unknown_method',
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
                          'HTTP_X_COMPONENT_NUMBER': 5},
                           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '412 ')


    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    def test_18_call_method_unknown_method(self):
        outbuf = StringIO()
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)
        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'unknown_method',
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
                          'HTTP_X_GLOBAL_MAP_VERSION': 1.4,
                          'HTTP_X_COMPONENT_NUMBER': 5},
                           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '405 ')



    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    def test_19_call_method_permanent_moved(self):
        outbuf = StringIO()
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)
        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'GET',
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
                          'HTTP_X_GLOBAL_MAP_VERSION': 1.0,
                          'HTTP_X_COMPONENT_NUMBER': 5},
                           start_response)
        self.assertEquals(errbuf.getvalue(), '') 
        self.assertEquals(outbuf.getvalue()[:4], '301 ')


    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    def test_20_call_method_temporary_moved(self):
        outbuf = StringIO()
        cont_obj._ContainerController__blocked_component_list = [5]
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'GET',
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
                          'HTTP_X_GLOBAL_MAP_VERSION': 1.1,
                          'HTTP_X_COMPONENT_NUMBER': 5},
                           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '307 ')

    @mock.patch('osd.containerService.server.ContainerOperationManager.commit_recovered_data', trans_mock)
    @mock.patch('osd.containerService.server.TransactionManager.commit_recovered_data', trans_mock)
    @mock.patch('osd.containerService.server.get_global_map',mock_get_global_map)
    @mock.patch('osd.containerService.server.ContainerController._ContainerController__get_my_components_from_map', my_components_from_map_mock)
    def test_21_call_method_temporary_moved(self):
        outbuf = StringIO()
        cont_obj._ContainerController__blocked_component_list = [5]
        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        inbuf = StringIO()
        errbuf = StringIO()

        def start_response(*args):
            """Sends args to outbuf"""
            outbuf.writelines(args)

        #self.object_controller.node_delete = True
        cont_obj.__call__({'REQUEST_METHOD': 'GET',
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
                          'HTTP_X_GLOBAL_MAP_VERSION': 4.0,
                          'HTTP_X_COMPONENT_NUMBER': 2},
                           start_response)
        self.assertEquals(errbuf.getvalue(), '')
        self.assertEquals(outbuf.getvalue()[:4], '404 ')

    @mock.patch('osd.containerService.server.TransferCompMessage', FakeTransferCompMessage)
    @mock.patch('osd.containerService.server.libContainerLib.list_less__component_names__greater_', FakeComponentList)
    @mock.patch('osd.containerService.server.Req',FakeReq)
    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_22_transfer_components(self):
        print "Test 22 TRANSFER COMPONENTS"
        with mock.patch('osd.containerService.server.ContainerOperationManager.extract_container_data') as mock_ins:
            mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
            with mock.patch('osd.containerService.server.TransactionManager.extract_transaction_data') as mock_ins:
                mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
                with mock.patch('osd.containerService.server.ContainerController._connect_target_node') as mock_ins:
                    conn = httplib.HTTPSConnection("1.1.1.1", 7890)
                    conn.node = {}
                    conn.node['ip'] = "1.1.1.1"
                    conn.node['port'] = 7890
                    mock_ins.return_value = conn
                    with mock.patch("osd.containerService.server.ContainerController._ContainerController__send_data", FakeSendData):
                        with mock.patch("osd.containerService.server.ContainerController.get_conn_response", FakeGetConnResp):
                            resp = cont_obj.TRANSFER_COMPONENTS(self.req)

    @mock.patch('osd.containerService.server.TransferCompMessage', FakeTransferCompMessage)
    @mock.patch('osd.containerService.server.libContainerLib.list_less__component_names__greater_', FakeComponentList)
    @mock.patch('osd.containerService.server.Req',FakeReq)
    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_23_transfer_components(self):
        print "Test 23 TRANSFER COMPONENTS"
        with mock.patch('osd.containerService.server.ContainerOperationManager.extract_container_data') as mock_ins:
            mock_ins.return_value = {}
            with mock.patch('osd.containerService.server.TransactionManager.extract_transaction_data') as mock_ins:
                mock_ins.return_value = {}
                resp = cont_obj.TRANSFER_COMPONENTS(self.req)

    @mock.patch('osd.containerService.server.TransferCompMessage', FakeTransferCompMessage)
    @mock.patch('osd.containerService.server.libContainerLib.list_less__component_names__greater_', FakeComponentList)
    @mock.patch('osd.containerService.server.Req',FakeReq)
    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_24_transfer_components(self):
        print "Test 24 TRANSFER COMPONENTS"
        with mock.patch('osd.containerService.server.ContainerOperationManager.extract_container_data') as mock_ins:
            mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
            with mock.patch('osd.containerService.server.TransactionManager.extract_transaction_data') as mock_ins:
                mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
                with mock.patch('osd.containerService.server.ContainerController._connect_target_node') as mock_ins:
                    conn = httplib.HTTPSConnection("2.2.2.2", 8000)
                    conn.node = {}
                    conn.node['ip'] = "2.2.2.2"
                    conn.node['port'] = 6570
                    mock_ins.return_value = conn
                    with mock.patch("osd.containerService.server.ContainerController._ContainerController__send_data", FakeSendData):
                        with mock.patch('osd.containerService.server.ContainerController.get_conn_response') as mock_ins:
                            conn.resp = False
                            conn.status = False
                            mock_ins.return_value = (conn.resp, conn)
                            resp = cont_obj.TRANSFER_COMPONENTS(self.req)

    @mock.patch('osd.containerService.server.TransferCompMessage', FakeTransferCompMessage)
    @mock.patch('osd.containerService.server.libContainerLib.list_less__component_names__greater_', FakeComponentList)
    @mock.patch('osd.containerService.server.Req',FakeReq)
    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_25_transfer_components(self):
        print "Test 25 TRANSFER COMPONENTS"
        with mock.patch('osd.containerService.server.ContainerOperationManager.extract_container_data') as mock_ins:
            mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
            with mock.patch('osd.containerService.server.TransactionManager.extract_transaction_data') as mock_ins:
                mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
                with mock.patch('osd.containerService.server.ContainerController._connect_target_node') as mock_ins:
                    conn = httplib.HTTPSConnection("2.2.2.2", 8000)
                    conn.node = {}
                    conn.node['ip'] = "2.2.2.2"
                    conn.node['port'] = 8000 #Invalid port Exception to be raised
                    mock_ins.return_value = conn
                    with mock.patch("osd.containerService.server.ContainerController._ContainerController__send_data", FakeSendData):
                        with mock.patch('osd.containerService.server.ContainerController.get_conn_response') as mock_ins:
                            conn.resp = False
                            conn.status = False
                            mock_ins.return_value = (conn.resp, conn)
                            resp = cont_obj.TRANSFER_COMPONENTS(self.req)

    '''
    @mock.patch('osd.containerService.server.TransferCompMessage', FakeTransferCompMessage)
    @mock.patch('osd.containerService.server.libContainerLib.list_less__component_names__greater_', FakeComponentList)
    @mock.patch('osd.containerService.server.Req',FakeReq)
    @mock.patch("osd.containerService.server.http_connect", mock_http_release(200))
    def test_26_transfer_components(self):
        print "Test 26 TRANSFER COMPONENTS"
        with mock.patch('osd.containerService.server.ContainerOperationManager.extract_container_data') as mock_ins:
            mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
            with mock.patch('osd.containerService.server.TransactionManager.extract_transaction_data') as mock_ins:
                mock_ins.return_value = {'ABC' : 20, 'PQR' : 40, 'XYZ' : 30}
                with mock.patch('osd.containerService.server.ContainerController._connect_target_node') as mock_ins:
                    conn = httplib.HTTPSConnection("2.2.2.2", 8000)
                    conn.node = {}
                    conn.node['ip'] = "1.1.1.1"
                    conn.node['port'] = 7890
                    mock_ins.return_value = conn
                    with mock.patch('osd.containerService.server.ContainerController._ContainerController__send_data', FakeSendData):
                        with mock.patch('osd.containerService.server.ContainerController.get_conn_response') as mock_ins:
                            #conn.getresponse() = True
                            conn.status = True
                            conn.target_service = "%s:%s"% ("1.1.1.1", str(7890))
                            #conn.etresponse().status = 200
                            mock_ins.return_value = (mock_http_release(200), conn)
                            resp = cont_obj.TRANSFER_COMPONENTS(self.req)
    '''


if __name__ == '__main__':
    unittest.main()
