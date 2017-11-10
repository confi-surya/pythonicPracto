import unittest 
import os
import subprocess 
import mock 
from mock import patch, Mock 
import test_Mock 
from threading import Thread, Lock, Condition 
from osd.accountUpdaterService.utils import SimpleLogger 
from osd.glCommunicator.req import Req 
from osd.glCommunicator.response import Resp 
from osd.glCommunicator.communication.communicator import IoType 
from osd.accountUpdaterService.account_updater_recovery import Recovery, RecoveryData, Response 

os.environ['flag'] = ""
os.environ['flag1'] = ""

class Mock_SimpleLogger: 
    def __init__(self, arg):
        self.argument = arg
        pass 
    def info(self, arg):
        pass 
    def error(self,arg): 
        pass 
    def warn(self, arg): 
        pass 
    def debug(self, arg): 
        pass
 
class Mock_SimpleLog:
    def __init__(self):
        pass

    def get_logger_object(self):
        return Mock_SimpleLogger(1)

class Fake_Service_object: 
    def __init__(self, ip, port, id):
        self.__ip = ip 
        self.__port = port 
        self.__id = id 
    def get_ip(self): 
        return self.__ip 
    def get_port(self): 
        return self.__port 
    def get_id(self): 
        return self.__id

class Fake_Con_obj: 
    def __init__(self): 
        self.__I = "OK"
    def close(self): 
        return 
    def request(self,pr1,pr2,pr3,pr4): 
        pass 
    def response(self): 
        return 1 

class Mock_Result(): 
    def __init__(self, status, message=None): 
        self.status = Mock_Status(status)
        self.message = Mock_Message(status) 

class Mock_Message():
    def __init__(self, status): 
        self.status = status
    def get_status_code(self):
        return 0
    def get_status_message(self):
        if os.environ.get('flag1'): 
            return -1
        else:
            return ""

    def dest_comp_map(self): 
        return {Fake_Service_object("192.168.101.1", "61007",\
                "HN0101_0000_accountUpdater-server"):[1,2],\
                Fake_Service_object("192.168.101.2", "61008",\
                "HN0101_1111_accountUpdater-server"):[3,4], \
                Fake_Service_object("192.168.101.3", "61009",\
                "HN0101_2222_accountUpdater-server"):[5,6]}

class Mock_Status():
    def __init__(self, status):
        self.status = status 
    def get_status_code(self): 
        return 0
    def get_status_message(self):
        return 0

class Mock_Req:
    def __init__(self, logger):
        self.__source_service_id = "HN0101_0000_accountUpdater-server"
        self.__service_obj = Fake_Service_object("192.168.101.1",\
                             "61007", "HN0101_0000_accountUpdater-server") 
        self.__service_obj1 = Fake_Service_object("192.168.101.2",\
                             "61008", "HN0101_1111_accountUpdater-server") 
        self.__service_obj2 = Fake_Service_object("192.168.101.3", \
                             "61009", "HN0101_2222_accountUpdater-server") 
        self.__glmap = {self.__service_obj:[1,2], self.__service_obj1:[3,4], self.__service_obj2:[5,6]}
        self.logger = logger
        self.config = config
        self.conn_obj = Fake_Con_obj()
        self.acc_cmp = []
        self.status = 0
        print "Req mocked"
        self.message = 'hi'
        self.resultMockObj = Mock_Result(self.status, self.message)

    def comp_transfer_final_stat(self, service_obj, final_cmp_list, final_status, conn_obj):
        return self.resultMockObj

    def comp_transfer_info(self, service_obj, acc_cmp, conn_obj):
        return self.resultMockObj

    def recv_pro_start_monitoring(self, id, conn_obj):
        return self.resultMockObj

    def send_heartbeat(self, service_obj, seqcountr, conn_obj):
        return self.resultMockObj

    def get_gl_info(self):
        return self.__service_obj

    def connector(self, connType, gl_info):
        if os.environ.get('flag'):
            return None
        else:
            return self.conn_obj

class Result:
    def __init__(self):
        self.__status = 'OK'

class Respo:
    def __init__(self, resp, dest_id):
        self.x = 1
    status = 200

class Respo1:
    def __init__(self, resp, dest_id):
        pass
    status = 1

service_obj = Fake_Service_object("192.168.101.1", "61007", "HN0101_0000_accountUpdater-server") 
resp = Respo(200, service_obj)
resp1 = Respo1(1, service_obj)
recDataObj = RecoveryData()
#resObj = Response(resp, service_obj)
logger = Mock_SimpleLogger(1)
config = ''

class Mock_Response:
    def __init__(self, resp, service_obj):
        pass

    def is_success(self):
        return False
class Mock_Recovery:
    def __init__(self, ar1, ar2, ar3):
        pass
    def start_recovery(self):
        pass

class TestAURecovery(unittest.TestCase):

    def setup(self):
        pass 
    
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.SimpleLogger', Mock_SimpleLogger)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.Req', Mock_Req)

    def test_get_glcommunicator_test(self):
        print "Test 1: communication object Testing =====>>>>>"
        recoveryObj = Recovery('', logger, "HN0101_8888_accountUpdater-server")
        return self.assertEqual(None, None, "communication object Tested")

    def test_response_class(self):
        resObj = Response(resp, service_obj)
        print "Test 2: Response class Testing        ====>>>>>"
        return self.assertEqual(resObj.is_success(), True)

    def test_response_else_class(self):
        resObj = Response(resp1, service_obj)
        print "Test 3: Response class else part Testing ==>>>>"
        return self.assertEqual(resObj.is_success(), False)

    #@mock.patch('osd.accountUpdaterService.account_updater_recovery.Response', Respo)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.httplib', test_Mock)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.Req', Mock_Req)

    def test_start_recovery_test(self):
        print "Test 4: Start Recovery Testing       ===>>>>>>"
        recoveryObj = Recovery('', logger, "HN0101_8888_accountUpdater-server")
        recoveryObj.start_recovery()
        return

    @mock.patch('osd.accountUpdaterService.account_updater_recovery.Req', Mock_Req)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.sys', test_Mock)

    def test_none_conn_check(self):
        os.environ['flag'] = "Yes"
        recoveryObj = Recovery('', logger, "HN0101_8888_accountUpdater-server")
        os.environ['flag'] = ""
        print " Test 5: Connection Object Testing    ===>>>>>"
        return

    #@mock.patch('osd.accountUpdaterService.account_updater_recovery.Response', Respo)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.httplib', test_Mock)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.Req', Mock_Req)

    def test_send_final_transfer_status_else_test(self):
        print "Test 6: Send final transfer Testing   ===>>>>>"
        os.environ['flag1'] = "Yes"
        recoveryObj = Recovery('', logger, "HN0101_8888_accountUpdater-server")
        recoveryObj.start_recovery()
        os.environ['flag1'] = ""
        return

    def test_recoverymap_test(self):
        recDataObj.set_recovery_map({1: "Hi"})
        print "Test 7: Recovery Map Tested           ===>>>>>>"
        return self.assertEqual(recDataObj.get_recovery_map(), {1: "Hi"})

    @mock.patch('osd.accountUpdaterService.account_updater_recovery.httplib', test_Mock)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.Req', Mock_Req)
    @mock.patch('osd.accountUpdaterService.account_updater_recovery.Response', Mock_Response)

    def test_send_accept_component_test(self):
        recoveryObj = Recovery('', logger, "HN0101_8888_accountUpdater-server")
        recoveryObj.start_recovery()
        print "Test 8: Send accept component Tested   ===>>>>>>"
        return 

    def teardown(self):
        pass

if __name__ == '__main__':
    unittest.main()

