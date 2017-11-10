"""
Provides common utility classes and functions
"""

import os
import sys
import traceback
import eventlet
from osd.accountUpdaterService.utils import DirtyLogger
from osd.common.utils import ThreadPool
from eventlet import greenio
from osd.common.ring.ring import get_all_service_id, get_fs_list, get_service_id

FAIL_ERROR = -1
FAIL_TIMEOUT = -2
WRITE_MODE = 1
READ_MODE = 2
HEADERS = ['x-container-read', 'x-container-write']
RING_DIR = '/export/.osd_meta_config'
TMPDIR = "tmp"
FILENAME = '/tmp/sock'
# Max expired time
MAX_EXP_TIME = 15 * 60

def releaseStdIOs(logger):
    fd = os.open("/dev/null", os.O_RDWR)
    stdio = [sys.stdin, sys.stdout, sys.stderr]
    for f in stdio:
        try:
            f.flush()
        except IOError:
            logger.warning("Got IO Error while performing flush on fd: %s" %f.fileno())
            pass
        try:
            os.dup2(fd, f.fileno())
        except OSError:
            logger.warning("Got OS Error while performing dup2 on fd: %s" %f.fileno())
            pass

    sys.stdout = DirtyLogger(logger)
    sys.stderr = DirtyLogger(logger)

    sys.excepthook = lambda * exc_info: \
        logger.critical("UNCAUGHT EXCEPTION: %s%s%s" %(exc_info[1], "".join(traceback.format_tb(exc_info[2])), exc_info[1]))


class OsdExceptionCode(object):
    """ Class to define exception codes """

    OSD_MOUNT_CHECK_FAILED = 10
    OSD_NOT_FOUND = 20
    OSD_FILE_ALREADY_EXIST = 30
    OSD_FILE_OPERATION_ERROR = 40
    OSD_INTERNAL_ERROR = 50
    OSD_BAD_CONFIG_ERROR = 60
    OSD_MAX_META_COUNT_REACHED = 70
    OSD_MAX_META_SIZE_REACHED = 71
    OSD_MAX_ACL_SIZE_REACHED = 73
    OSD_MAX_SYSTEM_META_SIZE_REACHED = 75
    OSD_BULK_DELETE_CONTAINER_FAILED = 77
    OSD_OPERATION_SUCCESS = 100
    OSD_INFO_FILE_ACCEPTED = 200
    OSD_BULK_DELETE_TRANS_ADDED = 201
    OSD_BULK_DELETE_CONTAINER_SUCCESS = 202
    OSD_FLUSH_ALL_DATA_SUCCESS = 402
    OSD_FLUSH_ALL_DATA_ERROR = 92

def get_filesystem(logger):
    """ Returns the filesystem for info file"""
    fs_list = get_fs_list(logger)
    return fs_list

def get_container_id():
    """ Function to get container service id """
    return get_service_id('container')

def get_all_container_id(logger):
    """ Function to get all container service id """
    return get_all_service_id('container', logger)

class SingletonType(type):
    """ Class to create singleton classes object"""
    __instance = None

    def __call__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(SingletonType, cls).__call__(*args, **kwargs)
        return cls.__instance

class AsyncLibraryHelper(object):
    """Wrap Synchronous libraries"""

    def __init__(self, lib):
        self.__lib = lib
        self.__running = True
        self.__waiting = {}
        self.__next_event_id = 1

    def wait_event_stop(self):
        self.__running = False

    def wait_event(self):
        __pool = ThreadPool()
        while self.__running:
            __ret = __pool.run_in_thread(self.__lib.event_wait)
            if (__ret > 0):
                self.__waiting[__ret].send()
                del self.__waiting[__ret]

    def call(self, func_name, *args):
        func = getattr(self.__lib, func_name)
        __event_id = self.__next_event_id
        self.__next_event_id += 1
        self.__waiting[__event_id] = eventlet.event.Event()
        ret = func(*(args + (__event_id,)))
        self.__waiting[__event_id].wait()
        return ret

class AsyncLibraryWithPipe(object):
    def __init__(self, lib):
        self.__lib = lib

    def call(self, func_name, *args):
        func = getattr(self.__lib, func_name)
        __rawpipe, __wpipe = os.pipe()
        __rpipe = greenio.GreenPipe(__rawpipe, 'rb', bufsize=0)
        ret = func(*(args + (__wpipe,)))
        _ = __rpipe.read(4)
        __rpipe.close()
        os.close(__wpipe)
        return ret

FILE_PATH_DOES_NOT_EXISTS = 2
FILE_DOES_NOT_EXISTS = 1
FILE_EXISTS = 0

def isFile(file_path, file_name):
	if not os.path.exists(file_path):
		return FILE_PATH_DOES_NOT_EXISTS
	if not os.path.isfile(file_path + file_name):
		return FILE_DOES_NOT_EXISTS
	return FILE_EXISTS
