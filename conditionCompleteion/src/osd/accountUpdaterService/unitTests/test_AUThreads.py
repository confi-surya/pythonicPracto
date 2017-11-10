import os
import mock
import time
import unittest
from threading import *
from tempfile import mkdtemp
from osd.accountUpdaterService.updater import Updater
from osd.accountUpdaterService.walker import Walker
from osd.accountUpdaterService.reader import Reader
from osd.accountUpdaterService.accountSweeper import AccountSweep
from osd.accountUpdaterService.containerSweeper import ContainerSweeper
from osd.accountUpdaterService.monitor import WalkerMap, ReaderMap, \
GlobalVariables
#from osd.accountUpdaterService.unitTests import FakeContainerDispatcher, FakeAccountInfo
from osd.accountUpdaterService.unitTests import mockReq, mockStatus, mockResp, mockMapData, FakeRingFileReadHandler, mockServiceInfo, mockResult, conf, logger, FakeGlobalVariables
'''
tempo_dir = mkdtemp()
path = os.path.join(tempo_dir, "OSP_01", "tmp", "account", "1", "del")

def mkdirs(path):
    if not os.path.isdir(path):
        try:
            os.makedirs(path)
        except OSError as err:
            if err.errno != errno.EEXIST or not os.path.isdir(path):
                raise

mkdirs(path)
'''
walker_map = WalkerMap()
reader_map = ReaderMap()
def fake_kill_thread(obj):
    if obj.isAlive():
        try:
            obj._Thread__stop()
            print(str(obj.getName()) + ' terminated')
        except:
            print(str(obj.getName()) + ' could not be terminated')
    obj.join()

class TestWalker(unittest.TestCase):
    @mock.patch('osd.accountUpdaterService.walker.GlobalVariables', FakeGlobalVariables)
    def setUp(self):
        self.walker_obj = Walker(walker_map, conf, logger)
    
    def test_walker_run(self):
        print "test_walker_run"
        self.walker_obj.setDaemon(True) 
        self.walker_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.walker_obj._walker_map, ['10/2015-09-08.05:51:24', '10/2015-09-08.05:51:25'])
        fake_kill_thread(self.walker_obj)
   
    def test_walker_run_exception(self):
        print "test_walker_run_exception"
        with mock.patch('osd.accountUpdaterService.walker.os.listdir', side_effect=Exception):
            self.walker_obj.setDaemon(True)
            self.walker_obj.start()
            time.sleep(0.5)
            self.assertEquals(self.walker_obj._walker_map, ['10/2015-09-08.05:51:24', '10/2015-09-08.05:51:25'])
            fake_kill_thread(self.walker_obj)
 
    def test_walker_stop_service_event(self):
        print "test_walker_stop_service_event"
        self.walker_obj._complete_all_event.set()
        self.walker_obj.setDaemon(True)
        self.walker_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.walker_obj._walker_map, [])
        fake_kill_thread(self.walker_obj)
        self.walker_obj._complete_all_event.clear()
    
    def test_walker_transfer_comp_event(self):
        print "test_walker_transfer_comp_event"
        self.walker_obj._transfer_cmp_event.set()
        self.walker_obj.setDaemon(True)
        self.walker_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.walker_obj._walker_map, [])
        self.assertEquals(self.walker_obj.msg.get_sweeper_list(), [('4', '2015-09-08.05:51:28'), ('5', '2015-09-08.05:51:29'), ('5', '2015-09-08.05:51:28'), ('4', '2015-09-08.05:51:27'), ('2', '2015-09-08.05:51:26'), ('2', '2015-09-08.05:51:25'), ('3', '2015-09-08.05:51:27'), ('3', '2015-09-08.05:51:26')])
        fake_kill_thread(self.walker_obj)
        self.walker_obj._transfer_cmp_event.clear()

class TestReader(unittest.TestCase):
    def setUp(self):
        self.msg = GlobalVariables(logger)
        walker_map[:] = ['1/2015-09-08.05:51:24']
        self.reader_obj = Reader(walker_map, reader_map, conf, logger)

    def test_reader_run(self):
        print "test_reader_run" 
        self.reader_obj.setDaemon(True) 
        self.reader_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.reader_obj._reader_map, {'1': {'2015-09-08.05:51:24': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dfcc8f269e5752ebea'], 'e3891c4148ce912256a07a50b69a8f48': ['082287da66aa3c25b281e06ae721a6d0', '5df2383886036d49657789bda3702b44']}}})
        fake_kill_thread(self.reader_obj)
   
    def test_reader_run_exception(self):
        print "test_reader_run_exception"
        with mock.patch('osd.accountUpdaterService.walker.os.path.exists', side_effect=Exception):
            self.reader_obj.setDaemon(True)
            self.reader_obj.start()
            time.sleep(0.5)
            self.assertEquals(self.reader_obj._reader_map, {'1': {'2015-09-08.05:51:24': {}}})
            fake_kill_thread(self.reader_obj)
 
    def test_reader_stop_service_event(self):
        print "test_Reader_stop_service_event"
        self.reader_obj._complete_all_event.set()
        self.reader_obj.setDaemon(True)
        self.reader_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.reader_obj._reader_map, {})
        fake_kill_thread(self.reader_obj)
        self.reader_obj._complete_all_event.clear()
    
    def test_reader_transfer_comp_event(self):
        print "test_Reader_transfer_comp_event"
        self.msg.set_ownershipList(range(1,10))
        self.reader_obj._transfer_cmp_event.set()
        self.reader_obj.setDaemon(True)
        self.reader_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.reader_obj._reader_map, {'1': {'2015-09-08.05:51:24': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dfcc8f269e5752ebea'], 'e3891c4148ce912256a07a50b69a8f48': ['082287da66aa3c25b281e06ae721a6d0', '5df2383886036d49657789bda3702b44']}}})
        fake_kill_thread(self.reader_obj)
        self.reader_obj._transfer_cmp_event.clear()

    def test_reader_transfer_comp_event_without_new_ownership(self):
        print "test_Reader_transfer_comp_event_without_new_ownership"
        self.msg.set_ownershipList([])
        self.reader_obj._transfer_cmp_event.set()
        self.reader_obj.setDaemon(True)
        self.reader_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.reader_obj._reader_map, {})
        fake_kill_thread(self.reader_obj)
        self.reader_obj._transfer_cmp_event.clear()

class FakeContainerDispatcher:
    def __init__(self, conf, logger):
        pass
 
    def dispatcher_executer(self, account_instance):
        #print "dispatcher_executer", account_instance
        pass

    def account_update(self, account_instance):
        #print "account_update", account_instance
        return True

def fake_get_container_path(self):
    #print "fake_get_container_path"
    pass 

class TestUpdater(unittest.TestCase):
    def setUp(self):
        pass
 
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    @mock.patch('osd.accountUpdaterService.accountSweeper.GlobalVariables', FakeGlobalVariables)
    @mock.patch('osd.accountUpdaterService.updater.AccountInfo.get_container_path', fake_get_container_path)
    @mock.patch('osd.accountUpdaterService.updater.ContainerDispatcher', FakeContainerDispatcher)
    def test_updater_run(self):
        print "test_updater_run"
        walker_map[:] = ['1/2015-09-08.05:51:24']
        reader_map = {'1': {'2015-09-08.05:51:24': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dfcc8f269e5752ebea'], 'e3891c4148ce912256a07a50b69a8f48': ['082287da66aa3c25b281e06ae721a6d0', '5df2383886036d49657789bda3702b44']}}, '2': {'2015-09-08.05:51:26': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dacc8f269e5752ebea']}}}
        self.updater_obj = Updater(walker_map, reader_map, \
                                conf, logger)
        self.updater_obj.setDaemon(True) 
        self.updater_obj.start()
        time.sleep(1)
        self.assertEquals(self.updater_obj._updater_map, {})
        fake_kill_thread(self.updater_obj)

def fake_remove(path):
    #print "fake_remove: ", path
    pass

def fake_delete_dir(self, path):
    #print "fake_delete_dir: ", path
    return True

class TestAccountSweep(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    @mock.patch('osd.accountUpdaterService.accountSweeper.GlobalVariables', FakeGlobalVariables)
    def setUp(self):
        self.sweeper_obj = AccountSweep(conf, logger)
    
    @mock.patch('osd.accountUpdaterService.accountSweeper.os.remove', fake_remove)
    @mock.patch('osd.accountUpdaterService.accountSweeper.AccountSweep._delete_dir', fake_delete_dir)
    def test_accountSweeper(self):
        print "test_accountSweeper"
        self.sweeper_obj.setDaemon(True)
        self.sweeper_obj.start()
        time.sleep(0.5)
        fake_kill_thread(self.sweeper_obj)        
   
    @mock.patch('osd.accountUpdaterService.accountSweeper.os.remove', fake_remove)
    @mock.patch('osd.accountUpdaterService.accountSweeper.AccountSweep._delete_dir', fake_delete_dir)
    def test_accountSweeper_exception_get_del_files(self):
        print "test_accountSweeper_exception_get_del_files"
        with mock.patch('osd.accountUpdaterService.accountSweeper.os.listdir', side_effect=Exception):
            self.sweeper_obj.setDaemon(True)
            self.sweeper_obj.start()
            time.sleep(0.5)
            fake_kill_thread(self.sweeper_obj)
 
    @mock.patch('osd.accountUpdaterService.accountSweeper.os.remove', fake_remove)
    def test_accountSweeper_exception_delete_dir(self):
        print "test_accountSweeper_exception_delete_dir"
        with mock.patch('osd.accountUpdaterService.accountSweeper.AccountSweep._delete_dir', side_effect=Exception):
            self.sweeper_obj.setDaemon(True)
            self.sweeper_obj.start()
            time.sleep(0.5)
            fake_kill_thread(self.sweeper_obj)        

    @mock.patch('osd.accountUpdaterService.accountSweeper.os.remove', fake_remove)
    def test_accountSweeper_exception_delete_dir_False(self):
        print "test_accountSweeper_exception_delete_dir_False"
        with mock.patch('osd.accountUpdaterService.accountSweeper.AccountSweep._delete_dir', return_value=False):
            self.sweeper_obj.setDaemon(True)
            self.sweeper_obj.start()
            time.sleep(0.5)
            fake_kill_thread(self.sweeper_obj)

    @mock.patch('osd.accountUpdaterService.accountSweeper.AccountSweep._delete_dir', fake_delete_dir)
    def test_accountSweeper_exception_os_remove(self):
        print "test_accountSweeper_exception_os_remove"
        with mock.patch('osd.accountUpdaterService.accountSweeper.os.remove', side_effect=Exception):
            self.sweeper_obj.setDaemon(True)
            self.sweeper_obj.start()
            time.sleep(0.5)
            fake_kill_thread(self.sweeper_obj)  

    def test_accountSweep_stop_service_event(self):
        print "test_accountSweep_stop_service_event"
        self.sweeper_obj._complete_all_event.set()
        self.sweeper_obj.setDaemon(True)
        self.sweeper_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.sweeper_obj.isAlive(), False)
        self.sweeper_obj._complete_all_event.clear()
    
    @mock.patch('osd.accountUpdaterService.accountSweeper.os.remove', fake_remove)
    @mock.patch('osd.accountUpdaterService.accountSweeper.AccountSweep._delete_dir', fake_delete_dir)
    def test_accountSweep_transfer_comp_event(self):
        print "test_accountSweep_transfer_comp_event"
        self.msg = FakeGlobalVariables(logger)
        self.msg.set_ownershipList(range(1,10))
        self.sweeper_obj._transfer_cmp_event.set()
        self.sweeper_obj.setDaemon(True)
        self.sweeper_obj.start()
        time.sleep(0.5)
        fake_kill_thread(self.sweeper_obj)     

def fake_os_unlink(file_name):
    #print "fake_unlink:" , file_name
    pass

class TestContainerSweeper(unittest.TestCase):
    @mock.patch('osd.accountUpdaterService.containerSweeper.GlobalVariables', FakeGlobalVariables)
    def setUp(self):
        walker_map[:] = ['1/2015-09-08.05:51:24', '2/2015-09-08.05:51:26']
        reader_map = {'1': {'2015-09-08.05:51:24': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dfcc8f269e5752ebea'], 'e3891c4148ce912256a07a50b69a8f48': ['082287da66aa3c25b281e06ae721a6d0', '5df2383886036d49657789bda3702b44']}}, '2': {'2015-09-08.05:51:26': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dacc8f269e5752ebea']}}}
        self.cont_sweeper_obj = ContainerSweeper(walker_map, reader_map, conf, logger)
        self.cont_sweeper_obj.msg = FakeGlobalVariables(logger)
        self.cont_sweeper_obj.msg.create_sweeper_list(('1', '2015-09-08.05:51:24'))
        self.cont_sweeper_obj.msg.create_sweeper_list(('2', '2015-09-08.05:51:26'))

    @mock.patch('osd.accountUpdaterService.containerSweeper.os.unlink', fake_os_unlink)
    def test_containerSweeper(self):
        print "test_containerSweeper"
        self.cont_sweeper_obj.setDaemon(True)
        self.cont_sweeper_obj.start()
        time.sleep(0.5)
        self.assertEquals(self.cont_sweeper_obj._walker_map, [])
        self.assertEquals(self.cont_sweeper_obj._reader_map, {})
        self.assertEquals(self.cont_sweeper_obj.msg.get_sweeper_list(), [])
        fake_kill_thread(self.cont_sweeper_obj)
    
    def test_containerSweeper_exception(self):
        print "test_containerSweeper_exception"
        with mock.patch('osd.accountUpdaterService.containerSweeper.remove_file', side_effect=Exception):
            self.cont_sweeper_obj.setDaemon(True)
            self.cont_sweeper_obj.start()
            time.sleep(0.5)
            self.assertEquals(self.cont_sweeper_obj._walker_map, ['1/2015-09-08.05:51:24', '2/2015-09-08.05:51:26'])
            self.assertEquals(self.cont_sweeper_obj._reader_map, {'1': {'2015-09-08.05:51:24': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dfcc8f269e5752ebea'], 'e3891c4148ce912256a07a50b69a8f48': ['082287da66aa3c25b281e06ae721a6d0', '5df2383886036d49657789bda3702b44']}}, '2': {'2015-09-08.05:51:26': {'da5599f9ed2104bd4cfbbfa723b45fd8': ['945c7a7b3a4330dacc8f269e5752ebea']}}})
            self.assertEquals(self.cont_sweeper_obj.msg.get_sweeper_list(), [])
            fake_kill_thread(self.cont_sweeper_obj)
    
if __name__ == '__main__':
    unittest.main()
