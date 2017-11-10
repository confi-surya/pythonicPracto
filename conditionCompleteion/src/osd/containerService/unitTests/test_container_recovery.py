import sys
import os

sys.path.append("../")
sys.path.append("../../../../src")
sys.path.append("../../../../bin/release_prod_64")
homePath = os.environ["HOME"]
os.environ["OBJECT_STORAGE_CONFIG"] = homePath + "/osd_config"
import unittest
import eventlet
import mock


from Queue import Queue as queue

#import mox
from osd.containerService.container_recovery import RecoveryMgr,Recovery
import osd.containerService.container_recovery
from libContainerLib import ContainerStat, LibraryImpl, ObjectRecord, ListObjectWithStatus, ContainerStatWithStatus,Status,TransferObjectRecord
import libContainerLib
from osd.containerService.utils import AsyncLibraryHelper
import time
import libraryUtils
from subprocess import check_call
from osd.common.swob import Request, Response
import libTransactionLib      #account lib path
import string
import shutil
import threading
from threading import Thread
from osd.common.utils import get_logger
import time
import subprocess
libraryUtils.OSDLoggerImpl("container-library").initialize_logger()
from subprocess import check_call


path = os.getcwd()

g=0 #global value

id=72057594037927936

if not os.path.exists("./JournalDir"):
        os.mkdir("./JournalDir")
#transObj = libTransactionLib.TransactionLibrary('./JournalDir',1)
#compo = libContainerLib.list_less__component_names__greater_()
#compo = libTransactionLib.list_less__recordStructure_scope_ContainerRecord__greater_()
#compo.append(1)
#transObj.start_recovery(compo)
conf = {'filesystems' : '/export', 'mount_check' : 'true', 'disable_fallocate' : 'true', 'bind_port' : 61007, 'workers' : 1, 'user' : 'root' ,'log_facility' : 'LOG_LOCAL6', 'recon_cache_path' : '/var/cache/swift', 'eventlet_debug' : 'true', 'log_level' : 'info', 'journal_path_trans' : '/export/transaction_journal', 'journal_path_cont' : '/export/container_journal', 'conn_timeout' : 100 ,'node_timeout' :500, 'CLIENT_TIMEOUT': 10}


ob = libraryUtils.OSDLoggerImpl("container-library") # for logging
ob.initialize_logger()
if  not (os.path.exists(os.getcwd() + "/Journal")):
        os.makedirs(os.getcwd() + "/Journal")
path=os.getcwd()+"/"
s=path.split('/')
tmp_path = os.getcwd()+"/tmp"
contObj = LibraryImpl("./Journal",1)
helper = AsyncLibraryHelper(contObj)
pool = eventlet.GreenPool()
pool.spawn_n(helper.wait_event)
status = Status()
container_stat_obj = ContainerStatWithStatus()
components = libContainerLib.list_less__component_names__greater_()
components.append(1)
components.append(2)
res = contObj.start_recovery(components)
MaxEntry = 100


def mock_http_connect(response, with_exc=False):
    class FakeConn(object):
	def __init__(self, status, with_exc):
	    self.status = status
	    self.reason = 'Fake'
	    self.host = '1.2.3.4'
	    self.port = '1234'
	    self.with_exc = with_exc
	def getresponse(self):
	    if self.with_exc:
	        raise Exception('test')
	    return self
	def getexpect(self):
	    return self
	def read(self, amt=None):
	    return ''
    return lambda *args, **kwargs: FakeConn(response, with_exc)


def mock_http_connection(response, node, with_exc=False):
    class FakeConn(object):
        def __init__(self, status, with_exc):
            self.status = status
            self.reason = 'Fake'
            self.host = '1.2.3.4'
            self.port = '1234'
            self.node = node
            self.resp = None
            self.give_send = None
            self.received = 0
            self.failed = False
            self.reader = "this is a string of length 29"
            self.with_exc = with_exc
        def getresponse(self):
            if self.with_exc:
                raise Exception('test')
            return self
        def getexpect(self):
            return self
        def read(self, amt=None):
            return ''

        def send(self, amt=None):
            self.received += len(amt)

    return FakeConn(response, with_exc)

def mock_heart_beat(self):
    class Thrd:
        def __init__(self,target=None, *args, **kwargs):
            pass
        def start(self):
            return "Hey ;)"
    return Thrd


class Service_object:
    def __init__(self, ip, port, id):
        self.ip = ip
        self.port = port
        self.id = id
    def get_ip(self):
        return self.ip
    def get_port(self):
        return self.port
    def get_id(self):
        return self.id 

class GLmap:
    def __init__(self):
        self.service_obj = Service_object("192.168.101.1","61005","HN0101_0000_container-server")
        self.service_obj1 = Service_object("192.168.101.4","61005","HN0103_0000_container-server")
        self.service_obj2 = Service_object("192.168.101.6","61004","HN0102_0000_container-server")
        self.map = {self.service_obj:[1,2], self.service_obj1:[3,4], self.service_obj2:[5,6]}

    def dest_comp_map(self):
        return self.map

    def source_service_obj(self):
        return Service_object("192.168.101.1","61004","HN0101_0000_container-server")


def mock_get_object_gl_version(self):
    return 9

def mock_send_file(self):
    pass

class Monitor:
    def __init__(self,*args,**kwargs):
        pass
    def __call__(self, *args, **kwargs):
        pass
    def start(self):
        pass
    def join(self):
        pass

class ContainerRecovery(unittest.TestCase):
    def setUp(self):
        self.gl_map_object = GLmap()
        self.conf = {'CLIENT_TIMEOUT': 10}
        self.communicator = "None_for_now"
        self.service_id = "HN0101_61014_container-server"
        self.logger = get_logger({}, log_route='recovery')
        self.__trans_path = "/export/HN0101_61014_transaction_journal"
        self.__cont_path = "/export/HN0101_61014_container_journal"
        osd.containerService.container_recovery.final_recovery_status_list = list()
        osd.containerService.container_recovery.list_of_tuple = queue(maxsize=0)
        osd.containerService.container_recovery.Thread = Monitor()



    def tearDown(self):
        pass


    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version",mock_get_object_gl_version)
    def test_restructure_entries(self):
        comm_obj = "Sample"
        self.recovery_object = RecoveryMgr(conf, self.gl_map_object, \
                self.service_id, comm_obj, self.communicator,logger=None)

        container_dict = {1:[1,2,3],2:[4,5,6],3:[7,8],4:[9],5:[1,2],6:[4,5]}
        dictionary_new = {'192.168.101.4:61005':[6],'192.168.101.1:61005':[5], '192.168.101.6:61005': [1,2,3,4]}
        jk = {'192.168.101.4:61005': [4,5],'192.168.101.1:61005':[1,2],'192.168.101.6:61005': [1,2,3,4,5,6,7,8,9]}
        container_entries = \
                self.recovery_object.restructure_entries(dictionary_new, container_dict)
        self.assertEqual(container_entries, jk)

#    @mock.patch("osd.containerService.container_recovery.heart_beat_thread")
    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version", mock_get_object_gl_version)
    def test_get_map_restructure(self):
        comm_obj = "Sample"
        osd.containerService.container_recovery.EXPORT = "/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage"

        self.recovery_object = RecoveryMgr(conf, self.gl_map_object, \
                self.service_id, comm_obj, self.communicator, logger=None)
        service_location_map, service_component_map = self.recovery_object._get_map_restructure()
        self.assertEqual(service_location_map,{'192.168.101.4':'61005','192.168.101.6': '61004', '192.168.101.1': '61005'})
        self.assertEqual(service_component_map,{'192.168.101.4:61005':[3,4],'192.168.101.1:61005': [1, 2], '192.168.101.6:61004': [5, 6]})

    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version",mock_get_object_gl_version)
    def test_read_container_journal(self):
        comm_obj = "Sample"
        if os.path.exists("/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/HN0101_61014_container_journal"):
            os.rmdir("/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/HN0101_61014_container_journal")
            os.mkdir("/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/HN0101_61014_container_journal")
        osd.containerService.container_recovery.EXPORT="/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/"

        self.recovery_object = RecoveryMgr(conf, self.gl_map_object, \
                self.service_id, comm_obj, self.communicator)
        container_entries = self.recovery_object.read_container_journal({'192.168.101.4:61005':[3,4],'192.168.101.1:61005':[1,2], '192.168.101.6:61004': [5, 6]})
        self.assertEqual(container_entries, {})

    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version",mock_get_object_gl_version)
    def test_read_transaction_journal(self):
        comm_obj = "Sample"
        if os.path.exists("/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/HN0101_61014_transaction_journal"):
            os.rmdir("/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/HN0101_61014_transaction_journal")
            os.mkdir("/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/HN0101_61014_transaction_journal")
        osd.containerService.container_recovery.EXPORT="/remote/hydra042/jkothari/CONTAINER_UT_REC/objectStorage/"

        print "--------------------------------------------2"
        self.recovery_object = RecoveryMgr(conf, self.gl_map_object, \
                self.service_id, comm_obj, self.communicator)
        transaction_entries = self.recovery_object.read_transaction_journal({'192.168.101.4:61005':[3,4],'192.168.101.1:61005':[1,2], '192.168.101.6:61004': [5, 6]})
        print "--------------------------------------------1"
        self.assertEqual(transaction_entries, {})
    

    #def test_read_transaction_journal(self):
    #    comm_obj = "Sample"
    #    self.recovery_object = RecoveryMgr(conf, self.gl_map_object, \
    #            self.service_id, comm_obj, self.communicator)
    #    container_entries = self.recovery_object.read_transaction_journal({'192.168.101.4:61005':[3,4],'192.168.101.1:61005':[1,2], '192.168.101.6:61004': [5, 6]})
    #    self.assertEqual(container_entries, {})

    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version",mock_get_object_gl_version)
    @mock.patch("osd.containerService.container_recovery.http_connect",mock_http_connect(100))
    def test_recovery_connection(self):

        print "--------------------------------------------3"
        dictionary_new = {'1.2.3.4:1234': [1]}
        communicator_obj = "HN0101_container_service"
        service_component_map = {'1.2.3.4:1234': [1]}

        recovery_object = Recovery(conf, dictionary_new, communicator_obj, \
            service_component_map, self.service_id)

        print "--------------------------------------------4"
        nodes = {'ip':'1.2.3.4','port':'1234'}
        resp = recovery_object._connect_put_node(nodes, {'x-timestamp':'1408505144.00290'}, self.logger.thread_locals)
        self.assertEqual(resp.status, 100)

    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version",mock_get_object_gl_version)
    @mock.patch("osd.containerService.container_recovery.http_connect", mock_http_connect(200))
    def test_recovery_connection_case2(self):
        dictionary_new = {'1.2.3.4:1234': [1]}
        communicator_obj = "HN0101_container_service"
        service_component_map = {'1.2.3.4:1234': [1]}

        recovery_object = Recovery(conf, dictionary_new, communicator_obj, \
            service_component_map, self.service_id )
        nodes = {'ip':'1.2.3.4','port':'1234'}
        resp = recovery_object._connect_put_node(nodes, {'x-timestamp':'1408505144.00290'}, self.logger.thread_locals)
        self.assertEqual(resp.status, 200)
        self.assertEqual(set(resp.node) == set(nodes), True)


    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version",mock_get_object_gl_version)
    def test_recovery_connection_case3(self):
        osd.containerService.container_recovery.final_recovery_status_list = list()
        osd.containerService.container_recovery.list_of_tuple = queue(maxsize=0)
        communicator_obj = "HN0101_container_service"
        dictionary_new = {'1.2.3.4:1234': [1], '5.6.7.8:5678':[2],'9.2.3.1:9231':[3]}
        service_component_map = {'1.2.3.4:1234': [1], '5.6.7.8:5678':[2],'9.2.3.1:9231':[3]}

        recovery_object = Recovery(conf, dictionary_new, communicator_obj, \
            service_component_map, self.service_id )
        nodes = [{'ip':'1.2.3.4','port':'1234'}, {'ip':'5.6.7.8','port':'5678'}, {'ip':'9.2.3.1','port':'9231'}]
        s =[mock_http_connection(200,nodes[0]), mock_http_connection(200,nodes[1]), mock_http_connection(500,nodes[2])]
        a, b, c, g  = recovery_object._get_put_responses(s, nodes)
        self.assertEqual(a,[(200, '1.2.3.4:1234'), (200, '5.6.7.8:5678'), (500, '9.2.3.1:9231')])
        self.assertEqual(b,['Fake', 'Fake', 'Fake'])
        self.assertEqual(c, ['', '', ''])
        self.assertEqual(osd.containerService.container_recovery.final_recovery_status_list,[(1, True), (2, True), (3, False)])
        self.assertEqual(g, ['1.2.3.4:1234', '5.6.7.8:5678'])



    @mock.patch("osd.containerService.container_recovery.RecoveryMgr._get_object_gl_version",mock_get_object_gl_version)
    @mock.patch("osd.containerService.container_recovery.http_connect",mock_http_connect(100))
    def test_recovery_connection4(self):
        dictionary_new = {'1.2.3.4:1234': [1]}
        communicator_obj = "HN0101_container_service"
        service_component_map = {'1.2.3.4:1234': [1]}

        recovery_object = Recovery(conf, dictionary_new, communicator_obj, \
            service_component_map, self.service_id)

        nodes = {'ip':'1.2.3.4','port':'1234'}
        conn = mock_http_connection(200,nodes)
        resp = recovery_object._send_file(conn,"nothing")

        self.assertEqual(conn.received, 29)


    def test_recovery_connection5(self):
        dictionary_new = {'1.2.3.4:1234': [1]}
        communicator_obj = "HN0101_container_service"
        service_component_map = {'1.2.3.4:1234': [1]}

        nodes = {'ip':'1.2.3.4','port':'1234'}
        conn = mock_http_connection(200,nodes)

        recovery_object = Recovery(conf, dictionary_new, communicator_obj, \
            service_component_map, self.service_id)
        a, b, c, d = recovery_object.recovery_container("ACCEPT_THIS")

        self.assertEqual(a, [503])
        self.assertEqual(b, [''])
        self.assertEqual(c, [''])
        self.assertEqual(d, [])


    def __del__(self):
        check_call("rm -rf updater Journal test_acc %s" %tmp_path ,shell=True)
        check_call("rm -rf JournalDir" , shell=True)
        subprocess.call(['chmod', '777', path])
        pass





if __name__ == '__main__':
    unittest.main()
