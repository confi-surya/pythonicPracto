"""Tests for osd.objectService.objectutils"""
import sys


from osd.objectService.objectutils import LockManager, TranscationManager
from osd.objectService.unitTests import debug_logger
import unittest
import mock

def mock_http_acquire(response, trans_id):
    """ mock function for acquire_lock """
    class FakeConn(object):
        """ Fake class for HTTP Connection """
        def __init__(self, status, trans_id):
            """ initialize function"""
            self.status = status
            self.reason = 'Fake'
            self.host = '1.2.3.4'
            self.port = '1234'
            self.body = trans_id

        def getresponse(self):
            """ return response """
            return self

        def getheaders(self):
            return [('x-request-trans-id', self.body)]

        def read(self):
            """ read response """
            return ''

    return lambda *args, **kwargs: FakeConn(response, trans_id)

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

class TestTranscationManager(unittest.TestCase):
    """ Test class for TranscationManager """
    def setUp(self):
        """
        Set up for testing osd.objectServer.objectutils.TranscationManager
        """
        self.logger = debug_logger()
        self.http_comm = TranscationManager(self.logger)

    @mock.patch("osd.objectService.objectutils.http_connect", \
        mock_http_acquire(201, '789012'))
    def test_http_connection_1(self):
        """ Method for http connection"""
        data = {'x-server-id' : '96880', 'x-object-path' : 'a/c/o', \
            'x-object_port' : '98', 'x-mode' : 'w'}

        host = '127.0.0.1:61005'
        resp = self.http_comm.acquire_lock(host, 'fs1', 'abc', data)

        self.assertEqual(resp.status, 201)
	trans_id = self.http_comm.get_transaction_id_from_resp(resp)
	self.assertEqual(trans_id, '789012')

    @mock.patch("osd.objectService.objectutils.http_connect", \
        mock_http_acquire(201, '-1'))
    def test_http_connection_2(self):
        """ Method for http connection """
        data = {'x-server-id' : '96880', 'x-object-path' : 'a/c/o', \
            'x-object_port' : '98', 'x-mode' : 'w'}

        host = '127.0.0.1:61005'
        resp = self.http_comm.acquire_lock(host, 'fs1', 'abc', data)

        self.assertEqual(resp.status, 201)
	trans_id = self.http_comm.get_transaction_id_from_resp(resp)
	self.assertEqual(trans_id, '-1')

    @mock.patch("osd.objectService.objectutils.http_connect", \
        mock_http_acquire(409, None))
    def test_http_connection_3(self):
        """ Method for http connection """
        data = {'x-server-id' : '96880', 'x-object-path' : 'a/c/o', \
            'x-object_port' : '98', 'x-mode' : 'w'}
        host = '127.0.0.1:61005'
        resp = self.http_comm.acquire_lock(host, 'fs1', 'abc', data)
        self.assertEqual(resp.status, 409)
	trans_id = self.http_comm.get_transaction_id_from_resp(resp)
	self.assertEqual(trans_id, None)

    @mock.patch("osd.objectService.objectutils.http_connect", \
        mock_http_release(404))
    def test_http_connection_4(self):
        """ Method for http connection """
        data = {'x-object-path' : 'a/c/o', \
                'x-request-trans-id' : '98897'}
        host = '127.0.0.1:61005'
        resp = self.http_comm.release_lock(host, 'fs1', 'abc', data)

        self.assertEqual(resp.status, 404)

    @mock.patch("osd.objectService.objectutils.http_connect", \
        mock_http_release(201))
    def test_http_connection_5(self):
        """ Method for http connection """
        data = {'x-object-path' : 'a/c/o', \
                'x-request-trans-id' : '907767'}
        host = '127.0.0.1:61005'
        resp = self.http_comm.release_lock(host, 'fs1', 'abc', data)

        self.assertEqual(resp.status, 201)

    @mock.patch("osd.objectService.objectutils.http_connect", \
        mock_http_acquire(503, None))
    def test_http_connection_6(self):
        """ Method for http connection """
        data = {'x-server-id' : '96880', 'x-object-path' : 'a/c/o', \
            'x-object_port' : '98', 'x-mode' : 'w'}
        host = '127.0.0.1:61005'
        resp = self.http_comm.acquire_lock(host, 'fs1', 'abc', data)
        self.assertEqual(resp.status, 503)
	trans_id = self.http_comm.get_transaction_id_from_resp(resp)
	self.assertEqual(trans_id, None)

class TestLockManager(unittest.TestCase):
    """ Test class for LockManager """
    def setUp(self):
        """
        Set up for testing osd.objectServer.objectutils
        """
        self.logger = debug_logger()
        self.lock_obj = LockManager(self.logger)
    
    def test_add_manager_1(self):
        """ test memory manager """
        full_path = 'ac1\con1\o1.txt'
        trans_id = '123456'
        self.assertEqual(self.lock_obj.add(full_path, trans_id), None)

    def test_add_manager_2(self):
        """ test memory manager """
        full_path = ''
        trans_id = '123456'
        self.assertRaises(ValueError, self.lock_obj.add, full_path, trans_id)

    def test_add_manager_3(self):
        """ test memory manager """
        full_path = 'ac1\con1\o1.txt'
        trans_id = ''
        self.assertRaises(ValueError, self.lock_obj.add, full_path, trans_id)

    def test_add_manager_4(self):
        """ test memory manager """
        full_path = ''
        trans_id = ''
        self.assertRaises(ValueError, self.lock_obj.add, full_path, trans_id)

    def test_isexists_1(self):
        """ test memory manager """
        full_path = 'ac1\con1\o2.txt'
        trans_id = '789012'
        self.assertEqual(self.lock_obj.add(full_path, trans_id), None)
        self.assertTrue(self.lock_obj.is_exists(full_path))

    def test_isexists_2(self):
        """ test memory manager """
        full_path = 'ac1\con1\o2.txt'
        self.assertFalse(self.lock_obj.is_exists(full_path))

    def test_isexists_3(self):
        """ test memory manager """
        full_path = ''
        self.assertFalse(self.lock_obj.is_exists(full_path))
    
    def test_delete_manager_1(self):
        """ test memory manager """
        full_path = 'ac1\con1\o2.txt'
        trans_id = '789012'
        self.assertEqual(self.lock_obj.add(full_path, trans_id), None)
        self.assertEqual(self.lock_obj.delete(full_path, trans_id), None)

    def test_delete_manager_2(self):
        """ test memory manager """
        full_path = 'ac1\con1\o2.txt'
        trans_id = '789012'
        trans_id_1 = '888888'
        self.assertEqual(self.lock_obj.add(full_path, trans_id), None)
        self.assertEqual(self.lock_obj.add(full_path, trans_id_1), None)
        self.assertEqual(self.lock_obj.delete(full_path, trans_id), None)
        self.assertTrue(self.lock_obj.is_exists(full_path))
        self.assertEqual(self.lock_obj.delete(full_path, trans_id_1), None)
        self.assertFalse(self.lock_obj.is_exists(full_path))
    
    def test_delete_manager_3(self):
        """ test memory manager """
        full_path = 'ac1\con1\o2.txt'
        trans_id = '789012'
        self.assertRaises(KeyError, self.lock_obj.delete, full_path, trans_id)

    def test_delete_manager_4(self):
        """ test memory manager """
        full_path = ''
        trans_id = ''
        self.assertRaises(KeyError, self.lock_obj.delete, full_path, trans_id)
    

if __name__ == '__main__':
    unittest.main()
