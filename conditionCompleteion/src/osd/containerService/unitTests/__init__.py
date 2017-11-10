import logging
import sys
from collections import defaultdict
from osd.common.utils import LogAdapter
from osd.glCommunicator.service_info import ServiceInfo
from osd.glCommunicator.response import *
import httplib 

class FakeAccountRingData(object):
    def __init__(self, fs_list=['fs1', 'fs2'], \
                       account_level_dir_list=['a1', 'a2'], \
                       container_level_dir_list=['d0', 'd1', 'd2', 'd3'], \
                       account_service_list=[{'as_1': {'ip': '1.2.3.4', \
                                               'port': '0000'}}], \
                       comp_list=['x0'], \
                       comp_account_dist={"as_1": ["x0"]}):
        self.fs_list = fs_list
        self.account_level_dir_list = account_level_dir_list
        self.container_level_dir_list = container_level_dir_list
        self.account_service_list = account_service_list
        self.comp_list = comp_list
        self.comp_account_dist = comp_account_dist

class FakeAccountRing(object):
    def __init__(self, file_location):
        self.ring_data = FakeAccountRingData()

    def get_node(self, account):
        return [self.ring_data.account_service_list[0]['as_1']], \
            'fs1', account

    def get_service_list(self):
        return self.ring_data.account_service_list

class FakeContainerRingData(object):
    def __init__(self, fs_list=['fs1', 'fs2'], \
                       account_level_dir_list=['a1', 'a2'], \
                       container_level_dir_list=['d0', 'd1', 'd2', 'd3'], \
                       container_service_list=[{'cs_1': {'ip': '1.2.3.4', \
                                               'port': '0000'}}], \
                       comp_list=['x0'], \
                       comp_container_dist={"cs1": ["x0"]}):
        self.fs_list = fs_list
        self.account_level_dir_list = account_level_dir_list
        self.container_level_dir_list = container_level_dir_list
        self.container_service_list = container_service_list
        self.comp_list = comp_list
        self.comp_container_dist = comp_container_dist

class FakeContainerRing(object):
    def __init__(self, file_location):
        self.ring_data = FakeContainerRingData()

    def get_node(self, account, container):
        return [self.ring_data.container_service_list[0]['cs_1']], \
            'fs1', 'a1'

    def get_service_list(self):
        return self.ring_data.container_service_list

class FakeObjectRingData(object):
    def __init__(self, fs_list=['fs1', 'fs2'], \
                       dir_list=['d0', 'd1', 'd2', 'd3'], \
                       object_service_list=[{'os_1': {'ip': '1.2.3.4', \
                                            'port': '0000'}}]):
        self.fs_list = fs_list
        self.dir_list = dir_list
        self.object_service_list = object_service_list

    def get_fs_list(self):
        return self.fs_list

class FakeObjectRing(object):
    def __init__(self, file_location):
        self.ring_data = FakeObjectRingData()

    def get_fs_list(self):
        return self.ring_data.fs_list

    def get_directory(self, account, container, obj):
        return 'd0'

    def get_filesystem(self, directory):
        return 'fs0'

    def get_node(self, account, container, obj):
        return [self.ring_data.object_service_list[0]['os_1']], \
            'fs1', 'd0'

class FakeLogger(logging.Logger):
    # a thread safe logger

    def __init__(self, *args, **kwargs):
        self._clear()
        self.name = 'osd.unit.fake_logger'
        self.level = logging.NOTSET
        if 'facility' in kwargs:
            self.facility = kwargs['facility']
        self.statsd_client = None
        self.thread_locals = None

    def _clear(self):
        self.log_dict = defaultdict(list)
        self.lines_dict = defaultdict(list)

    def _store_in(store_name):
        def stub_fn(self, *args, **kwargs):
            self.log_dict[store_name].append((args, kwargs))
        return stub_fn

    def _store_and_log_in(store_name):
        def stub_fn(self, *args, **kwargs):
            self.log_dict[store_name].append((args, kwargs))
            self._log(store_name, args[0], args[1:], **kwargs)
        return stub_fn

    def get_lines_for_level(self, level):
        return self.lines_dict[level]

    error = _store_and_log_in('error')
    info = _store_and_log_in('info')
    warning = _store_and_log_in('warning')
    warn = _store_and_log_in('warning')
    debug = _store_and_log_in('debug')

    def exception(self, *args, **kwargs):
        self.log_dict['exception'].append((args, kwargs,
                                           str(sys.exc_info()[1])))
        print 'FakeLogger Exception: %s' % self.log_dict

    # mock out the StatsD logging methods:
    increment = _store_in('increment')
    decrement = _store_in('decrement')
    timing = _store_in('timing')
    timing_since = _store_in('timing_since')
    update_stats = _store_in('update_stats')
    set_statsd_prefix = _store_in('set_statsd_prefix')

    def get_increments(self):
        return [call[0][0] for call in self.log_dict['increment']]

    def get_increment_counts(self):
        counts = {}
        for metric in self.get_increments():
            if metric not in counts:
                counts[metric] = 0
            counts[metric] += 1
        return counts

    def setFormatter(self, obj):
        self.formatter = obj

    def close(self):
        self._clear()

    def set_name(self, name):
        # don't touch _handlers
        self._name = name

    def acquire(self):
        pass

    def release(self):
        pass

    def createLock(self):
        pass

    def emit(self, record):
        pass

    def _handle(self, record):
        try:
            line = record.getMessage()
        except TypeError:
            print 'WARNING: unable to format log message %r %% %r' % (
                record.msg, record.args)
            raise
        self.lines_dict[record.levelno].append(line)

    def handle(self, record):
        self._handle(record)

    def flush(self):
        pass

    def handleError(self, record):
        pass


class DebugLogger(FakeLogger):
    """A simple stdout logging version of FakeLogger"""

    def __init__(self, *args, **kwargs):
        FakeLogger.__init__(self, *args, **kwargs)
        self.formatter = logging.Formatter("%(server)s: %(message)s")

    def handle(self, record):
        self._handle(record)
        print self.formatter.format(record)


def debug_logger(name='test'):
    """get a named adapted debug logger"""
    return LogAdapter(DebugLogger(), name)


class FakeTransferCompMessage:
    def de_serialize(self, comp_list):
        self.comp_list = comp_list
        self.comp_map = {}
    
    def service_comp_map(self):
        #my_dict = {"ABC" : ['1', '2', '3'], "XYZ" : ['1', '2', '3'], "PQR" : ['1', '2', '3']}
        my_dict = {}
        my_dict[ServiceInfo(10, '1.1.1.1', 7890)] = [1, 2, 3]
        my_dict[ServiceInfo(101, '2.2.2.2', 6570)] = [7, 8, 9]
        my_dict[ServiceInfo(111, '6.6.6.6', 4321)] = [4, 5, 6]
        return my_dict 

def FakeComponentList():
    my_list = [1, 2, 3]
    return my_list

class FakeReq:
    def __init__(self, logger=None, timeout=60):
        self.__timeout = timeout
        self.__logger = logger or FakeLogger()

    def get_gl_info(self):
        return None

    def connector(self, conn_type, host_info):
        return 1

    def comp_transfer_info(self, service_obj, acc_cmp, conn_obj):
        return Result(Status(Resp.TIMEOUT, 'Timeout Expired'), None)

def FakeConnectTargetNode():
    conn = HTTPSConnection("1.1.1.1", 7890)
    conn.node = {}
    conn.node['ip'] = "1.1.1.1"
    conn.node['port'] = 7890
    return conn

def FakeSendData(self, conn):
    pass

def FakeGetConnResp(self, conn):
    conn.status = False
    conn.resp = False
    return (conn.resp, conn)

