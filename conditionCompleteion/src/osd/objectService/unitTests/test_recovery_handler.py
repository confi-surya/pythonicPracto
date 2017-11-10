import unittest
import json, os
from time import time
import tempfile
import cPickle as pickle
import mock
from mock import MagicMock
from shutil import rmtree
from tempfile import mkdtemp
from hashlib import md5
from osd.objectService.unitTests import FakeLogger
from osd.objectService.unitTests import FakeObjectRing, FakeContainerRing

from osd.objectService.recovery_handler import RecoveryHandler, \
    DirectoryIterator, FileRecovery, CommunicateContainerService, \
    TriggeredRecoveryHandler
from osd.common.utils import normalize_timestamp
from osd.common.ring.ring import hash_path

#pylint: disable=E1101, C0103, W0612, W0212, W0613, W0621, E1120
ACCOUNT = 'test_account'
CONTAINER = 'test_container'
DATA_FILE_PREFIX = '.data'
META_FILE_PREFIX = '.meta'

def mock_http_connection(response):
    """ mock method for http_connection """
    class FakeConn(object):
        """ Fake class for HTTP Connection """
        def __init__(self, status):
            """ initialize function """
            self.status = status
            self.reason = 'Fake'
            self.host = '1.2.3.4'
            self.port = '0000'

        def getresponse(self):
            """ return response """
            return self

        def read(self):
            """ read response """
            return ''

    return lambda *args, **kwargs: FakeConn(response)

def mock_healthMonitoring(*args, **kwargs):
    print "2"
    return 1234


class TestRecoveryHandler(unittest.TestCase):
    @mock.patch('osd.objectService.recovery_handler.DirectoryIterator')
    def create_instance(self, DirectoryIterator):
        return RecoveryHandler('test_os_1', FakeLogger(), \
            object_ring=FakeObjectRing(''))
 
    @mock.patch('osd.objectService.recovery_handler.ContainerRing', FakeContainerRing)
    @mock.patch('osd.objectService.recovery_handler.ObjectRing', FakeObjectRing)
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.tmp_dir = os.path.join(self.test_dir, 'tmp')
        self.object_server_tmp_dir = os.path.join(self.tmp_dir, 'test_os_1')
        self.recover_handler = self.create_instance()

    def tearDown(self):
        rmtree(self.test_dir, ignore_errors=1) 

    def test_get_file_system_list(self): 
        self.assertEqual(['fs1', 'fs2'], self.recover_handler. \
            _get_file_system_list())

    def test_get_tmp_dir_list(self):
        with mock.patch('osd.objectService.recovery_handler.RecoveryHandler._get_file_system_list', return_value=['fs1', 'fs2']):
            tmp_dir_list = ['/export/fs1/tmp/test_os_1', '/export/fs2/tmp/test_os_1']
            self.assertEqual(tmp_dir_list, self.recover_handler._get_tmp_dir_list())
    
    @mock.patch('osd.objectService.recovery_handler.TMPDIR', 'tmp')
    def test_move_data(self):
        dir1 = os.path.join(self.tmp_dir, 'fs1', 'HN0101_object-server')
        dir2 = os.path.join(self.tmp_dir, 'fs2', 'HN0101_object-server')
        os.makedirs(dir1)
        os.makedirs(dir2)
        dir_list = [dir1, dir2]
        with mock.patch('osd.objectService.recovery_handler.RecoveryHandler._get_tmp_dir_list_to_move', return_value=dir_list):
            with mock.patch('osd.objectService.recovery_handler.EXPORT_PATH', self.tmp_dir):
                self.recover_handler._move_data()
                self.assertFalse(os.path.exists(dir1))
                self.assertFalse(os.path.exists(dir2))
                self.assertTrue(os.path.exists(os.path.join(self.tmp_dir, 'fs1', 'test_object-server')))
                self.assertTrue(os.path.exists(os.path.join(self.tmp_dir, 'fs2', 'test_object-server')))

    @mock.patch('osd.objectService.recovery_handler.TMPDIR', 'tmp')
    def test_move_data_with_files(self):
        dir1 = os.path.join(self.tmp_dir, 'fs1', 'HN0101_object-server')
        dir2 = os.path.join(self.tmp_dir, 'fs2', 'HN0101_object-server')
        os.makedirs(dir1)
        os.makedirs(dir2)
        dir_list = [dir1, dir2]
        open(os.path.join(dir1, 'a.txt'), 'w')
        with mock.patch('osd.objectService.recovery_handler.RecoveryHandler._get_tmp_dir_list_to_move', return_value=dir_list):
            with mock.patch('osd.objectService.recovery_handler.EXPORT_PATH', self.tmp_dir):
                self.recover_handler._move_data()
                self.assertFalse(os.path.exists(dir1))
                self.assertFalse(os.path.exists(dir2))
                self.assertTrue(os.path.exists(os.path.join(self.tmp_dir, 'fs1', 'test_object-server', 'a.txt')))
                self.assertTrue(os.path.exists(os.path.join(self.tmp_dir, 'fs2', 'test_object-server')))

    @mock.patch('osd.objectService.recovery_handler.TMPDIR', 'tmp')
    def test_get_tmp_dir_list_to_move(self):
        dir1 = os.path.join(self.tmp_dir, 'fs1', 'tmp', 'HN0101_object-server')
        dir2 = os.path.join(self.tmp_dir, 'fs2', 'tmp', 'HN0101_object-server')
        def create_dir():
            os.makedirs(dir1)
            os.makedirs(dir2)
        with mock.patch('osd.objectService.recovery_handler.EXPORT_PATH', self.tmp_dir):
            with mock.patch('osd.objectService.recovery_handler.RecoveryHandler._get_file_system_list', return_value=['fs1', 'fs2']):
                self.assertEquals([], self.recover_handler._get_tmp_dir_list_to_move())
                create_dir()
                self.assertEquals([dir1, dir2], self.recover_handler._get_tmp_dir_list_to_move())
            
       
    @mock.patch('osd.objectService.recovery_handler.create_recovery_file')
    @mock.patch('osd.objectService.recovery_handler.remove_recovery_file')
    @mock.patch('osd.objectService.recovery_handler.healthMonitoring', mock_healthMonitoring)
    def test_startup_recovery(self, create_recovery_file,
                                    remove_recovery_file):
        tmp_dir_list = ['/export/fs1/tmp/os_1', '/export/fs2/tmp/os_1', '/export/fs1/tmp/os_2', '/export/fs2/tmp/os_2']
        with mock.patch('osd.objectService.recovery_handler.RecoveryHandler._get_tmp_dir_list', return_value=tmp_dir_list):
            # test succesful case
            self.recover_handler.startup_recovery()
            self.recover_handler._RecoveryHandler__directory_iterator_obj. \
                recover.assert_any_call('/export/fs1/tmp/os_1')
            self.recover_handler._RecoveryHandler__directory_iterator_obj. \
                recover.assert_any_call('/export/fs2/tmp/os_1')
            self.recover_handler._RecoveryHandler__directory_iterator_obj. \
                recover.assert_any_call('/export/fs1/tmp/os_2')
            self.recover_handler._RecoveryHandler__directory_iterator_obj. \
                recover.assert_any_call('/export/fs2/tmp/os_2')
            self.assertTrue(self.recover_handler. \
                _RecoveryHandler__directory_iterator_obj.recover.called)
            self.assertEqual(self.recover_handler. \
                _RecoveryHandler__directory_iterator_obj.recover.call_count, 4)
            create_recovery_file.assert_called_once_with('object-server')
            #healthmonitor_mock_file.assert_called_once_with(None,None,None,None)
            remove_recovery_file.assert_called_once_with('object-server')

    @mock.patch('osd.objectService.recovery_handler.remove_recovery_file')
    @mock.patch('osd.objectService.recovery_handler.healthMonitoring', mock_healthMonitoring)
    def test_startup_recovery_create_recover_file_failed_case(self, remove_recovery_file):
        with mock.patch('osd.objectService.recovery_handler.create_recovery_file', side_effect=OSError):
            self.assertFalse(self.recover_handler.startup_recovery())
            self.assertFalse(remove_recovery_file.called)
    """ 
    @mock.patch('osd.objectService.recovery_handler.create_recovery_file')
    def test_startup_recovery_move_data_failure_case(self, create_recovery_file):
        with mock.patch('osd.objectService.recovery_handler.RecoveryHandler._move_data', side_effect=OSError):
            self.assertFalse(self.recover_handler.startup_recovery())
    """
    @mock.patch('osd.objectService.recovery_handler.create_recovery_file')
    def test_startup_recovery_remove_recover_file_failed_case(self, create_recovery_file):
        with mock.patch('osd.objectService.recovery_handler.remove_recovery_file', side_effect=OSError):
            self.assertFalse(self.recover_handler.startup_recovery())

    @mock.patch('osd.objectService.recovery_handler.create_recovery_file')
    @mock.patch('osd.objectService.recovery_handler.remove_recovery_file')
    @mock.patch('osd.objectService.recovery_handler.healthMonitoring', mock_healthMonitoring)
    def test_startup_recovery_exception_case(self, create_recovery_file,
                                             remove_recovery_file):
        with mock.patch('osd.objectService.recovery_handler.get_all_service_id', return_value=['os_1', 'os_2']):
            self.recover_handler._RecoveryHandler__directory_iterator_obj. \
            recover = MagicMock(side_effect=Exception)
            self.assertFalse(self.recover_handler.startup_recovery())
            create_recovery_file.assert_called_once_with('object-server')
            remove_recovery_file.assert_called_once_with('object-server')

    def test_triggered_recovery(self):
        self.recover_handler. \
            _RecoveryHandler__triggered_recovery_handler_obj.recover = \
            MagicMock()
        self.recover_handler.triggered_recovery('/a/c/o', 'PUT', '1.2.3:123')
        self.recover_handler. \
            _RecoveryHandler__triggered_recovery_handler_obj.recover. \
            assert_called_once_with('/a/c/o', 'PUT', '1.2.3:123')


            
        
    
class TestDirectoryIterator(unittest.TestCase):
    @mock.patch('osd.objectService.recovery_handler.FileRecovery')
    def create_instance(self, FileRecovery):
        return DirectoryIterator(FakeLogger())

    def create_test_file_only_data(self, path):
        # case 1 Only data file exist
        file_name = hash_path(ACCOUNT, CONTAINER, 'a.txt')
        data_file_path = os.path.join(path, file_name + DATA_FILE_PREFIX)
        with open(data_file_path, 'w') as data_fd:
            data_fd.write('test data file case 1')

    def create_test_file_data_and_empty_meta(self, path):
         # case 2 Both data file and empty meta file
        file_name = hash_path(ACCOUNT, CONTAINER, 'b.txt')
        data_file_path = os.path.join(path, file_name + DATA_FILE_PREFIX)
        meta_file_path = os.path.join(path, file_name + META_FILE_PREFIX)
        with open(data_file_path, 'w') as data_fd:
            data_fd.write('test data file case 2')
        fd = open(meta_file_path, 'w')
        fd.close()

    def create_test_file_data_and_meta(self, path):
        # case 3 Both data file and meta file 
        file_name = hash_path(ACCOUNT, CONTAINER, 'c.txt')
        data_file_path = os.path.join(path, file_name + DATA_FILE_PREFIX)
        meta_file_path = os.path.join(path, file_name + META_FILE_PREFIX)
        with open(data_file_path, 'w') as data_fd:
            data_fd.write('test data file case 3')
        with open(meta_file_path, 'w') as meta_fd:
            meta_fd.write('test meta file case 3')

    def create_test_file_only_meta(self, path):
        # case 4 Only meta file exist
        file_name = hash_path(ACCOUNT, CONTAINER, 'd.txt')
        meta_file_path = os.path.join(path, file_name + META_FILE_PREFIX)
        with open(meta_file_path, 'w') as meta_fd:
            meta_fd.write('test meta file case 4')

    def create_test_file_only_meta_empty(self, path):
        # case 5 Only empty meta file exist
        file_name = hash_path(ACCOUNT, CONTAINER, 'e.txt')
        meta_file_path = os.path.join(path, file_name + META_FILE_PREFIX)
        open(meta_file_path, 'w')

    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        rmtree(self.test_dir, ignore_errors=1)
        os.mkdir(self.test_dir)
        self.tmp_dir = os.path.join(self.test_dir, 'tmp')
        os.mkdir(self.tmp_dir)
        self.object_server_tmp_dir = os.path.join(self.tmp_dir, 'test_os_1')
        os.mkdir(self.object_server_tmp_dir)
        self.directory_iterator_obj = self.create_instance()

    def tearDown(self):
        rmtree(self.object_server_tmp_dir, ignore_errors=1)

    def test_process_dir(self):
        self.create_test_file_data_and_meta(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        data_list_expected = [hash_path(ACCOUNT, CONTAINER, 'c.txt') + '.data']
        meta_list_expected = [hash_path(ACCOUNT, CONTAINER, 'c.txt') + '.meta']
        self.assertEqual(data_list, data_list_expected)
        self.assertEqual(meta_list, meta_list_expected)

    def test_recover_empty_dir_case(self):
        self.directory_iterator_obj._recover_each_tmp_dir = MagicMock()
        self.directory_iterator_obj.recover('test')
        self.assertFalse(self.directory_iterator_obj. \
            _recover_each_tmp_dir.called)
        self.assertEqual(self.directory_iterator_obj. \
            _recover_each_tmp_dir.call_count, 0)

    def test_recover_successful_case(self):
        self.directory_iterator_obj._recover_each_tmp_dir = MagicMock()
        self.create_test_file_data_and_meta(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        self.directory_iterator_obj.recover(self.object_server_tmp_dir)
        self.assertTrue(self.directory_iterator_obj. \
            _recover_each_tmp_dir.called)
        self.assertEqual(self.directory_iterator_obj. \
            _recover_each_tmp_dir.call_count, 1)
        self.directory_iterator_obj._recover_each_tmp_dir. \
            assert_called_once_with(self.object_server_tmp_dir,
            data_list, meta_list)

    def test_recover_each_tmp_dir_only_data_case(self):
        self.create_test_file_only_data(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_incomplete_file = MagicMock()
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file = MagicMock()
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, data_list[0])))
        status = self.directory_iterator_obj._recover_each_tmp_dir \
            (self.object_server_tmp_dir, data_list, meta_list)
        self.assertTrue(status)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_incomplete_file.assert_called_once_with \
            (hash_path(ACCOUNT, CONTAINER, 'a.txt'))
        self.assertFalse(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_complete_file.called)
        self.assertFalse(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, data_list[0])))

    def test_recover_each_tmp_dir_data_and_empty_meta_case(self):
        self.create_test_file_data_and_empty_meta(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_incomplete_file = MagicMock()
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file = MagicMock()
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, data_list[0])))
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, meta_list[0])))
        status = self.directory_iterator_obj._recover_each_tmp_dir \
            (self.object_server_tmp_dir, data_list, meta_list)
        self.assertTrue(status)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_incomplete_file.assert_called_once_with \
            (hash_path(ACCOUNT, CONTAINER, 'b.txt'))
        self.assertFalse(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_complete_file.called)
        self.assertFalse(os.path.exists(os.path.join( \
            self.object_server_tmp_dir, data_list[0])))
        self.assertFalse(os.path.exists(os.path.join \
            (self.object_server_tmp_dir,
            '758c6502ca2a7be97c8edb1535c173fb.meta')))
        
    def test_recover_each_tmp_dir_data_and_meta_case(self):
        # test successful case
        self.create_test_file_data_and_meta(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_incomplete_file = MagicMock()
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file = MagicMock()
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, data_list[0])))
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, meta_list[0])))
        status = self.directory_iterator_obj._recover_each_tmp_dir \
            (self.object_server_tmp_dir, data_list, meta_list)
        self.assertTrue(status)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file.assert_called_once_with \
            (self.object_server_tmp_dir, hash_path(ACCOUNT, CONTAINER, 'c.txt'))
        self.assertFalse(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_incomplete_file.called)

    def test_recover_each_tmp_dir_only_meta_case(self):
        self.create_test_file_only_meta(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file = MagicMock()
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, meta_list[0])))
        status = self.directory_iterator_obj._recover_each_tmp_dir \
            (self.object_server_tmp_dir, data_list, meta_list)
        self.assertTrue(status)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file.assert_called_once_with \
            (self.object_server_tmp_dir,
            hash_path(ACCOUNT, CONTAINER, 'd.txt'), True)

    def test_recover_each_tmp_dir_only_meta_empty_case(self):
        self.create_test_file_only_meta_empty(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file = MagicMock()
        self.assertTrue(os.path.exists(os.path.join(self.object_server_tmp_dir,
            meta_list[0])))
        status = self.directory_iterator_obj._recover_each_tmp_dir \
            (self.object_server_tmp_dir, data_list, meta_list)
        self.assertTrue(status)
        self.assertFalse(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_complete_file.called)
        self.assertFalse(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, meta_list[0])))

    def test_recover_each_tmp_dir_exception(self):
        self.create_test_file_data_and_meta(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                    _DirectoryIterator__process_dir \
                                    (self.object_server_tmp_dir)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file = MagicMock(side_effect=Exception) 
        self.assertRaises(Exception, self.directory_iterator_obj. \
            _recover_each_tmp_dir, self.object_server_tmp_dir,
            data_list, meta_list)

    def test_recover_each_tmp_dir(self):
        self.create_test_file_data_and_meta(self.object_server_tmp_dir)
        self.create_test_file_only_meta_empty(self.object_server_tmp_dir)
        self.create_test_file_only_meta(self.object_server_tmp_dir)
        self.create_test_file_only_data(self.object_server_tmp_dir)
        self.create_test_file_data_and_empty_meta(self.object_server_tmp_dir)
        data_list, meta_list = self.directory_iterator_obj. \
                                   _DirectoryIterator__process_dir \
                                   (self.object_server_tmp_dir)
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_incomplete_file = MagicMock()
        self.directory_iterator_obj._DirectoryIterator__recover_file_obj. \
            recover_complete_file = MagicMock()
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, data_list[0])))
        self.assertTrue(os.path.exists(os.path.join \
            (self.object_server_tmp_dir, meta_list[0])))
        status = self.directory_iterator_obj._recover_each_tmp_dir \
            (self.object_server_tmp_dir, data_list, meta_list)
        self.assertTrue(status)
        self.assertTrue(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_complete_file.called)
        self.assertTrue(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_incomplete_file.called)
        self.assertEqual(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_complete_file.call_count, 2)
        self.assertEqual(self.directory_iterator_obj. \
            _DirectoryIterator__recover_file_obj. \
            recover_incomplete_file.call_count, 2)

    def test_recover_process_dir_exception(self):
        with mock.patch('osd.objectService.recovery_handler.DirectoryIterator._DirectoryIterator__process_dir', side_effect=OSError):
            self.assertRaises(OSError, self.directory_iterator_obj.recover, 'test')

    def test_recover_recover_each_tmp_dir_exception_case(self):
        with mock.patch('osd.objectService.recovery_handler.DirectoryIterator._DirectoryIterator__process_dir', return_value=('a', 'b')):
            self.directory_iterator_obj._recover_each_tmp_dir = mock.MagicMock(side_effect=OSError)
            self.assertRaises(OSError, self.directory_iterator_obj.recover, 'test')

class TestFileRecovery(unittest.TestCase):
    @mock.patch('osd.objectService.recovery_handler.CommunicateContainerService')
    def create_instance(self, CommunicateContainerService):
        return FileRecovery(FakeLogger(), object_ring=FakeObjectRing(''))

    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        rmtree(self.test_dir, ignore_errors=1)
        os.mkdir(self.test_dir)
        self.tmp_dir = os.path.join(self.test_dir, 'tmp')
        os.mkdir(self.tmp_dir)
        self.object_server_tmp_dir = os.path.join(self.tmp_dir, 'test_os_1')
        os.mkdir(self.object_server_tmp_dir)
        self.file_recovery_obj = self.create_instance()

    def tearDown(self):
        rmtree(self.object_server_tmp_dir, ignore_errors=1)
       
    
    def create_ondisk_file(self):
        """ create the data amd meta data file"""
        timestamp = time()
        timestamp = normalize_timestamp(timestamp)
        metadata = {}
        metadata['X-Timestamp'] = normalize_timestamp(timestamp)
        etag = md5()
        etag.update('data')
        metadata['ETag'] = etag.hexdigest()
        metadata['name'] = '/%s/%s/o' % (ACCOUNT, CONTAINER)
        metadata['Content-Length'] = str(len('data'))
        metadata['Content-Type'] = 'text'
        obj_hash = hash_path(ACCOUNT, CONTAINER, 'o')
        cont_hash = hash_path(ACCOUNT, CONTAINER)
        acc_hash = hash_path(ACCOUNT)
        file_name = '_'.join([acc_hash, cont_hash, obj_hash]) 
        data_file = os.path.join(self.object_server_tmp_dir,
            file_name + DATA_FILE_PREFIX)
        meta_file = os.path.join(self.object_server_tmp_dir,
            file_name + META_FILE_PREFIX)
        with open(data_file, 'wb') as f:
            f.write('data')
        with open(meta_file,'wb') as f:
            f.write(pickle.dumps(metadata))
        return metadata

    def test_recover_complete_file_successful_case_overwrite_case(self):
        metadata = self.create_ondisk_file()
        obj_hash = hash_path(ACCOUNT, CONTAINER, 'o')
        cont_hash = hash_path(ACCOUNT, CONTAINER)
        acc_hash = hash_path(ACCOUNT)
        file_name = '_'.join([acc_hash, cont_hash, obj_hash])
        data_target_path = os.path.join(self.object_server_tmp_dir,
           '123.data')
        meta_target_path = os.path.join(self.object_server_tmp_dir,
           '123.meta')
        open(data_target_path, 'w')
        open(meta_target_path, 'w')
        data_file = os.path.join(self.object_server_tmp_dir,
            file_name + DATA_FILE_PREFIX)
        meta_file = os.path.join(self.object_server_tmp_dir,
            file_name + META_FILE_PREFIX)
        self.file_recovery_obj._FileRecovery__get_target_path = MagicMock( \
            return_value= (data_target_path, meta_target_path)) 
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update = MagicMock()
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock = MagicMock()
        self.file_recovery_obj.recover_complete_file \
            (self.object_server_tmp_dir, file_name)
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update.assert_called_once_with(metadata, True) 
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock.assert_called_once_with(file_name.replace('_', '/'), metadata) 
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.release_lock.call_count, 1)
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.container_update.call_count, 1)

        # verify files
        self.assertTrue(os.path.exists(data_target_path))
        self.assertTrue(os.path.exists(meta_target_path))

    def test_recover_complete_file_successful_case_new_file(self):
        metadata = self.create_ondisk_file()
        obj_hash = hash_path(ACCOUNT, CONTAINER, 'o')
        cont_hash = hash_path(ACCOUNT, CONTAINER)
        acc_hash = hash_path(ACCOUNT)
        file_name = '_'.join([acc_hash, cont_hash, obj_hash])
        data_target_path = os.path.join(self.object_server_tmp_dir,
           '123.data')
        meta_target_path = os.path.join(self.object_server_tmp_dir,
           '123.meta')
        data_file = os.path.join(self.object_server_tmp_dir,
            file_name + DATA_FILE_PREFIX)
        meta_file = os.path.join(self.object_server_tmp_dir,
            file_name + META_FILE_PREFIX)
        self.file_recovery_obj._FileRecovery__get_target_path = MagicMock( \
            return_value= (data_target_path, meta_target_path)) 
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update = MagicMock()
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock = MagicMock()
        self.file_recovery_obj.recover_complete_file \
            (self.object_server_tmp_dir, file_name)
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update.assert_called_once_with(metadata, False) 
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock.assert_called_once_with(file_name.replace('_', '/'), metadata) 
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.release_lock.call_count, 1)
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.container_update.call_count, 1)

        # verify files
        self.assertTrue(os.path.exists(data_target_path))
        self.assertTrue(os.path.exists(meta_target_path))

    def test_recover_file_container_update_fail(self):
        _ = self.create_ondisk_file()
        obj_hash = hash_path(ACCOUNT, CONTAINER, 'o')
        cont_hash = hash_path(ACCOUNT, CONTAINER)
        acc_hash = hash_path(ACCOUNT)
        file_name = '_'.join([acc_hash, cont_hash, obj_hash])
        data_target_path = os.path.join(self.object_server_tmp_dir,
           'test', '123.data')
        meta_target_path = os.path.join(self.object_server_tmp_dir,
           'test', '123.meta')
        self.file_recovery_obj._FileRecovery__get_target_path = \
            MagicMock(return_value= (data_target_path, meta_target_path)) 
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update = MagicMock(return_value=False)
        self.file_recovery_obj.recover_complete_file \
            (self.object_server_tmp_dir, file_name)
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.container_update.call_count, 1)
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.release_lock.call_count, 0)
        
    
    @mock.patch('osd.objectService.recovery_handler.mkdirs')
    def test_only_meta_case(self, mkdirs):
        metadata = self.create_ondisk_file()
        obj_hash = hash_path(ACCOUNT, CONTAINER, 'o')
        cont_hash = hash_path(ACCOUNT, CONTAINER)
        acc_hash = hash_path(ACCOUNT)
        file_name = '_'.join([acc_hash, cont_hash, obj_hash])
        data_target_path = os.path.join(self.object_server_tmp_dir,
            '123.data')
        meta_target_path = os.path.join(self.object_server_tmp_dir,
            '123.meta')
        meta_file = os.path.join(self.object_server_tmp_dir, \
            file_name + META_FILE_PREFIX)
        open(data_target_path, 'w')
        open(meta_target_path, 'w')
        
        self.file_recovery_obj._FileRecovery__get_target_path = \
            MagicMock(return_value= (data_target_path, meta_target_path))
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update = MagicMock()
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock = MagicMock()
        self.file_recovery_obj.recover_complete_file \
            (self.object_server_tmp_dir, file_name, only_meta=True)
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update.assert_called_once_with(metadata, True) 
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock.assert_called_once_with(file_name.replace('_', '/'), metadata) 
        self.assertEqual(self.file_recovery_obj. \
           _FileRecovery__communicator_obj.release_lock.call_count, 1)
        self.assertEqual(self.file_recovery_obj. \
           _FileRecovery__communicator_obj.container_update.call_count, 1)
        mkdirs.assert_called_once_with(os.path.dirname(meta_target_path))
        self.assertEqual(mkdirs.call_count, 1) 

    @mock.patch('osd.objectService.recovery_handler.mkdirs')
    def test_only_meta_case_new_file(self, mkdirs):
        metadata = self.create_ondisk_file()
        obj_hash = hash_path(ACCOUNT, CONTAINER, 'o')
        cont_hash = hash_path(ACCOUNT, CONTAINER)
        acc_hash = hash_path(ACCOUNT)
        file_name = '_'.join([acc_hash, cont_hash, obj_hash])
        meta_file = os.path.join(self.object_server_tmp_dir, \
            file_name + META_FILE_PREFIX)
        data_target_path = os.path.join(self.object_server_tmp_dir,
           '123.data')
        meta_target_path = os.path.join(self.object_server_tmp_dir,
           '123.meta')
        self.file_recovery_obj._FileRecovery__get_target_path = MagicMock( \
            return_value= (data_target_path, meta_target_path))
        self.file_recovery_obj.recover_complete_file \
            (self.object_server_tmp_dir, file_name, only_meta=True)
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update.assert_called_once_with(metadata, False)
        obj_hash = file_name.replace('_', '/')
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock.assert_called_once_with(obj_hash, metadata)
        self.assertEqual(self.file_recovery_obj. \
           _FileRecovery__communicator_obj.release_lock.call_count, 1)
        self.assertEqual(self.file_recovery_obj. \
           _FileRecovery__communicator_obj.container_update.call_count, 1)
        self.assertEqual(mkdirs.call_count, 1)


    
    '''def test_recover_complete_file_no_meta_file(self):
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            container_update = MagicMock()
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock = MagicMock()
        self.file_recovery_obj.recover_complete_file \
            (self.object_server_tmp_dir, 'abc')
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.release_lock.call_count, 0)
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.container_update.call_count, 0)
    '''

    def test_recover_incomplete_file(self):
        self.file_recovery_obj._FileRecovery__communicator_obj. \
            release_lock = MagicMock()
        self.file_recovery_obj.recover_incomplete_file('123')
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__communicator_obj.release_lock.call_count, 1)
        self.file_recovery_obj._FileRecovery__communicator_obj.release_lock. \
            assert_called_once_with('123')

    def test_get_target_path(self):
        data, meta = self.file_recovery_obj. \
            _FileRecovery__get_target_path({'name': '/a/c/o'}, 'abc')
        account = hash_path('a')
        container = hash_path('a', 'c')
        self.assertEqual(self.file_recovery_obj. \
            _FileRecovery__get_target_path({'name': '/a/c/o'}, 'abc'),
            (('/export/fs1/a1/%s/d0/%s/o1/data/abc.data' % (account, container)),
             ('/export/fs1/a1/%s/d0/%s/o1/meta/abc.meta' % (account, container))))
        

class TestCommunicateContainerService(unittest.TestCase):
    def setUp(self):
        self.communicate_container_service_obj = CommunicateContainerService( \
            FakeLogger(), FakeContainerRing(''))
        self.metadata = {'name': '/a/c/o', 'Content-Length': 2, \
                         'Content-Type': 'text', 'X-Timestamp': time(), \
                         'ETag': 'abcde'}

    def tearDown(self):
        pass

    def test_get_service_details(self):
        host, directory, filesystem = self.communicate_container_service_obj. \
            _CommunicateContainerService__get_service_details( \
            {'name': '/a/c/o'}) 
        self.assertEqual(host, '1.2.3.4:0000')
        self.assertEqual(filesystem, 'fs1')
        self.assertEqual(directory, 'a1')
 
    @mock.patch('osd.objectService.recovery_handler.http_connection', mock_http_connection(200))
    def test_container_update(self):
        # test non-empty metadata case
        self.assertTrue(self.communicate_container_service_obj. \
            container_update(self.metadata))
        self.assertTrue(self.communicate_container_service_obj. \
            container_update(None, duplicate=True, method='DELETE', host='1:1'))

    @mock.patch('osd.objectService.recovery_handler.http_connection', mock_http_connection(404))
    def test_container_update_failure(self):
        # test empty metadata case
        self.assertFalse(self.communicate_container_service_obj. \
            container_update(''))

        # test error
        self.assertFalse(self.communicate_container_service_obj. \
            container_update({'name': '/a/c/o'}))

        self.assertFalse(self.communicate_container_service_obj. \
            container_update(self.metadata))
    
    @mock.patch('osd.objectService.recovery_handler.http_connection', mock_http_connection(200))
    def test_release_lock_no_metadata_case(self):
        # test non-empty metadata case
        self.communicate_container_service_obj. \
            _CommunicateContainerService__container_ring.get_service_list = \
            MagicMock(return_value=[{'cs_1': {'ip': '1.2.3.4', \
                                     'port': '0000'}}])
        self.assertTrue(self.communicate_container_service_obj. \
            release_lock('abc/def'))
        self.assertEqual(self.communicate_container_service_obj. \
            _CommunicateContainerService__container_ring. \
            get_service_list.call_count, 1)
        self.assertTrue(self.communicate_container_service_obj. \
            _CommunicateContainerService__container_ring. \
            get_service_list.called)

    @mock.patch('osd.objectService.recovery_handler.http_connection', mock_http_connection(404))
    def test_release_lock_failure_no_metadata_case(self):
        # test empty metadata case
        self.assertFalse(self.communicate_container_service_obj. \
            release_lock(''))
       
        self.assertFalse(self.communicate_container_service_obj. \
            release_lock('/a/c/o'))
    
    @mock.patch('osd.objectService.recovery_handler.http_connection', mock_http_connection(200))
    def test_release_lock_metadata_case(self):
        self.communicate_container_service_obj. \
            _CommunicateContainerService__container_ring.get_service_list = \
            MagicMock()
        self.assertTrue(self.communicate_container_service_obj. \
            release_lock('abc', self.metadata))
        self.assertEqual(self.communicate_container_service_obj. \
            _CommunicateContainerService__container_ring. \
            get_service_list.call_count, 0) 
        self.assertFalse(self.communicate_container_service_obj. \
            _CommunicateContainerService__container_ring. \
            get_service_list.called)


class TestTriggeredRecoveryHandler(unittest.TestCase):
    EXPORT = os.getcwd() + '/export'
    @mock.patch('osd.objectService.recovery_handler.CommunicateContainerService')
    def create_instance(self, CommunicateContainerService):
        return TriggeredRecoveryHandler(FakeLogger(), FakeObjectRing(''))
 
    def setUp(self):
        self.test_dir = os.getcwd() + '/export'
        rmtree(self.test_dir, ignore_errors=1)       
        os.mkdir('export')
        self.triggered_recovery_obj = self.create_instance()

    def tearDown(self):
        rmtree(self.test_dir, ignore_errors=1)
   
    def create_test_files(self):
        file_name = hash_path('a', 'c', 'o')
        data_dir = os.path.join(self.test_dir, 'fs1', 'a1', hash_path('a'), 'd0',
            hash_path('a', 'c'), 'o1', 'data')
        rmtree(data_dir, ignore_errors=1)
        os.makedirs(data_dir)
        meta_dir = os.path.join(self.test_dir, 'fs1', 'a1', hash_path('a'), 'd0',
            hash_path('a', 'c'), 'o1', 'meta')
        rmtree(meta_dir, ignore_errors=1)
        os.makedirs(meta_dir)
        with open(os.path.join(data_dir, file_name + '.data'), 'w') as fd:
            fd.write('data')
        metadata = {'Content-Length': 4,
                    'ETag': '8d777f385d3dfec8815d20f7496026dc',
                    'X-Timestamp': '1408505144.00290',
                    'name': '/a/c/o',
                    'Content-Type': 'text' }
        with open(os.path.join(meta_dir, file_name + '.meta'), 'wb') as fd:
            fd.write(pickle.dumps(metadata))
        return file_name, metadata
    
    @mock.patch('osd.objectService.recovery_handler.EXPORT_PATH', EXPORT)
    def test_recover_put_request(self):
        meta_dir = os.path.join(self.test_dir, 'fs1', 'a1', hash_path('a'), 'd0',
            hash_path('a', 'c'), 'o1', 'meta')
        data_dir = os.path.join(self.test_dir, 'fs1', 'a1', hash_path('a'), 'd0',
            hash_path('a', 'c'), 'o1', 'data')
        obj_hash = "%s/%s/%s" % (hash_path('a'), hash_path('a', 'c'), hash_path('a', 'c', 'o'))
        file_name, metadata = self.create_test_files()
        # test successful case PUT case
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.container_update = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj.recover(obj_hash, 'PUT', "1.2.3:123")
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.assert_called_once_with(metadata, unknown=True, host="1.2.3:123")
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.called)
        self.assertEqual(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.call_count, 1)
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.assert_called_once_with(obj_hash, host="1.2.3:123")
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        self.assertEqual(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.call_count, 1)

        # test only data file case
        os.remove(os.path.join(meta_dir, file_name + '.meta'))
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj.recover(obj_hash, 'PUT', "1.2.3:123")
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.assert_called_once_with(obj_hash, host="1.2.3:123")
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        self.assertEqual(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.call_count, 1)

        # no files case
        os.remove(os.path.join(data_dir, file_name + '.data'))
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj.recover(obj_hash, 'PUT', "1.2.3:123")
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.assert_called_once_with(obj_hash, host="1.2.3:123")
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        self.assertEqual(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.call_count, 1)

    @mock.patch('osd.objectService.recovery_handler.EXPORT_PATH', EXPORT)
    def test_recover_delete_request(self):
        meta_dir = os.path.join(self.test_dir, 'fs1', 'a1', hash_path('a'), 'd0',
            hash_path('a', 'c'), 'o1', 'meta')
        data_dir = os.path.join(self.test_dir, 'fs1', 'a1', hash_path('a'), 'd0',
            hash_path('a', 'c'), 'o1', 'data')
        obj_hash = "%s/%s/%s" % (hash_path('a'), hash_path('a', 'c'), hash_path('a', 'c', 'o'))
        file_name, metadata = self.create_test_files()
        
        # test DELETE case when both file exists
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.container_update = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj.recover(obj_hash, 'DELETE', "1.2.3:123")
        self.assertFalse(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.called)
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.assert_called_once_with(obj_hash,host="1.2.3:123")
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        self.assertEqual(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.call_count, 1)

        # test only meta file case
        os.remove(os.path.join(data_dir, file_name + '.data'))
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.container_update = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock()
        self.triggered_recovery_obj.recover(obj_hash, 'DELETE', host='1.2.3:123')
        #self.triggered_recovery_obj. \
        #    _TriggeredRecoveryHandler__communicator_obj. \
        #    container_update.assert_called_once_with(metadata, unknown=True)
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.called)
        self.assertEqual(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.call_count, 1)
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        self.assertFalse(os.path.exists(os.path.join(meta_dir, file_name + '.meta'))) 
        
        # test only meta file case, container update failed case
        file_name, metadata = self.create_test_files()
        os.remove(os.path.join(data_dir, file_name + '.data'))
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.container_update = \
            MagicMock(return_value=False)
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock()
        self.triggered_recovery_obj.recover(obj_hash, 'DELETE', host='1.2.3:123')
        #self.triggered_recovery_obj. \
        #    _TriggeredRecoveryHandler__communicator_obj. \
        #    container_update.assert_called_once_with(metadata, unknown=True)
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.called)
        self.assertEqual(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.call_count, 1)
        self.assertFalse(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        self.assertTrue(os.path.exists(os.path.join(meta_dir, file_name + '.meta'))) 

        # test release lock fail
        file_name, metadata = self.create_test_files()
        os.remove(os.path.join(data_dir, file_name + '.data'))
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.container_update = \
            MagicMock(return_value=True)
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock(return_value=False)
        self.triggered_recovery_obj.recover(obj_hash, 'DELETE', host='1.2.3:123')
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.called)
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        
        # test meta file not exists
        file_name, metadata = self.create_test_files()
        os.remove(os.path.join(meta_dir, file_name + '.meta'))
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.container_update = \
            MagicMock()
        self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj.release_lock = \
            MagicMock()
        self.triggered_recovery_obj.recover(obj_hash, 'DELETE', host='1.2.3:123')
        self.assertFalse(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            container_update.called)
        self.assertTrue(self.triggered_recovery_obj. \
            _TriggeredRecoveryHandler__communicator_obj. \
            release_lock.called)
        self.assertTrue(os.path.exists(os.path.join(data_dir,
           file_name + '.data')))

if __name__=='__main__':
    unittest.main() 
