import mock
import unittest
from osd.accountUpdaterService.unitTests import FakeGlobalVariables, FakeConnectionCreator, FakeConnection, fake_get_directory_by_hash, conf, logger, mockReq, mockResp, FakeRingFileReadHandler, mockServiceInfo
from osd.accountUpdaterService.handler import AccountInfo, AccountReport
from osd.accountUpdaterService.communicator import AccountServiceCommunicator


class TestAccountServiceCommunicator(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)    
    @mock.patch("osd.accountUpdaterService.communicator.ConnectionCreator", FakeConnectionCreator)
    def setUp(self):
        self.account_service_communicator = AccountServiceCommunicator(conf, logger)

    def test_update_container_stat_info_with_202(self):
       with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.get_http_connection_instance', return_value=FakeConnection(202)):
           self.assertEquals(self.account_service_communicator.update_container_stat_info("acc", "stat_info"), True)

    def test_update_container_stat_info_with_404(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.get_http_connection_instance', return_value=FakeConnection(404)):
           self.assertEquals(self.account_service_communicator.update_container_stat_info("acc", "stat_info"), True)

    def test_update_container_stat_info_with_500(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.get_http_connection_instance', return_value=FakeConnection(500)):
           self.assertEquals(self.account_service_communicator.update_container_stat_info("acc", "stat_info"), False)

    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    def test_update_container_stat_info_with_301(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.get_http_connection_instance', return_value=FakeConnection(301)):
           self.assertEquals(self.account_service_communicator.update_container_stat_info("acc", "stat_info"), False)
 
class TestAccountInfo(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        account_name = "acc"
        self.account_instance = AccountInfo(account_name, conf, logger)
        self.account_report = AccountReport(conf, logger)

    def tearDown(self):
        pass

    def test_getAccountName(self):
        self.assertEquals(self.account_instance.getAccountName(), "acc")

    def test_add_container(self):
        self.account_instance.add_container("cont0")
        self.account_instance.add_container("cont1")

    def test_is_container(self):
        self.account_instance.add_container("cont0")
        self.assertEquals(self.account_instance.is_container("cont0"), True)
        self.assertEquals(self.account_instance.is_container("cont2"), None)

    @mock.patch("osd.accountUpdaterService.handler.ConnectionCreator", FakeConnectionCreator)
    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)
    def test_get_container_path(self):
        self.account_instance.add_container("cont0")
        self.account_instance.add_container("cont1")
        self.account_instance.get_container_path()
        self.assertEquals(self.account_instance.account_map, {('acc', 'cont0'): 'start', ('acc', 'cont1'): 'start'})

    @mock.patch("osd.accountUpdaterService.handler.ConnectionCreator", FakeConnectionCreator)
    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)
    def test_execute(self):
        self.account_instance.add_container('cont0')
        self.account_instance.add_container('cont1')
        self.account_instance.get_container_path()
        self.account_instance.execute()
        self.assertEquals(self.account_instance.record_instance[0].stat_info , {'cont0': {'object_count': 'dummy', 'container': 'container', 'deleted': False, 'bytes_used': 'dummy', 'put_timestamp': 'dummy', 'delete_timestamp': '0'}})
        self.assertEquals(self.account_instance.record_instance[1].stat_info , {'cont1': {'object_count': 'dummy', 'container': 'container', 'deleted': False, 'bytes_used': 'dummy', 'put_timestamp': 'dummy', 'delete_timestamp': '0'}})

        self.assertEquals(self.account_instance.account_map, {('acc', 'cont0'): 'success', ('acc', 'cont1'): 'success'}) 

    @mock.patch("osd.accountUpdaterService.handler.ConnectionCreator", FakeConnectionCreator)
    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)     
    def test_execute_when_container_head_404(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.connect_container', return_value=FakeConnection(404)):
            self.account_instance.account_name = "acc" 
            self.account_instance.add_container("cont0")
            self.account_instance.add_container("cont1")
            self.account_instance.get_container_path()
            self.account_instance.execute()
            self.assertEquals(self.account_instance.record_instance[0].stat_info , {'cont0': {'object_count': 0, 'container': '', 'deleted': True, 'bytes_used': 0, 'put_timestamp': '0', 'delete_timestamp': '0'}})
            self.assertEquals(self.account_instance.record_instance[1].stat_info , {'cont1': {'object_count': 0, 'container': '', 'deleted': True, 'bytes_used': 0, 'put_timestamp': '0', 'delete_timestamp': '0'}})
            self.assertEquals(self.account_instance.account_map, {('acc', 'cont0'): 'success', ('acc', 'cont1'): 'success'})

    @mock.patch("osd.accountUpdaterService.handler.ConnectionCreator", FakeConnectionCreator)
    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)
    def test_execute_when_container_head_500(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.connect_container', return_value=FakeConnection(500)):
            self.account_instance.account_name = "acc1"
            self.account_instance.add_container("cont0")
            self.account_instance.add_container("cont1")
            self.account_instance.get_container_path()
            self.account_instance.execute()
            self.assertEquals(self.account_instance.record_instance[0].stat_info , {})
            self.assertEquals(self.account_instance.record_instance[1].stat_info , {})
            self.assertEquals(self.account_instance.account_map, {('acc1', 'cont0'): 'fail', ('acc1', 'cont1'): 'fail'})
    
    @mock.patch("osd.accountUpdaterService.handler.ConnectionCreator", FakeConnectionCreator)
    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)
    def test_execute_when_container_head_301(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.connect_container', return_value=FakeConnection(301)):
            self.account_instance.account_name = "acc1"
            self.account_instance.add_container("cont0")
            self.account_instance.add_container("cont1")
            self.account_instance.get_container_path()
            self.account_instance.execute()
            self.assertEquals(self.account_instance.record_instance[0].stat_info , {})
            self.assertEquals(self.account_instance.record_instance[1].stat_info , {})
            self.assertEquals(self.account_instance.account_map, {('acc1', 'cont0'): 'fail', ('acc1', 'cont1'): 'fail'})

    @mock.patch("osd.accountUpdaterService.handler.ConnectionCreator", FakeConnectionCreator)
    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)
    def test_execute_when_container_head_exception(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.connect_container', side_effect=Exception):
            self.account_instance.account_name = "acc1"
            self.account_instance.add_container("cont0")
            self.account_instance.add_container("cont1")
            self.account_instance.get_container_path()
            self.account_instance.execute()
            self.assertEquals(self.account_instance.record_instance[0].stat_info , {})
            self.assertEquals(self.account_instance.record_instance[1].stat_info , {})
            self.assertEquals(self.account_instance.account_map, {('acc1', 'cont0'): 'fail', ('acc1', 'cont1'): 'fail'})

    @mock.patch("osd.accountUpdaterService.handler.ConnectionCreator", FakeConnectionCreator)
    @mock.patch("osd.accountUpdaterService.monitor.GlobalVariables", FakeGlobalVariables)
    @mock.patch("osd.common.ring.ring.ContainerRing.get_directory_by_hash", fake_get_directory_by_hash)
    def test_execute_when_12_container_head_404(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.connect_container', return_value=FakeConnection(404)):
            self.account_instance.account_name = "acc"
            for i in range(12): 
                self.account_instance.add_container("cont%s" %i)
            self.account_instance.get_container_path()
            self.account_instance.execute()
            for i in range(12):
                stat_info = self.account_instance.record_instance[i].stat_info
                self.assertEquals(self.account_instance.record_instance[i].stat_info , {'cont%s' %i: {'object_count': 0, 'container': '', 'deleted': True, 'bytes_used': 0, 'put_timestamp': '0', 'delete_timestamp': '0'}})
            self.assertEquals(self.account_instance.account_map, {('acc', 'cont3'): 'success', ('acc', 'cont0'): 'success', ('acc', 'cont1'): 'success', ('acc', 'cont6'): 'success', ('acc', 'cont7'): 'success', ('acc', 'cont4'): 'success', ('acc', 'cont5'): 'success', ('acc', 'cont10'): 'success', ('acc', 'cont11'): 'success', ('acc', 'cont8'): 'success', ('acc', 'cont2'): 'success', ('acc', 'cont9'): 'success'})
    

if __name__ == '__main__':
    unittest.main()





