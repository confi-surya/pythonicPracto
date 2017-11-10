import mock
import unittest
from osd.accountUpdaterService.unitTests import FakeConnectionCreator, FakeConnection, fake_get_directory_by_hash, FakeHTTPConnection, FakeResponse, conf, logger 
from osd.accountUpdaterService.unitTests.test_monitor import cont_map, acc_map
from osd.accountUpdaterService.communicator import AccountServiceCommunicator, ServiceLocator, ConnectionCreator
from osd.accountUpdaterService.unitTests import mockReq, mockResp, FakeRingFileReadHandler, mockServiceInfo, FakeGlobalVariables

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

    def test_update_container_stat_info_with_exception(self):
        with mock.patch('osd.accountUpdaterService.unitTests.FakeConnectionCreator.get_http_connection_instance', side_effect=Exception):
           self.assertEquals(self.account_service_communicator.update_container_stat_info("acc", "stat_info"), False)

class TestConnectionCreator(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.conn_obj = ConnectionCreator(conf, logger)

    @mock.patch('osd.accountUpdaterService.communicator.httplib.HTTPConnection', FakeHTTPConnection)
    def test_connect_container(self):
        with mock.patch('osd.accountUpdaterService.communicator.ServiceLocator.get_container_from_ring', return_value={'ip': '192.168.123.13', 'port': '3128'}):
            conn = self.conn_obj.connect_container("HEAD", "acc", "082287da66aa3c25b281e06ae721a6d0", "cont_path")
            self.assertTrue(isinstance(conn, FakeHTTPConnection))

    @mock.patch('osd.accountUpdaterService.communicator.httplib.HTTPConnection', FakeHTTPConnection)
    def test_get_http_connection_instance(self):
        stat_info = {'cont': ''}
        info = {'recovery_flag': False, 'method_name': 'PUT_CONTAINER_RECORD', 'entity_name': '', 'body_size': len(str(stat_info)), 'account_name':"e3891c4148ce912256a07a50b69a8f48", 'flag':  True}
        with mock.patch('osd.accountUpdaterService.communicator.ServiceLocator.get_container_from_ring', return_value={'ip': '192.168.123.13', 'port': '3128', 'fs':'OSP_01', 'dir':'dir1'}):
            conn = self.conn_obj.get_http_connection_instance(info, stat_info)
            self.assertTrue(isinstance(conn, FakeHTTPConnection))

        with mock.patch('osd.accountUpdaterService.communicator.ServiceLocator.get_account_from_ring', return_value={'ip': '192.168.123.13', 'port': '3128', 'fs':'OSP_01', 'dir':'dir1'}):
            info['flag'] = False
            conn = self.conn_obj.get_http_connection_instance(info, stat_info)
            self.assertTrue(isinstance(conn, FakeHTTPConnection))

    def test_get_http_response_instance(self):
        conn = FakeConnection()
        resp = self.conn_obj.get_http_response_instance(conn)
        self.assertTrue(isinstance(resp, FakeResponse))     

class TestServiceLocator(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.service_locator = ServiceLocator(conf['osd_dir'], logger)
           
    def test_get_container_from_ring(self):
        with mock.patch("osd.accountUpdaterService.monitor.GlobalVariables.get_container_map", return_value = cont_map):
            self.assertEquals(self.service_locator.get_container_from_ring("e3891c4148ce912256a07a50b69a8f48", "082287da66aa3c25b281e06ae721a6d0"), {'ip': '474.474.474.474', 'fs': 'OSP_01', 'port': '474474', 'dir': 'a08/d1'})
      
    def test_get_account_from_ring(self):
        with mock.patch("osd.accountUpdaterService.monitor.GlobalVariables.get_account_map", return_value = acc_map):
            self.assertEquals(self.service_locator.get_account_from_ring("e3891c4148ce912256a07a50b69a8f48"), {'ip': '65.65.65.65', 'account': 'e3891c4148ce912256a07a50b69a8f48', 'fs': 'OSP_01', 'port': '6565', 'dir': 'a08'}) 

if __name__ == '__main__':
    unittest.main()

