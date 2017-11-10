import unittest
import random
from osd.objectService.unitTests import FakeLogger
from osd.accountUpdaterService import server as account_updater
from osd.accountUpdaterService import communicator as account_communicator
from osd.accountUpdaterService import communicator as account_headers
from osd.accountUpdaterService import handler as account_parser
from osd.common.bufferedhttp import http_connect
import os


class TestContainerUpdater(unittest.TestCase):
    ACC_CALLED = False
    ACC_UPDATER = None
    TIMEOUT_CALLED = False
    ACCOUNT_UPDATER_TIMEOUT = None
    HTML_HEADER_BUILDER = None
    HTML_CALLED = False
    RING_CALLED = False
    SERVICE_LOCATOR = None
    CONECTION_CREATOR_CALLED = False
    CONECTION_CREATOR = None
    STAT_READER_CALLED = False
    CONTAINER_STAT_READER = None

    def setUp(self):
        pass
    def __get_account_updater_instance(self):
        if TestContainerUpdater.ACC_CALLED:
            return TestContainerUpdater.ACC_UPDATER
        else:
            TestContainerUpdater.ACC_UPDATER = account_updater.AccountUpdater(
                                                           {'interval': '1',
                                                           'node_timeout': '5',
                                                          'conn_timeout': '.5'}, logger = FakeLogger())
      
            TestContainerUpdater.ACC_CALLED = True
            return TestContainerUpdater.ACC_UPDATER

    def __get_HtmlHeaderBuilder_instance(self):
        if TestContainerUpdater.HTML_CALLED:
            return TestContainerUpdater.HTML_HEADER_BUILDER
        else:
            TestContainerUpdater.HTML_HEADER_BUILDER = \
                                 account_headers.HtmlHeaderBuilder()
            TestContainerUpdater.HTML_CALLED = True      
        return TestContainerUpdater.HTML_HEADER_BUILDER

    def __get_AccountUpdaterTimeout_instance(self):
        if TestContainerUpdater.TIMEOUT_CALLED:
            return TestContainerUpdater.ACCOUNT_UPDATER_TIMEOUT
        else:
            TestContainerUpdater.ACCOUNT_UPDATER_TIMEOUT = \
                                 account_communicator.AccountUpdaterTimeout()
            TestContainerUpdater.TIMEOUT_CALLED = True
            return TestContainerUpdater.ACCOUNT_UPDATER_TIMEOUT

    def __get_ServiceLocator_instance(self):
        if TestContainerUpdater.RING_CALLED:
            return TestContainerUpdater.SERVICE_LOCATOR
        else:
            TestContainerUpdater.SERVICE_LOCATOR = \
                                 account_communicator.ServiceLocator()
            TestContainerUpdater.RING_CALLED = True   
            return TestContainerUpdater.SERVICE_LOCATOR

    def __get_conection_creator_instance(self):
        if TestContainerUpdater.CONECTION_CREATOR_CALLED:
            return TestContainerUpdater.CONECTION_CREATOR
        else:
            TestContainerUpdater.CONECTION_CREATOR = \
                                 account_communicator.ConnectionCreator(
                                                         {'node_timeout': '5',
                                                          'conn_timeout': '.5'})
            TestContainerUpdater.CONECTION_CREATOR_CALLED = True    
            return TestContainerUpdater.CONECTION_CREATOR

    def __get_ContainerStatReader_instance(self):
        if TestContainerUpdater.STAT_READER_CALLED:
            return TestContainerUpdater.CONTAINER_STAT_READER
        else:
            TestContainerUpdater.CONTAINER_STAT_READER = \
                                     account_parser.ContainerStatReader(\
                                        'dk', '/remote/hydra043/dkushwaha/data')
            TestContainerUpdater.STAT_READER_CALLED = True
            return TestContainerUpdater.CONTAINER_STAT_READER    

    def test_logger_instance(self):
        cu = self.__get_account_updater_instance()
        self.assert_(hasattr(cu, 'logger'))
        self.assert_(cu.logger is not None)

    def test_container_service_instance(self):
        cu = self.__get_account_updater_instance() 
        self.assert_(hasattr(cu, 'account'))
        self.assert_(cu.account_updater is not None)

    def test_parser_instance(self):
        cu = self.__get_account_updater_instance()
        self.assert_(hasattr(cu, 'account_updater')) 
        self.assert_(cu.account_updater is not None)

    def test_container_service_instance(self):
        cu = self.__get_account_updater_instance()
        self.assert_(hasattr(cu, 'account_map'))
        self.assert_(cu.account_map is not None)

    def test_get_headers(self):
        bh = self.__get_HtmlHeaderBuilder_instance() 
        self.assertRaises(TypeError, bh.get_headers)
        dic = {'X-Account': [],
               'user-agent': os.getpid()}
        headers = bh.get_headers(header_info = [])
	self.assert_(headers is not None)
	#self.assertDictEqual(dic, headers)        

    def test_get_connection_timeout(self):
        to = self.__get_AccountUpdaterTimeout_instance()
        conn_timeout = .5 
        timeout = to.get_connection_timeout(conn_timeout)
        self.assert_(timeout is not None) 

    def test_get_node_timeout(self):
        to = self.__get_AccountUpdaterTimeout_instance()
        node_timeout = 3
        self.assertRaises(TypeError, to.get_node_timeout(node_timeout))

    def test_get_http_connection_instance(self):
        conn_creator = self.__get_conection_creator_instance()
        cu = self.__get_account_updater_instance()
        info = {
             'recovery_flag': True,
             'method_name': 'GET_LIST',
             'entity_name': '',     
             'header_info': '',
             'account_name': '',
             'flag':  True}
        conn = conn_creator.get_http_connection_instance(info, cu.logger)
        self.assert_(conn is not None) 
        #resp = conn_creator.get_http_response_instance(conn, cu.logger)
        #self.assertEquals(resp.status, 405) 
    
    def test_read_container_stat_info(self):
        sr = self.__get_ContainerStatReader_instance()     
        account_name = 'AUTH_Test'
        sr.read_container_stat_info()
        info = sr.stat_info
        self.assert_(info is not None)         


if __name__ == '__main__':
   unittest.main()

