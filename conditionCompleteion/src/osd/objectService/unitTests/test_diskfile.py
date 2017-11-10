import cPickle as pickle
import os
import errno
import mock
import unittest
import email
import tempfile
import uuid
#import xattr
from shutil import rmtree
from time import time
from tempfile import mkdtemp
from hashlib import md5
from contextlib import closing, nested
from gzip import GzipFile

from eventlet import tpool
from osd.objectService.unitTests import FakeLogger, temptree
from osd.objectService.unitTests import mock as unit_mock

from osd.objectService import diskfile
from osd.common import utils
#from osd.common.utils import hash_path, mkdirs, normalize_timestamp
from osd.common.utils import mkdirs, normalize_timestamp
from osd.common.ring.ring import hash_path
from osd.common import ring
from osd.common.exceptions import DiskFileNotExist, DiskFileQuarantined, \
    DiskFileDeviceUnavailable, DiskFileDeleted, DiskFileNotOpen, \
    DiskFileError, ReplicationLockTimeout, PathNotDir, DiskFileCollision, \
    DiskFileExpired, SwiftException, DiskFileNoSpace
import osd





class TestDiskFileManager(unittest.TestCase):
    def setUp(self):
        self.tmpdir = mkdtemp()
        self.testdir = os.path.join(self.tmpdir,'test_object_server_disk_file_mgr')
        mkdirs(os.path.join(self.testdir,"export", "fs1"))
        mkdirs(os.path.join(self.testdir,"export", "fs1"))
        self.filesystems = os.path.join(os.path.join(self.testdir,"export"))
        self._orig_tpool_exc = tpool.execute
        tpool.execute = lambda f, *args, **kwargs: f(*args, **kwargs)
        self.conf = dict(filesystems=self.filesystems,mount_check='false',
                        keep_cache_size=2 * 1024)
        self.df_mgr = diskfile.DiskFileManager(self.conf,FakeLogger())

    def tearDown(self):
        rmtree(self.tmpdir, ignore_errors=1)

    def test_construct_filesystem_path(self):
        res_path = self.df_mgr.construct_filesystem_path("abc")
        self.assertEqual(os.path.join(self.filesystems,"abc"),res_path)

    def test_get_filesystem_path(self):
        res_path = self.df_mgr.get_filesystem_path("abc")
        self.assertEqual(os.path.join(self.filesystems,"abc"),res_path)


class TestDiskFile(unittest.TestCase):
    def setUp(self):
        """ Setup the test"""  
        self.tmpdir = mkdtemp()
        self.testdir = os.path.join(self.tmpdir,'test_object_server_disk_file')
        self.filesystems = os.path.join(os.path.join(self.testdir,"export"))
        self.filesystem = "fs1"
        mkdirs(os.path.join(self.filesystems,self.filesystem))
        self._orig_tpool_exc = tpool.execute
        tpool.execute = lambda f, *args, **kwargs: f(*args, **kwargs)
        self.conf = dict(filesystems=self.filesystems, mount_check='false',
                         keep_cache_size=2 * 1024, )
        self.df_mgr = diskfile.DiskFileManager(self.conf, FakeLogger())

    def tearDown(self):
        """tear down the test"""
        rmtree(self.tmpdir, ignore_errors=1)
        tpool.execute = self._orig_tpool_exc
    
    def _create_ondisk_file(self, df, data, timestamp, metadata=None):
        """ create the data amd meta data file"""
        if timestamp is None:
            timestamp = time()
        timestamp = normalize_timestamp(timestamp)
        if not metadata:
            metadata = {}
        if 'X-Timestamp' not in metadata:
            metadata['X-Timestamp'] = normalize_timestamp(timestamp)
        if 'ETag' not in metadata:
            etag = md5()
            etag.update(data)
            metadata['ETag'] = etag.hexdigest()
        if 'name' not in metadata:
            metadata['name'] = '/a/c/o'
        if 'Content-Length' not in metadata:
            metadata['Content-Length'] = str(len(data))
        hash_name = df._name_hash
        mkdirs(df._datadir)
        mkdirs(df._metadir)
        data_file = os.path.join(df._datadir, df._name_hash + ".data")
        meta_file = os.path.join(df._metadir, df._name_hash + ".meta")
        with open(data_file, 'wb') as f:
            f.write(data)
        with open(meta_file,'wb') as f:
            f.write(pickle.dumps(metadata, diskfile.PICKLE_PROTOCOL))
            f.write("EOF")

    def _simple_get_diskfile(self, acc_dir="a1", cont_dir='d1', obj_dir='o1', account='a', container='c', obj='o'):
        """create the DiskFile object"""
        return self.df_mgr.get_diskfile(self.filesystem, acc_dir, cont_dir,
                                        obj_dir, account, container, obj)
       

    def _create_test_file(self, data, timestamp=None, metadata=None, account='a', container='c', obj='o'):
        """ creates the test file and opens it"""
        if metadata is None:
            metadata = {}
        metadata.setdefault('name', '/%s/%s/%s' % (account, container, obj))
        df = self._simple_get_diskfile(account=account, container=container,
                                       obj=obj)
        self._create_ondisk_file(df, data, timestamp, metadata)
        df = self._simple_get_diskfile(account=account, container=container,
                                       obj=obj)
        df.open()
        return df

    def test_open_not_exist(self):
        """ Test for DiskFileNotExist Exception"""
        df = self._simple_get_diskfile()
        self.assertRaises(DiskFileNotExist, df.open)

    def test_open_expired(self):
        """Test for DiskFileExpired Exception. 
           although it will not be used in Hydra
        """
        self.assertRaises(DiskFileExpired,
                          self._create_test_file,
                          '1234567890', metadata={'X-Delete-At': '0'})

    def test_open_not_expired(self):
        """ DiskFileExpired exception should not be raised"""
        try:
            self._create_test_file(
                '1234567890', metadata={'X-Delete-At': str(2 * int(time()))})
        except SwiftException as err:
            self.fail("Unexpected swift exception raised: %r" % err)

    def test_get_metadata(self):
        """get metadata """
        df = self._create_test_file('1234567890', timestamp=42)
        md = df.get_metadata()
        self.assertEqual(md['X-Timestamp'], normalize_timestamp(42))

    def test_read_metadata(self):
        """ read metadata """
        self._create_test_file('1234567890', timestamp=42)
        df = self._simple_get_diskfile()
        md = df.read_metadata()
        self.assertEqual(md['X-Timestamp'], normalize_timestamp(42))

    def test_get_metadata_not_opened(self):
        """ get metadata when the metadata field is not populated"""
        df = self._simple_get_diskfile()
        self.assertRaises(DiskFileNotOpen, df.get_metadata)

    def test_not_opened(self):
        """ test DiskFileNotOpen exception"""
        df = self._simple_get_diskfile()
        try:
            with df:
                pass
        except DiskFileNotOpen:
            pass
        else:
            self.fail("Expected DiskFileNotOpen exception")

    def _get_open_disk_file(self, invalid_type=None, obj_name='o', fsize=1024,
                            csize=8, mark_deleted=False, prealloc=False,
                            ts=None, mount_check=False, extra_metadata=None):
        '''returns a DiskFile'''
        df = self._simple_get_diskfile(obj=obj_name)
        data = '0' * fsize
        etag = md5()
        if ts is not None:
            timestamp = ts
        else:
            timestamp = normalize_timestamp(time())
        with df.create() as writer:
            upload_size = writer.write(data)
            etag.update(data)
            etag = etag.hexdigest()
            metadata = {
                'ETag': etag,
                'X-Timestamp': timestamp,
                'Content-Length': str(upload_size),
            }
            metadata.update(extra_metadata or {})
            writer.put(metadata)
            if invalid_type == 'ETag':
                etag = md5()
                etag.update('1' + '0' * (fsize - 1))
                etag = etag.hexdigest()
                metadata['ETag'] = etag
                diskfile.write_metadata(writer._fd_meta, metadata)
            elif invalid_type == 'Content-Length':
                metadata['Content-Length'] = fsize - 1
                diskfile.write_metadata(writer._fd_meta, metadata)
            elif invalid_type == 'Bad-Content-Length':
                metadata['Content-Length'] = 'zero'
                diskfile.write_metadata(writer._fd_meta, metadata)
            elif invalid_type == 'Missing-Content-Length':
                del metadata['Content-Length']
                diskfile.write_metadata(writer._fd_meta, metadata)
            elif invalid_type == 'Bad-X-Delete-At':
                metadata['X-Delete-At'] = 'bad integer'
                diskfile.write_metadata(writer._fd_meta, metadata)

        if mark_deleted:
            df.delete(timestamp)

        data_file = [os.path.join(df._datadir, fname)
                      for fname in sorted(os.listdir(df._datadir),
                                          reverse=True)
                      if fname.endswith('.data')]
        meta_file = [os.path.join(df._metadir, fname)
                      for fname in sorted(os.listdir(df._metadir),
                                          reverse=True)
                      if fname.endswith('.meta')]
                      
        ''' if invalid_type == 'Corrupt-Xattrs':
            # We have to go below read_metadata/write_metadata to get proper
            # corruption.
            meta_xattr = open(meta_file,'rb').read()
            wrong_byte = 'X' if meta_xattr[0] != 'X' else 'Y'
            xattr.setxattr(data_files[0], "user.osd.metadata",
                           wrong_byte + meta_xattr[1:])
        elif invalid_type == 'Truncated-Xattrs':
            meta_xattr = xattr.getxattr(data_files[0], "user.osd.metadata")
            xattr.setxattr(data_files[0], "user.osd.metadata",
                           meta_xattr[:-1])
        '''
        if invalid_type == 'Missing-Name':
            with open(meta_file,'r') as fd:
                md = diskfile.read_metadata(fd)
                del md['name']
            fd = os.open(meta_file,os.O_WRONLY|os.O_TRUNC)
            diskfile.write_metadata(fd, md)
        elif invalid_type == 'Bad-Name':
            with open(meta_file,'r') as fd:
                md = diskfile.read_metadata(fd)
                md['name'] = md['name'] + 'garbage'
            fd = os.open(meta_file,os.O_WRONLY|os.O_TRUNC)
            diskfile.write_metadata(fd, md)

        self.conf['disk_chunk_size'] = csize
        self.conf['mount_check'] = mount_check
        self.df_mgr = diskfile.DiskFileManager(self.conf, FakeLogger())
        df = self._simple_get_diskfile(obj=obj_name)
        df.open()
        if invalid_type == 'Zero-Byte':
            fp = open(df._data_file, 'w')
            fp.close()
        df.unit_test_len = fsize
        return df

    @mock.patch('osd.objectService.diskfile.DiskFileWriter.write', return_value=0)
    def test_delete(self, write):
        df = self._get_open_disk_file()
        ts = time()
        df.delete('data')
        data_file_name = df._name_hash + ".data"
        meta_file_name = df._name_hash + ".meta" 
        dl = os.listdir(df._datadir)
        ml = os.listdir(df._metadir)
        self.assertTrue(data_file_name not  in set(dl))
        self.assertTrue(meta_file_name in set(ml))
        df.delete('meta')
        dl = os.listdir(df._datadir)
        ml = os.listdir(df._metadir)
        self.assertTrue(meta_file_name not in set(ml))


    @mock.patch('osd.objectService.diskfile.DiskFileWriter.write', return_value=0)
    def test_open_deleted(self, write):
        df = self._get_open_disk_file()
        df.delete('data')
        df.delete('meta')
        data_file_name = df._name_hash + ".data"
        meta_file_name = df._name_hash + ".meta"
        dl = os.listdir(df._datadir)
        ml = os.listdir(df._metadir)
        self.assertTrue(data_file_name not  in set(dl))
        self.assertTrue(meta_file_name not  in set(ml))
        df = self._simple_get_diskfile()
        self.assertRaises(DiskFileNotExist, df.open) 

    def test_listdir_enoent(self):
        oserror = OSError()
        oserror.errno = errno.ENOENT
        self.df_mgr.logger.error = mock.MagicMock()
        with mock.patch('os.listdir', side_effect=oserror):
            self.assertEqual(self.df_mgr._listdir('path'), [])
            self.assertEqual(self.df_mgr.logger.error.mock_calls, [])

    def test_listdir_other_oserror(self):
        oserror = OSError()
        self.df_mgr.logger.error = mock.MagicMock()
        with mock.patch('os.listdir', side_effect=oserror):
            self.assertEqual(self.df_mgr._listdir('path'), [])
            self.df_mgr.logger.error.assert_called_once_with(
                'ERROR: Skipping %r due to error with listdir attempt: %s',
                'path', oserror)

    def test_listdir(self):
        self.df_mgr.logger.error = mock.MagicMock()
        with mock.patch('os.listdir', return_value=['abc', 'def']):
            self.assertEqual(self.df_mgr._listdir('path'), ['abc', 'def'])
            self.assertEqual(self.df_mgr.logger.error.mock_calls, [])

    def test_diskfile_names(self):
        df = self._simple_get_diskfile()
        self.assertEqual(df.account, 'a')
        self.assertEqual(df.container, 'c')
        self.assertEqual(df.obj, 'o')

    @mock.patch('osd.objectService.diskfile.DiskFileWriter.write', return_value=0)
    def test_diskfile_content_length_not_open(self, write):
        df = self._simple_get_diskfile()
        exc = None
        try:
            df.content_length
        except DiskFileNotOpen as err:
            exc = err
        self.assertEqual(str(exc), '')

    def test_diskfile_content_length(self):
        self._get_open_disk_file()
        df = self._simple_get_diskfile()
        with df.open():
            self.assertEqual(df.content_length, 1024)

    @mock.patch('osd.objectService.diskfile.DiskFileWriter.write', return_value=0)
    def test_diskfile_timestamp_not_open(self, write):
        df = self._simple_get_diskfile()
        exc = None
        try:
            df.timestamp
        except DiskFileNotOpen as err:
            exc = err
        self.assertEqual(str(exc), '')

    def test_diskfile_timestamp(self):
        self._get_open_disk_file(ts='1383181759.12345')
        df = self._simple_get_diskfile()
        with df.open():
            self.assertEqual(df.timestamp, '1383181759.12345')

    def test_write_metdata(self):
        df = self._create_test_file('1234567890')
        timestamp = normalize_timestamp(time())
        metadata = {'X-Timestamp': timestamp, 'X-Object-Meta-test': 'data'}
        metadata['name'] = '/a/c/o'
        metadata['Content-Length'] = '10'
        df.write_metadata(metadata)
        metadata = df.read_metadata()
        self.assertEquals(metadata['X-Object-Meta-test'],'data')
    
    '''
    def test_create_close_oserror(self):
        df = self.df_mgr.get_diskfile(self.filesystem, '0', 'abc', '123',
                                      'xyz')
        with mock.patch("osd.obj.diskfile.os.close",
                        mock.MagicMock(side_effect=OSError(
                            errno.EACCES, os.strerror(errno.EACCES)))):
            try:
                with df.create():
                    pass
            except Exception as err:
                self.fail("Unexpected exception raised: %r" % err)
            else:
                pass 
    '''
      
    def test_disk_file_mkstemp_creates_dir(self):
        tmpdir = os.path.join(self.filesystems, self.filesystem)
        os.rmdir(tmpdir)
        df = self._simple_get_diskfile()
        with df.create():
            self.assert_(os.path.exists(tmpdir)) 
   
    def test_disk_file_create_tmp_file(self):
        tmpdir = os.path.join(self.filesystems, self.filesystem) 
        os.rmdir(tmpdir)
        df = self._simple_get_diskfile()
        file_name = '_'.join([hash_path(df.account), hash_path(df.account, df.container),
                              df._name_hash])
        with df.create():
            self.assert_(os.path.exists(df._tmpdir))
            self.assert_(os.path.exists(os.path.join(df._tmpdir, file_name + ".data")))
            self.assert_(os.path.exists(os.path.join(df._tmpdir, file_name + ".meta")))

    def test_disk_file_finalize_put(self):
        tmpdir = os.path.join(self.filesystems, self.filesystem)
        os.rmdir(tmpdir)
        df = self._simple_get_diskfile()
        metadata = {'X-metadata-value':'data'}
        file_name = '_'.join([hash_path(df.account), hash_path(df.account, df.container),
                              df._name_hash])
        with df.create()  as writer:
            self.assertTrue(os.path.exists(df._tmpdir))
            self.assertTrue(os.path.exists(os.path.join(df._tmpdir, file_name + ".data")))
            self.assertTrue(os.path.exists(os.path.join(df._tmpdir, file_name + ".meta")))
            writer.put(metadata)
        self.assertTrue(os.path.exists(os.path.join(df._datadir, df._name_hash + ".data")))
        self.assertTrue(os.path.exists(os.path.join(df._metadir, df._name_hash + ".meta")))
        self.assertTrue(os.path.exists(df._tmpdir))
        self.assertFalse(os.path.exists(os.path.join(df._tmpdir, df._name_hash + ".data")))
        self.assertFalse(os.path.exists(os.path.join(df._tmpdir, df._name_hash + ".meta")))

    def test_disk_file_reader_iter(self):
        df = self._create_test_file('1234567890')
        reader = df.reader()
        self.assertEqual(''.join(reader), '1234567890')

    def test_disk_file_reader_iter_w_quarantine(self):
        df = self._create_test_file('1234567890')
        reader = df.reader()
        reader._obj_size += 1
        self.assertRaises(DiskFileQuarantined, ''.join, reader)

    def test_disk_file_app_iter_corners(self):
        df = self._create_test_file('1234567890')
        quarantine_msgs = []
        reader = df.reader()
        self.assertEquals(''.join(reader.app_iter_range(0, None)),
                          '1234567890')
        df = self._simple_get_diskfile()
        with df.open():
            reader = df.reader()
            self.assertEqual(''.join(reader.app_iter_range(5, None)), '67890')


    def test_disk_file_app_iter_range_w_none(self):
        df = self._create_test_file('1234567890')
        reader = df.reader()
        self.assertEqual(''.join(reader.app_iter_range(None, None)),
                         '1234567890')

    def test_disk_file_app_iter_partial_closes(self):
        df = self._create_test_file('1234567890')
        reader = df.reader()
        it = reader.app_iter_range(0, 5)
        self.assertEqual(''.join(it), '12345')
        self.assertTrue(reader._fp is None)

    @mock.patch('osd.objectService.diskfile.DiskFileWriter.write', return_value=0)
    def test_disk_file_app_iter_ranges(self, write):
        df = self._create_test_file('012345678911234567892123456789')
        reader = df.reader()
        it = reader.app_iter_ranges([(0, 10), (10, 20), (20, 30)],
                                    'plain/text',
                                    '\r\n--someheader\r\n', 30)
        value = ''.join(it)
        self.assertTrue('0123456789' in value)
        self.assertTrue('1123456789' in value)
        self.assertTrue('2123456789' in value)


    """
    def test_disk_file_app_iter_ranges_w_quarantine(self):
        df = self._create_test_file('012345678911234567892123456789')
        reader = df.reader()
        reader._obj_size += 1
        try:
            it = reader.app_iter_ranges([(0, 30)],
                                    'plain/text',
                                    '\r\n--someheader\r\n', 30)
            value = ''.join(it)
        except DiskFileQuarantined as e:
               err = e
        #self.assertEqual('DiskFileQuarantined',str(err))
        self.assertTrue('0123456789' in value)
        self.assertTrue('1123456789' in value)
        self.assertTrue('2123456789' in value)
    """
    
    def test_disk_file_app_iter_ranges_w_no_etag_quarantine(self):
        df = self._create_test_file('012345678911234567892123456789')
        reader = df.reader()
        it = reader.app_iter_ranges([(0, 10)],
                                    'plain/text',
                                    '\r\n--someheader\r\n', 30)
        value = ''.join(it)
        self.assertTrue('0123456789' in value)

    def test_disk_file_app_iter_ranges_edges(self):
        df = self._create_test_file('012345678911234567892123456789')
        reader = df.reader()
        it = reader.app_iter_ranges([(3, 10), (0, 2)], 'application/whatever',
                                    '\r\n--someheader\r\n', 30)
        value = ''.join(it)
        self.assertTrue('3456789' in value)
        self.assertTrue('01' in value)

    def test_disk_file_large_app_iter_ranges(self):
        # This test case is to make sure that the disk file app_iter_ranges
        # method all the paths being tested.
        long_str = '01234567890' * 65536
        target_strs = ['3456789', long_str[0:65590]]
        df = self._create_test_file(long_str)
        reader = df.reader()
        it = reader.app_iter_ranges([(3, 10), (0, 65590)], 'plain/text',
                                    '5e816ff8b8b8e9a5d355497e5d9e0301', 655360)

        # The produced string actually missing the MIME headers
        # need to add these headers to make it as real MIME message.
        # The body of the message is produced by method app_iter_ranges
        # off of DiskFile object.
        header = ''.join(['Content-Type: multipart/byteranges;',
                          'boundary=',
                          '5e816ff8b8b8e9a5d355497e5d9e0301\r\n'])

        value = header + ''.join(it)

        parts = map(lambda p: p.get_payload(decode=True),
                    email.message_from_string(value).walk())[1:3]
        self.assertEqual(parts, target_strs)

    def test_disk_file_app_iter_ranges_empty(self):
        # This test case tests when empty value passed into app_iter_ranges
        # When ranges passed into the method is either empty array or None,
        # this method will yield empty string
        df = self._create_test_file('012345678911234567892123456789')
        reader = df.reader()
        it = reader.app_iter_ranges([], 'application/whatever',
                                    '\r\n--someheader\r\n', 100)
        self.assertEqual(''.join(it), '')

        df = self._simple_get_diskfile()
        with df.open():
            reader = df.reader()
            it = reader.app_iter_ranges(None, 'app/something',
                                        '\r\n--someheader\r\n', 150)
            self.assertEqual(''.join(it), '')



    @mock.patch('osd.objectService.diskfile.DiskFileWriter.write', return_value=0)
    def test_keep_cache(self, write):
        df = self._get_open_disk_file(fsize=65)
        with mock.patch("osd.objectService.diskfile.drop_buffer_cache") as foo:
            for _ in df.reader():
                pass
            self.assertTrue(foo.called)

        df = self._get_open_disk_file(fsize=65)
        with mock.patch("osd.objectService.diskfile.drop_buffer_cache") as bar:
            for _ in df.reader(keep_cache=False):
                pass
            self.assertTrue(bar.called)

        df = self._get_open_disk_file(fsize=65)
        with mock.patch("osd.objectService.diskfile.drop_buffer_cache") as boo:
            for _ in df.reader(keep_cache=True):
                pass
            self.assertFalse(boo.called)

        df = self._get_open_disk_file(fsize=5 * 1024, csize=256)
        with mock.patch("osd.objectService.diskfile.drop_buffer_cache") as goo:
            for _ in df.reader(keep_cache=True):
                pass
            self.assertTrue(goo.called)


if __name__ =="__main__":
    unittest.main() 
