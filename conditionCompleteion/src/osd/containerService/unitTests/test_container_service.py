"""Tests for src.containerService.server"""
import sys
sys.path.append("../../../../bin/release_prod_64")
sys.path.append("../../../../src")
sys.path.append("../../../../scripts")
import os
from osd.common.swob import HTTPConflict

homePath = os.environ["HOME"]

os.environ["OBJECT_STORAGE_CONFIG"] = homePath + "/osd_config"
import shutil
import contextlib
import time
import unittest
import mock
from osd.containerService.utils import AsyncLibraryHelper
from osd.common.swob import Request, HTTPException
from osd.containerService.unitTests import FakeObjectRing, \
    FakeContainerRing, FakeAccountRing, debug_logger
from osd.containerService.server import ContainerController
from osd.containerService.recovery_handler import recover_conflict_expired_id
from eventlet import greenpool

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
    global_map = {'x1':'1,2,3'}
    global_version = float(1.1)
    conn_obj = 1
    return global_map, global_version, conn_obj


def health_monitor_fake(*args,**kwargs):
    pass

def create_proc(*args, **kwargs):
    pass

def fake_object_gl_version(*args,**kwargs):
    return 1

def conn_obj_mock(*args, **kwargs):
    pass


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

    def tearDown(self):
        if os.path.exists(EXPORT):
            shutil.rmtree(EXPORT)
        UPDATER = os.getcwd() + '/updater'
        if os.path.exists(UPDATER):
            shutil.rmtree(UPDATER)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(201))
    def test_01_put_container_created(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "-------------------------------------------1--------------------------------------------------"
        req = Request.blank(
                '/fs0/accdir/contdir/pramita/container1',
                environ={'REQUEST_METHOD': 'PUT', 'HTTP_X_COMPONENT_NUMBER': 4},
                headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                         'Content-Length': '6', 'X-Size' : 97,'X-GLOBAL-MAP-VERSION': 4.0 , 'x-component-number': 4,
                         'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu','X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        #print "------------------finish---------------"
        self.assertEqual(resp.status_int, 201)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(201))
    def test_02_put_container_accepted(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "-------------------------------------------2-------------------------------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 202)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_03_put_container_update_None(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "--------------3-----------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        #print "--------------finish 3-----------"
        self.assertEqual(resp.status_int, 201)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(404))
    def test_04_put_container_not_found(self):
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 404)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(403))
    def test_04_put_container_forbidden(self):
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 403)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_05_put_container_metadata(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "----------5----------------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'X-Container-Meta-subject' : 'abc', 'X-Container-read' : 'True',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)


    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_06_put_object_created(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "----------6----------------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        req1 = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97,'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req1.body = 'VERIFY'
        resp = cont_obj.PUT(req1)
        self.assertEqual(resp.status_int, 201)


    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_07_container_delete(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "----------7---------------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'DELETE'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        resp = cont_obj.DELETE(req)
        self.assertEqual(resp.status_int, 204)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_08_container_delete_not_found(self):
        #print "---------------------8----------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'DELETE'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.DELETE(req)
        self.assertEqual(resp.status_int, 404)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_09_object_delete(self):

        #print "---------------------9---------------------------"
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'PUT'},
            headers={'x-timestamp': '1408505144.00290', 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'text/plain', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'DELETE'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.DELETE(req)
        self.assertEqual(resp.status_int, 204)


    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_10_container_get(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)

        #print "---------------------10---------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'GET'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        resp = cont_obj.GET(req)
        self.assertEqual(resp.status_int, 204)

    def test_11_container_get_not_found(self):

        #print "---------------------11---------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'GET'},
            headers={'x-timestamp': time.time(),
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.GET(req)
        self.assertEqual(resp.status_int, 404)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_12_container_post(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "---------------------12---------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'POST'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        resp = cont_obj.POST(req)
        self.assertEqual(resp.status_int, 204)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_13_container_post_metadata(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "---------------------13---------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'POST'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'X-Container-Meta-subject' : 'abc', 'X-Container-read' : 'True',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        resp = cont_obj.POST(req)
        self.assertEqual(resp.status_int, 204)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_14_container_post_metadata_acl_sysmeta(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "---------------------14---------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'POST'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'X-Container-SysMeta-subject' : 'abc', 'X-Container-write' : 'True',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        resp = cont_obj.POST(req)
        self.assertEqual(resp.status_int, 204)

    def test_15_container_post_not_found(self):
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'POST'},
            headers={'x-timestamp': time.time(),
                'X-Container-Meta-subject' : 'abc', 'X-Container-read' : 'True',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.POST(req)
        self.assertEqual(resp.status_int, 404)

    @mock.patch("osd.containerService.server.http_connect", \
        mock_http_release(None))
    def test_16_container_head(self):
        if not os.path.exists(cont_jour):
            os.makedirs(cont_jour)
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'HEAD'},
            headers={'x-timestamp': time.time(), 'X-Account-Host' : '127.0.0.1:61005',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu', 'X-Account-Component-Number': 4})
        req.body = 'VERIFY'
        resp = cont_obj.PUT(req)
        self.assertEqual(resp.status_int, 201)
        resp = cont_obj.HEAD(req)
        self.assertEqual(resp.status_int, 204)

    def test_17_container_head_not_found(self):

        #print "---------------------17-------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1', environ={'REQUEST_METHOD': 'HEAD'},
            headers={'x-timestamp': time.time(),
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.HEAD(req)
        self.assertEqual(resp.status_int, 404)

    def test_18_get_lock_error(self):
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "---------------------18------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'GET_LOCK'},
            headers={'x-timestamp': time.time(), 'x-server-id' : 'egegeq',
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'x-operation' : 'PUT',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.GET_LOCK(req)
        self.assertEqual(resp.status_int, 409)

    def test_19_release_lock(self):
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)

        #print "---------------------19------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'GET_LOCK'},
            headers={'x-timestamp': time.time(), 'x-server-id' : 'egegeq',
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'x-operation' : 'PUT', 'x-mode' : 'w',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.GET_LOCK(req)
        self.assertEqual(resp.status_int, 200)
        trans_id = resp.headers.get('x-request-trans-id')
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'FREE_LOCK'},
            headers={'x-timestamp': time.time(), 'x-request-trans-id' : trans_id,
                'Content-Length': '6', 'X-Size' : 97,
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.FREE_LOCK(req)
        self.assertEqual(resp.status_int, 200)



    def test_20_release_lock_error(self):
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)

        #print "---------------------20------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'FREE_LOCK'},
            headers={'x-timestamp': time.time(),
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.FREE_LOCK(req)
        self.assertEqual(resp.status_int, 409)

    def test_21_release_lock_internal_error(self):
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)

        #print "---------------------21------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'FREE_LOCK'},
            headers={'x-timestamp': time.time(), 'x-request-trans-id' : '88988',
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.FREE_LOCK(req)
        self.assertEqual(resp.status_int, 503)


    def test_22_release_locks(self):
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)

        #print "---------------------22------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'GET_LOCK'},
            headers={'x-timestamp': time.time(), 'x-server-id' : 'egegeq',
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'x-operation' : 'PUT', 'x-mode' : 'w',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.GET_LOCK(req)
        self.assertEqual(resp.status_int, 200)
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'FREE_LOCKS'},
            headers={'x-timestamp': time.time(),
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.FREE_LOCKS(req)
        self.assertEqual(resp.status_int, 200)

    def test_23_release_locks_error(self):
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)

        #print "---------------------23------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'FREE_LOCKS'},
            headers={'x-timestamp': time.time(),
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.FREE_LOCKS(req)
        self.assertEqual(resp.status_int, 409)

    def test_24_release_locks_internal_error(self):
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        #print "---------------------24------------------------"
        req = Request.blank(
            '/fs0/accdir/contdir/pramita/container1/object1', environ={'REQUEST_METHOD': 'FREE_LOCKS'},
            headers={'x-timestamp': time.time(), 
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.FREE_LOCKS(req)
        self.assertEqual(resp.status_int, 503)

    @mock.patch('osd.containerService.recovery_handler.conn_for_obj', conn_obj_mock)
    def test_25_test_recover_conflict_id(self):
        global_map_version = 4.0
        operation = 'PUT'
        full_path= '/'
        obj_serv_id = 'HN0101_61014_container-server'
        trans_id = 'dummy_trans_id'
        object_map= {}
        trans_list = [(trans_id, full_path, obj_serv_id, operation)] 
        inner_dict = {'x-server-id' : obj_serv_id,
             'x-object-path' : full_path, 'x-operation' : operation,
             'x-request-trans-id' : trans_id, 'x-global-map-version' : global_map_version}
        req = Request.blank(
            '/fs0/accdir/contdir/jaivish/container1/object1', environ={'REQUEST_METHOD': 'RECOVER'},
            headers=inner_dict)
        recover_conflict_expired_id(req, trans_list, 5, 10, logger, object_map, obj_serv_id, global_map_version)

    """
    def test_26_get_lock_error(self):
        osd.containerService.server.trans_mgr.get_lock = mock.MagicMock()
        osd.containerService.server.trans_mgr.get_lock.side_effect= HTTPConflict(request={})
        if not os.path.exists(trans_jour):
            os.makedirs(trans_jour)
        req = Request.blank(
            '/fs0/accdir/contdir/jaivish/container1/object1', environ={'REQUEST_METHOD': 'GET_LOCK'},
            headers={'x-timestamp': time.time(), 'x-server-id' : 'egegeq',
                'x-object-path' : '4144e20bbf5c7f7aea749590b30d4fc4/32c31b3327b4870a5d7fc8c6f62cea71/4144e20bbf5c7f7aea749590b30d4fc4',
                'x-operation' : 'PUT',
                'Content-Length': '6', 'X-Size' : 97, 'x-component-number': 4,
                'X-Content-Type' : 'application', 'X-etag' : 'cxwecuwcbwiu'})
        req.body = 'VERIFY'
        resp = cont_obj.GET_LOCK(req)
        self.assertEqual(resp.status_int, 409)
    """





if __name__ == '__main__':
    unittest.main()
