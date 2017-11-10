# Copyright (c) 2010-2013 OpenStack Foundation
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

"""
Disk File Interface for the Swift Object Server

The `DiskFile`, `DiskFileWriter` and `DiskFileReader` classes combined define
the on-disk abstraction layer for supporting the object server REST API
interfaces. Other implementations wishing to provide
an alternative backend for the object server must implement the three
classes. An example alternative implementation can be found in the
`mem_server.py` and `mem_diskfile.py` modules along size this one.

The `DiskFileManager` is a reference implemenation specific class and is not
part of the backend API.

The remaining methods in this module are considered implementation specifc and
are also not considered part of the backend API.
"""

import cPickle as pickle
import errno
import os
import time
import uuid
import hashlib
import logging
import traceback
from os.path import basename, dirname, exists, getmtime, join
from contextlib import contextmanager
from collections import defaultdict

from eventlet import Timeout, greenio

from osd import gettext_ as _
from osd.common.constraints import check_mount
from osd.common.utils import mkdirs, normalize_timestamp, \
    storage_directory, renamer, fallocate, fsync, \
    fdatasync, drop_buffer_cache, ThreadPool, lock_path, write_pickle, \
    config_true_value, listdir, ismount, remove_file, create_tmp_file
from osd.common.helper_functions import split_path
from osd.common.ring.ring import get_service_id, hash_path
from osd.common.exceptions import DiskFileQuarantined, DiskFileNotExist, \
    DiskFileCollision, DiskFileNoSpace, DiskFileDeviceUnavailable, \
    DiskFileDeleted, DiskFileError, DiskFileNotOpen, PathNotDir, \
    ReplicationLockTimeout, DiskFileExpired
from osd.common.swob import multi_range_iterator
from libObjectLib import ObjectLibrary
import libraryUtils
import struct


PICKLE_PROTOCOL = 2
ONE_WEEK = 604800
# These are system-set metadata keys that cannot be changed with a POST.
# They should be lowercase.
DATAFILE_SYSTEM_META = set('content-length content-type deleted etag'.split())
ASYNCDIR = 'async_pending'
EXPORT_PATH = "/export"
DATADIR = "data"
METADIR = "meta"
TMPDIR = "tmp"
UPDATEDIR = "updater"


def read_metadata(fd_meta):
    """
    Helper function to read the pickled metadata from an object file.

    :param fd: file object opened by open call

    :returns: dictionary of metadata
    """
    metadata = fd_meta.read()
    return pickle.loads(metadata)


def write_metadata(fd_meta, metadata):
    """
    Helper function to write pickled metadata for a metadata file file.

    :param fd: file descriptor or filename to write the metadata
    :param metadata: metadata to write
    """
    metastr = pickle.dumps(metadata, PICKLE_PROTOCOL)
    try:
        while metastr:
            written = os.write(fd_meta, metastr[:65536])
            metastr = metastr[written:]
        os.write(fd_meta,"EOF")
    except OSError:
        pass      
        

class DiskFileManager(object):
    """
    Management class for devices, providing common place for shared parameters
    and methods not provided by the DiskFile class (which primarily services
    the object server REST API layer).

    The `get_diskfile()` method is how this implementation creates a `DiskFile`
    object.

    .. note::

        This class is reference implementation specific and not part of the
        pluggable on-disk backend API.

    .. note::

        TODO(portante): Not sure what the right name to recommend here, as
        "manager" seemed generic enough, though suggestions are welcome.

    :param conf: caller provided configuration object
    :param logger: caller provided logger
    """
    def __init__(self, conf, logger):
        libraryUtils.OSDLoggerImpl("object-library").initialize_logger()
        self.logger = logger
        self.filesystems = conf.get('filesystems', '/export')
        self.disk_chunk_size = int(conf.get('disk_chunk_size', 65536))
        self.keep_cache_size = int(conf.get('keep_cache_size', 5242880))
        self.mount_check = config_true_value(conf.get('mount_check', 'true'))
        self.reclaim_age = int(conf.get('reclaim_age', ONE_WEEK))
        threads_for_read = int(conf.get('threads_for_read', '8'))
        threads_for_write = int(conf.get('threads_for_write', '4'))
        self.threadpools_write = defaultdict(
            lambda: ThreadPool(nthreads=threads_for_write))
        self.object_server_id = get_service_id('object')
        self.object_lib = ObjectLibrary(threads_for_write, threads_for_read)

        #if statInstance:
        #    self.statInstance = statInstance
	#Creating Performance Stats parameter
        #self.statInstance = libraryUtils.StatsInterface()
        #self.statInstance = pystat.createAndRegisterStat("object_stat_get", libraryUtils.Module.ObjectServiceGet)

        #self.get_object_counter = 0

        #self.get_object_size = 0
        #self.get_time = 0

    def construct_filesystem_path(self, filesystem):
        """
        Construct the path to a filesystem without checking if it is mounted.

        :param device: name of target filesystem
        :returns: full path to the filesystem
        """
        return os.path.join(self.filesystems, filesystem)

    def get_filesystem_path(self, filesystem, mount_check=None):
        """
        Return the path to a filesystem, checking to see that it is a proper mount
        point based on a configuration parameter.

        :param device: name of target filesystem
        :param mount_check: whether or not to check mountedness of filesystem.
                            Defaults to bool(self.mount_check).
        :returns: full path to the filesystem, None if the path to the filesystem is
                  not a proper mount point.
        """
        should_check = self.mount_check if mount_check is None else mount_check
        if should_check and not check_mount(self.filesystems, filesystem):
            filesystem_path = None
        else:
            filesystem_path = os.path.join(self.filesystems, filesystem)
        return filesystem_path

    def get_diskfile(self, filesystem, acc_dir, cont_dir, \
                     obj_dir, account, container, obj, \
                     **kwargs):
        filesystem_path = self.get_filesystem_path(filesystem)
        if not filesystem_path:
            raise DiskFileDeviceUnavailable()
        return DiskFile(self, filesystem_path, self.threadpools_write['write'],
                        self.object_lib, acc_dir, cont_dir,
                        obj_dir, account, container, obj, **kwargs)

    def _listdir(self, path):
        try:
            return os.listdir(path)
        except OSError as err:
            if err.errno != errno.ENOENT:
                self.logger.error(
                    'ERROR: Skipping %r due to error with listdir attempt: %s',
                    path, err)
        return []

class DiskFileWriter(object):
    """
    Encapsulation of the write context for servicing PUT REST API
    requests. Serves as the context manager object for the
    :class:`osd.objectService.diskfile.DiskFile` class's
    :func:`osd.objectService.diskfile.DiskFile.create` method.

    .. note::

        It is the responsibility of the
        :func:`osd.objectService.diskfile.DiskFile.create` method context manager to
        close the open file descriptor.

    .. note::

        The arguments to the constructor are considered implementation
        specific. The API does not define the constructor arguments.

    :param name: name of object from REST API
    :param name_hash: hash of the '/account/containr/object'
    :param metadir :on-disk directory metadata will end up in on 
                    :func:`osd.objectService.diskfile.DiskFileWriter.put`
    :param datadir: on-disk directory object will end up in on
                    :func:`osd.objectService.diskfile.DiskFileWriter.put`
    :param fd_data: open file descriptor of temporary file to receive data
    :param fd_meta: open file descriptor of temporary file to receive metadata
    :param tmppath_data: full path name of the opened file descriptor for data
    :param tmppath_meta: full path name of the opened file descriptor for metadata
    :param threadpool: internal thread pool to use for filesystem operations
    """
    def __init__(self, name, name_hash, datadir, metadir, fd_data, fd_meta,
            tmppath_data, tmppath_meta, rpipe, wpipe, threadpool, object_lib, logger):
        # Parameter tracking
        self._name = name
        self._name_hash = name_hash 
        self._datadir = datadir
        self._metadir = metadir
        self._fd_data = fd_data
        self._fd_meta = fd_meta
        self._tmppath_data = tmppath_data
        self._tmppath_meta = tmppath_meta
        self._wpipe = wpipe
        self._rpipe = rpipe
        self._threadpool = threadpool
        self._object_lib = object_lib

        # Internal attributes
        self._upload_size = 0
        self._data_extension = '.data'
        self._meta_extension = '.meta'
        self._logger = logger

    def write(self, chunk):
        """
        Write a chunk of data to disk. All invocations of this method must
        come before invoking the :func:

        For this implementation, the data is written into a temporary file.

        :param chunk: the chunk of data to write as a string object

        :returns: the total number of bytes written to an object
        """

        try:
            while chunk:
                self._object_lib.write_chunk(self._fd_data, chunk, len(chunk), self._wpipe)
                ret = self._rpipe.read(4)
                written, = struct.unpack("i", ret)
                if (written == -1):
                    raise IOError("write data file error")
                self._upload_size += written
                chunk = chunk[written:]
        except Exception as e:
            self._logger.error(_(
                'ERROR DiskFile %(data_file)s'
                ' write failure: %(exc)s : %(stack)s'),
                {'exc': e, 'stack': ''.join(traceback.format_stack()),
                 'data_file': self._name})
            remove_file(self._tmppath_data)
            remove_file(self._tmppath_meta)
            raise
            
        return self._upload_size

    def init_md5_hash(self):
        self._object_lib.init_md5_hash(self._fd_data)

    def get_md5_hash(self):
        return self._object_lib.get_md5_hash(self._fd_data)

    def _finalize_put(self, metadata, target_data_path, target_meta_path):
        # Write the metadata before calling fsync() so that both data and
        # metadata are flushed to disk.
        write_metadata(self._fd_meta, metadata)
        # We call fsync() before calling drop_cache() to lower the amount of
        # redundant work the drop cache code will perform on the pages (now
        # that after fsync the pages will be all clean).
        fsync(self._fd_meta)
        fsync(self._fd_data)
        # From the Department of the Redundancy Department, make sure we call
        # drop_cache() after fsync() to avoid redundant work (pages all
        # clean).
        drop_buffer_cache(self._fd_data, 0, self._upload_size)
        # After the rename completes, this object will be available for other
        # requests to reference.
        renamer(self._tmppath_data, target_data_path)
        renamer(self._tmppath_meta, target_meta_path)

    def put(self, metadata):
        """
        Finalize writing the file on disk.

        For this implementation, this method is responsible for renaming the
        temporary file to the final name and directory location.  This method
        should be called after the final call to
        :func:`osd.objectService.diskfile.DiskFileWriter.write`.

        :param metadata: dictionary of metadata to be associated with the
                         object
        """
        metadata['name'] = self._name
        target_data_path = join(self._datadir, self._name_hash + self._data_extension)
        target_meta_path = join(self._metadir, self._name_hash + self._meta_extension)

        self._threadpool.force_run_in_thread(
            self._finalize_put, metadata, target_data_path,target_meta_path)


class DiskFileReader(object):
    """
    Encapsulation of the WSGI read context for servicing GET REST API
    requests. Serves as the context manager object for the
    :class:`osd.objectService.diskfile.DiskFile` class's
    :func:`osd.objectService.diskfile.DiskFile.reader` method.

    .. note::

        The quarantining behavior of this method is considered implementation
        specific, and is not required of the API.

    .. note::

        The arguments to the constructor are considered implementation
        specific. The API does not define the constructor arguments.

    :param fp: open file object pointer reference
    :param data_file: on-disk data file name for the object
    :param obj_size: verified on-disk size of the object
    :param etag: expected metadata etag value for entire file
    :param threadpool: thread pool to use for read operations
    :param disk_chunk_size: size of reads from disk in bytes
    :param keep_cache_size: maximum object size that will be kept in cache
    :param device_path: on-disk device path, used when quarantining an obj
    :param logger: logger caller wants this object to use
    :param quarantine_hook: 1-arg callable called w/reason when quarantined
    :param keep_cache: should resulting reads be kept in the buffer cache
    """
    def __init__(self, fp, data_file, obj_size, etag, object_lib,
                 disk_chunk_size, keep_cache_size, device_path, logger,
                  keep_cache=False, statInstance=None):
        # Parameter tracking
        self._fp = fp
        self._data_file = data_file
        self._obj_size = obj_size
        self._etag = etag
        self._object_lib = object_lib
        self._disk_chunk_size = disk_chunk_size
        self._device_path = device_path
        self._logger = logger
        if keep_cache:
            # Caller suggests we keep this in cache, only do it if the
            # object's size is less than the maximum.
            self._keep_cache = obj_size < keep_cache_size
        else:
            self._keep_cache = False

        # Internal Attributes
        self._iter_etag = None
        self._bytes_read = 0
        self._started_at_0 = False
        self._read_to_eof = False
        self._suppress_file_closing = False
        self._quarantined_dir = None

        #Creating Performance Stats parameter
        if statInstance:
            self.statInstance = statInstance
        self.get_object_size = 0
        self.get_time = 0

    def __iter__(self):
        """Returns an iterator over the data file."""
        _raw_rpipe, self.wpipe = os.pipe()
        self.rpipe = greenio.GreenPipe(_raw_rpipe, 'rb', bufsize=0)

        try:
            dropped_cache = 0
            self._bytes_read = 0
            self._started_at_0 = False
            self._read_to_eof = False
            use_hash = False
            if self._fp.tell() == 0:
                self._started_at_0 = True
                use_hash = True
                self._object_lib.init_md5_hash(self._fp.fileno())
            chunk = "".zfill(self._disk_chunk_size)
            #Performance Start time
            start_time = time.time()
            while True:
                self._object_lib.read_chunk(self._fp.fileno(), chunk,
                        self._disk_chunk_size, self.wpipe)
                ret = self.rpipe.read(4)
                readBytes, = struct.unpack("i", ret)
                if readBytes == -1:
                    raise IOError("read data file error")
                if readBytes != 0:
                    self._bytes_read += readBytes
                    chunk = chunk[:readBytes]
                    if self._bytes_read - dropped_cache > (1024 * 1024):
                        self._drop_cache(self._fp.fileno(), dropped_cache,
                                         self._bytes_read - dropped_cache)
                        dropped_cache = self._bytes_read
                    yield chunk
                if readBytes != self._disk_chunk_size:
                    self._read_to_eof = True
                    self._drop_cache(self._fp.fileno(), dropped_cache,
                                     self._bytes_read - dropped_cache)
                    break

	    #Performance Stat for GET Request
            size_in_mb = self._bytes_read / (1024*1024)
            elapsed_time = time.time() - start_time 
            self.get_object_size += size_in_mb
            self.get_time += elapsed_time

            if self.statInstance:
                if elapsed_time:
                    band = self._bytes_read / elapsed_time
                    self.statInstance.avgBandForDownloadObject = self.get_object_size / self.get_time
                    if self.statInstance.maxBandForDownloadObject < band:
                        self.statInstance.maxBandForDownloadObject = band
                    if self.statInstance.minBandForDownloadObject == 0:
                        self.statInstance.minBandForDownloadObject = band
                    else:
                        if self.statInstance.minBandForDownloadObject > band:
                            self.statInstance.minBandForDownloadObject = band

        except (Exception, Timeout) as e:
            self._logger.error(_(
                'ERROR DiskFile %(data_file)s'
                ' read failure: %(exc)s : %(stack)s'),
                {'exc': e, 'stack': ''.join(traceback.format_stack()),
                 'data_file': self._data_file})
            raise
        finally:
            if use_hash:
                self._iter_etag = self._object_lib.get_md5_hash(self._fp.fileno())
            if not self._suppress_file_closing:
                self.close()
            self.rpipe.close()
            os.close(self.wpipe)

    def app_iter_range(self, start, stop):
        """Returns an iterator over the data file for range (start, stop)"""
        if start or start == 0:
            self._fp.seek(start)
        if stop is not None:
            length = stop - start
        else:
            length = None
        try:
            for chunk in self:
                if length is not None:
                    length -= len(chunk)
                    if length < 0:
                        # Chop off the extra:
                        yield chunk[:length]
                        break
                yield chunk
        finally:
            if not self._suppress_file_closing:
                self.close()

    def app_iter_ranges(self, ranges, content_type, boundary, size):
        """Returns an iterator over the data file for a set of ranges"""
        if not ranges:
            yield ''
        else:
            try:
                self._suppress_file_closing = True
                for chunk in multi_range_iterator(
                        ranges, content_type, boundary, size,
                        self.app_iter_range):
                    yield chunk
            finally:
                self._suppress_file_closing = False
                self.close()

    def _drop_cache(self, fd, offset, length):
        """Method for no-oping buffer cache drop method."""
        if not self._keep_cache:
            drop_buffer_cache(fd, offset, length)

    def _quarantine(self, msg):
        #Commented all the lines which are not required
        #only logging is done 
        #self._quarantined_dir = self._threadpool.run_in_thread(
        #   quarantine_renamer, self._device_path, self._data_file)
        self._logger.warn("Quarantined object %s: %s" % (
            self._data_file, msg))
        #self._logger.increment('quarantines')
        #self._quarantine_hook(msg)
        raise DiskFileQuarantined()

    def _handle_close_quarantine(self):
        """Check if file needs to be quarantined"""
        if self._bytes_read != self._obj_size:
            self._quarantine(
                "Bytes read: %s, does not match metadata: %s" % (
                    self._bytes_read, self._obj_size))
        elif self._iter_etag and \
                self._etag != self._iter_etag:
            self._quarantine(
                "ETag %s and file's md5 %s do not match" % (
                    self._etag, self._iter_etag))

    def close(self):
        """
        Close the open file handle if present.

        For this specific implementation, this method will handle quarantining
        the file if necessary.
        """
        if self._fp:
            try:
                if self._started_at_0 and self._read_to_eof:
                    self._handle_close_quarantine()
            except DiskFileQuarantined:
                raise
            except (Exception, Timeout) as e:
                self._logger.error(_(
                    'ERROR DiskFile %(data_file)s'
                    ' close failure: %(exc)s : %(stack)s'),
                    {'exc': e, 'stack': ''.join(traceback.format_stack()),
                     'data_file': self._data_file})
            finally:
                fp, self._fp = self._fp, None
                fp.close()

class DiskFile(object):
    """
    Manage object files and meta data files.

    This specific implementation manages object files on a disk formatted with
    a POSIX-compliant file system that does not supports extended attributes as
    metadata on a file or directory.Meatdata is stored in separate files in diffedrent 
    directory.

    .. note::

    The arguments to the constructor are considered implementation
    specific. The API does not define the constructor arguments.

    :param mgr: associated DiskFileManager instance
    :param filesystem: path to the target filesystem
    :param threadpool: thread pool to use for blocking operations
    :param directory: directory in the filesystem  in which the object and metadata file lives
    :param account: account name for the object
    :param container: container name for the object
    :param obj: object name for the object
      """
    
    def __init__(self, mgr, filesystem, threadPool_write,
                 object_lib, acc_dir, cont_dir, obj_dir,
                 account=None, container=None, obj=None):
        self._mgr = mgr
        self._filesystem = filesystem
        self._acc_dir = acc_dir
        self._cont_dir = cont_dir
        self._obj_dir = obj_dir
        self._threadpool_write = threadPool_write or ThreadPool(nthreads = 0)
        self._object_lib = object_lib
        self._logger = mgr.logger
        self._disk_chunk_size = mgr.disk_chunk_size
        self._object_server_id = mgr.object_server_id
        self._datadir = os.path.join(
               self._filesystem, acc_dir, hash_path(account), cont_dir,
               hash_path(account, container), obj_dir, DATADIR)
        self._metadir = os.path.join(
                self._filesystem, acc_dir, hash_path(account), cont_dir,
                hash_path(account, container), obj_dir, METADIR)
        self._tmpdir = os.path.join(self._filesystem, TMPDIR, self._object_server_id)
        self._meta_ext = ".meta"
        self._data_ext = ".data"
        if account and container and obj:
            self._name = '/' + '/'.join((account, container, obj))
            self._account = account
            self._container = container
            self._obj = obj
            self._name_hash = hash_path(account, container, obj)
        else:
            _name = None
            self._account = None
            self._container = None
            self._obj = None
            self._name_hash = None
        self._metadata = None
        self._data_file = None
        self._meta_file = None
        self._fp_meta = None
        self._fp = None
        self._content_length = None
        
        
    @property
    def account(self):
        return self._account

    @property
    def container(self):
        return self._container

    @property
    def obj(self):
        return self._obj

    @property
    def content_length(self):
        if self._metadata is None:
            raise DiskFileNotOpen()
        return self._content_length

    @property
    def timestamp(self):
        if self._metadata is None:
            raise DiskFileNotOpen()
        return self._metadata.get('X-Timestamp')
    
    def __enter__(self):
        """
        Context enter.

        .. note::

            An implemenation shall raise `DiskFileNotOpen` when has not
            previously invoked the :func:`osd.objectService.diskfile.DiskFile.open`
            method.
        """
        if self._metadata is None:
            raise DiskFileNotOpen()
        return self

    def __exit__(self, t, v, tb):
        """
        Context exit.

        .. note::

            This method will be invoked by the object server while servicing
            the REST API *before* the object has actually been read. It is the
            responsibility of the implementation to properly handle that.
        """
        if self._fp is not None:
            fp, self._fp = self._fp, None
            fp.close()
        if self._fp_meta is not None:
            fp_meta, self._fp_meta = self._fp_meta, None
            fp_meta.close()
    
    def open(self):
        """
        Open the object.

        This implementation opens the data file representing the object, reads
        the associated metadata from metadata file.

        .. note::

            An implementation is allowed to raise any of the following
            exceptions, but is only required to raise `DiskFileNotExist` when
            the object representation does not exist.

        :raises DiskFileCollision: on name mis-match with metadata
        :raises DiskFileNotExist: if the object does not exist
        :raises DiskFileDeleted: if the object was previously deleted
        :raises DiskFileQuarantined: if while reading metadata of the file
                                     some data did pass cross checks
        :returns: itself for use as a context manager
        """
        meta_file = self._get_meta_file_name()
        data_file = self._get_data_file_name()
        self._data_file = data_file
        self._meta_file = meta_file
        fp = self._threadpool_write.force_run_in_thread (\
                 self.check_file_exists_and_open, self._meta_file,\
                 self._data_file)
        meta_fp = self._construct_from_meta_file(meta_file)
        self._verify_data_file(data_file, fp)
        self._metadata = self._metadata or {}
        self._fp_meta = meta_fp
        self._fp = fp
        return self
    
    def check_file_exists_and_open(self, meta_file , data_file):
	
        if not exists(meta_file):
            raise DiskFileNotExist()
        if not exists(data_file):
            raise DiskFileNotExist()
        return self._open_file(data_file)
        
    
    def _get_meta_file_name(self):
        """
        Gives the full path of metadata file
        """
        if self._name_hash:
            return os.path.join(self._metadir, self._name_hash + self._meta_ext)
        self._name_hash = hash_path(self._account, self._container, self._obj)
        return os.path.join(self._metadir, self._name_hash + self._meta_ext)
    
    def _get_data_file_name(self):
        """
        Gives the full path of data file
        """
        if self._name_hash:
            return os.path.join(self._datadir, self._name_hash + self._data_ext)
        self._name_hash = hash_path(self._account, self._container, self._obj)
        return os.path.join(self._datadir, self._name_hash + self._data_ext)
        
        
    def _construct_from_meta_file(self, meta_file,):
        """
        opens the metadata files and read the metadata
        """
        meta_fp = self._open_file(meta_file)
        self._metadata = self._failsafe_read_metadata(meta_fp)
        if self._name is None:
            self._name = self._metadata['name']
        return meta_fp
            
    def _open_file(self, file_name):
        """
        opens a file and returns file object 'not file descriptor as returned by os.open()'
        """
        return open(file_name, 'rb')
    
    def get_metadata(self):
        """
        Provide the metadata for a previously opened object as a dictionary.

        :returns: object's metadata dictionary
        :raises DiskFileNotOpen: if the
            :func:`osd.objectService.diskfile.DiskFile.open` method was not previously
            invoked
        """
        if self._metadata is None:
            raise DiskFileNotOpen()
        return self._metadata
    
    def read_metadata(self):
        """
        Return the metadata for an object without requiring the caller to open
        the metadata file  first.

        :returns: metadata dictionary for an object
        :raises DiskFileError: this implementation will raise the same
                            errors as the `open()` method.
        """
        with self.open():
            return self.get_metadata()
    
    @contextmanager    
    def create(self):
        """
        Context manager to create a data file and metadata file .
        We create a temporary file first, and
        then return a DiskFileWriter object to encapsulate the state.

        .. note::

            An implementation is not required to perform on-disk
            preallocations even if the parameter is specified. But if it does
            and it fails, it must raise a `DiskFileNoSpace` exception.

        """
        #if not exists(self._tmpdir):
        #    mkdirs(self._tmpdir)
        tmp_data_file = '_'.join([hash_path(self.account), \
                                  hash_path(self.account, self.container), \
                                  self._name_hash]) + self._data_ext
        tmp_meta_file = '_'.join([hash_path(self.account), \
                                  hash_path(self.account, self.container), \
                                  self._name_hash]) + self._meta_ext
        #need to check if we need to generate the error
        
        def _create_tmp_file(tmpdir, tmp_data_file, tmp_meta_file):
            if not exists(tmpdir):
                mkdirs(tmpdir)
            fd_data, tmppath_data = create_tmp_file(tmpdir, tmp_data_file)
            fd_meta, tmppath_meta = create_tmp_file(tmpdir, tmp_meta_file)
            return (fd_data, tmppath_data, fd_meta, tmppath_meta)
        
        (fd_data, tmppath_data, fd_meta, tmppath_meta) = \
            self._threadpool_write.force_run_in_thread( \
            _create_tmp_file, self._tmpdir, tmp_data_file, tmp_meta_file)

        _raw_rpipe, wpipe = os.pipe()
        rpipe = greenio.GreenPipe(_raw_rpipe, 'rb', bufsize=0)

        #fd_data, tmppath_data = create_tmp_file(self._tmpdir, tmp_data_file)
        #rfd_meta, tmppath_meta = create_tmp_file(self._tmpdir, tmp_meta_file)
        
            
        try:
            yield DiskFileWriter(self._name, self._name_hash, self._datadir, self._metadir, fd_data,
                    fd_meta, tmppath_data, tmppath_meta, rpipe, wpipe,
                    self._threadpool_write, self._object_lib, self._logger)
        finally:
            def _finally(fd_data, fd_meta, tmppath_data, tmppath_meta):
                try:
                    os.close(fd_data)
                    os.close(fd_meta)
                except OSError:
                    pass
                try:
                    os.unlink(tmppath_data)
                    os.unlink(tmppath_meta)
                except OSError:
                    pass

            self._threadpool_write.force_run_in_thread(_finally,
                    fd_data, fd_meta, tmppath_data, tmppath_meta)
            rpipe.close()
            os.close(wpipe)
            
    def write_metadata(self, metadata):
        """
        Write a block of metadata to a metadata file  without requiring the caller to
        create the object first. Supports POST behaviour.

        :param metadata: dictionary of metadata to be associated with the
                         object
        :raises DiskFileError: this implementation will raise DiskFileError:
        """
        def _write_metadata(metadata):
            try:
                meta_fp = os.open(self._meta_file, os.O_WRONLY|os.O_TRUNC)
                ##during post we require to set the 'name' metadata
                if 'name' not in metadata:
                    metadata['name'] = self._name
                write_metadata(meta_fp, metadata)
            finally:
                try:
                    os.close(meta_fp)
                except OSError:
                    pass
        self._threadpool_write.force_run_in_thread( \
                _write_metadata, metadata)            
                
        
            
    def reader(self, keep_cache=False, statInstance=None):
        """
        Return a :class:`osd.common.swob.Response` class compatible
        "`app_iter`" object as defined by
        :class:`osd.objectService.diskfile.DiskFileReader`.

        For this implementation, the responsibility of closing the open file
        is passed to the :class:`osd.objectService.diskfile.DiskFileReader` object.

        :param keep_cache: caller's preference for keeping data read in the
                           OS buffer cache
        :param _quarantine_hook: 1-arg callable called when obj quarantined;
                                 the arg is the reason for quarantine.
                                 Default is to ignore it.
                                 Not needed by the REST layer.
        :returns: a :class:`osd.objectService.diskfile.DiskFileReader` object
        """
        dr = DiskFileReader(
            self._fp, self._data_file, int(self._metadata['Content-Length']),
            self._metadata['ETag'], self._object_lib, self._disk_chunk_size,
            self._mgr.keep_cache_size, self._filesystem, self._logger, keep_cache=keep_cache, statInstance=statInstance)
        # At this point the reader object is now responsible for closing
        # the file pointer.
        self._fp = None
        return dr
    
    def delete(self, file_type):
        """
        Delete the object and metadata file.
        :param file_type: type of file to be deleted, data or meta
        :returns: True, if delete successful
                  False otherwise.
        """
        if file_type == 'data':
            try:
                os.unlink(self._data_file)
            except OSError:
                return False
        else:
            try:
                os.unlink(self._meta_file)
            except OSError:
                return False
        return True

    def is_exist(self, file_type):
        """
        Check the existeance of object and metadata file.
        :param file_type: type of file to check for existance, data or meta
        :returns: True, if exist successful
                  False otherwise.
        """
        if file_type == 'data':
            return os.path.exists(self._data_file)
        elif file_type == 'meta':
            return os.path.exists(self._meta_file)
        else:
            raise ValueError('Ivalid file type. It should be either "data" or "meta"')

    def _quarantine(self, data_file, msg):
        """
        Quarantine a file; responsible for incrementing the associated logger's
        count of quarantines.

        :param data_file: full path of data file to quarantine
        :param msg: reason for quarantining to be included in the exception
        :returns: DiskFileQuarantined exception object
        """
        #we will not be quarantining the data in our implementation.
        # we keep this method to simply raise the DiskFileQuarantined exception
        #self._quarantined_dir = self._threadpool.run_in_thread(
        #    quarantine_renamer, self._device_path, data_file)
        self._logger.warn("Quarantined object %s: %s" % (
            data_file, msg))
        #self._logger.increment('quarantines')
        return DiskFileQuarantined(msg)

    def _verify_name_matches_hash(self, data_file):
        hash_from_fs = os.path.basename(self._datadir)
        hash_from_name = hash_path(self._name.lstrip('/'))
        if hash_from_fs != hash_from_name:
            raise self._quarantine(
                data_file,
                "Hash of name in metadata does not match directory name")

    def _verify_data_file(self, data_file, fp):
        """
        Verify the metadata's name value matches what we think the object is
        named.

        :param data_file: data file name being consider, used when quarantines
                          occur
        :param fp: open file pointer so that we can `fstat()` the file to
                   verify the on-disk size with Content-Length metadata value
        :raises DiskFileCollision: if the metadata stored name does not match
                                   the referenced name of the file
        :raises DiskFileExpired: if the object has expired
        :raises DiskFileQuarantined: if data inconsistencies were detected
                                     between the metadata and the file-system
                                     metadata
        """
        try:
            mname = self._metadata['name']
        except KeyError:
            raise self._quarantine(data_file, "missing name metadata")
        else:
            if mname != self._name:
                self._logger.error(
                    _('Client path %(client)s does not match '
                      'path stored in object metadata %(meta)s'),
                    {'client': self._name, 'meta': mname})
                raise DiskFileCollision('Client path does not match path '
                                        'stored in object metadata')
        try:
            x_delete_at = int(self._metadata['X-Delete-At'])
        except KeyError:
            pass
        except ValueError:
            # Quarantine, the x-delete-at key is present but not an
            # integer.
            raise self._quarantine(
                data_file, "bad metadata x-delete-at value %s" % (
                    self._metadata['X-Delete-At']))
        else:
            if x_delete_at <= time.time():
                raise DiskFileExpired(metadata=self._metadata)
        try:
            metadata_size = int(self._metadata['Content-Length'])
        except KeyError:
            raise self._quarantine(
                data_file, "missing content-length in metadata")
        except ValueError:
            # Quarantine, the content-length key is present but not an
            # integer.
            raise self._quarantine(
                data_file, "bad metadata content-length value %s" % (
                    self._metadata['Content-Length']))
        fd = fp.fileno()
        try:
            statbuf = os.fstat(fd)
        except OSError as err:
            # Quarantine, we can't successfully stat the file.
            raise self._quarantine(data_file, "not stat-able: %s" % err)
        else:
            obj_size = statbuf.st_size
        if obj_size != metadata_size:
            raise self._quarantine(
                data_file, "metadata content-length %s does"
                " not match actual object size %s" % (
                    metadata_size, statbuf.st_size))
        self._content_length = obj_size
        return obj_size

    def _failsafe_read_metadata(self, source, quarantine_filename=None):
        # Takes source and filename separately so we can read from an open
        def _read_metadata(source, quarantine_filename=None):# file if we have one
            try:
                return read_metadata(source)
            except Exception as err:
                raise self._quarantine(
                    quarantine_filename,
                    "Exception reading metadata: %s" % err)
                
        return self._threadpool_write.force_run_in_thread(_read_metadata, source, quarantine_filename)



