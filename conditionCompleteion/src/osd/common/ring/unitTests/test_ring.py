import os
import mock
import unittest
import cPickle as pickle
import tempfile
from shutil import rmtree
from osd.common.ring import ring
from osd.common.ring.ring import hash_path
from osd.objectService.unitTests import FakeLogger, debug_logger
OSD_DIR="/texport/osd_meta_config"
class mockServiceInfo:
    def __init__(self, id, ip, port):
        self.__ID = id
        self.__IP = ip
        self.__Port = port

    def get_id(self):
        return self.__ID
    def get_ip(self):
        return self.__IP
    def get_port(self):
        return self.__Port

acc_map = list()
cont_map = list()
obj_map = list()
au_map = list()
acc_id = list()
acc_ip = list()
acc_port = list()
cont_id = list()
cont_ip = list()
cont_port = list()
obj_id = list()
obj_ip = list()
obj_port = list()
au_id = list()
au_ip = list()
au_port = list()

for i in range(512):
    acc_id.append("HN010%s_61006_account" %i)
    cont_id.append("HN010%s_61007_container" %i)
    obj_id.append("HN010%s_61008_object" %i)
    au_id.append("HN010%s_61009_acc-up" %i)
    acc_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    cont_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    obj_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    au_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    acc_port.append("%s%s" %(i, i))
    cont_port.append("%s%s" %(i, i))
    obj_port.append("%s%s" %(i, i))
    au_port.append("%s%s" %(i, i))

    acc_map.append(mockServiceInfo("HN010%s_61006_account"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))
    cont_map.append(mockServiceInfo("HN010%s_61007_container"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))
    obj_map.append(mockServiceInfo("HN010%s_61008_object"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))
    au_map.append(mockServiceInfo("HN010%s_61009_acc-up"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))


class FakeRingFileReadHandler:
    def __init__(self, path=OSD_DIR):
	pass

    def get_fs_list(self):
        return ['fs1', 'fs2', 'fs3', 'fs4', 'fs5', 'fs6', 'fs7', 'fs8', 'fs9', 'fs10']

    def get_account_level_dir_list(self):
        return ['a1', 'a2', 'a3', 'a4', 'a5', 'a6', 'a7', 'a8', 'a9', 'a10']

    def get_container_level_dir_list(self):
        return ['d1', 'd2', 'd3', 'd4', 'd5', 'd6', 'd7', 'd8', 'd9', 'd10']

    def get_object_level_dir_list(self):
        return ['c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7', 'c8', 'c9', 'c10', 'c11', 'c12', 'c13', 'c14', 'c15', 'c16', 'c17', 'c18', 'c19', 'c20']

class mockMapData:
    def __init__(self):
        pass

    def map(self):
        return {
            "account-server": (acc_map, "3.0"),
            "container-server": (cont_map, "4.0"),
            "object-server": (obj_map, "3.0"),
            "accountUpdater-server": (au_map, "5.0")
            }

    def version(self):
        return "5.0"

class mockReq:
    def __init__(self, logger=None):
        pass
	
    def get_gl_info(self):
        gl_info_obj = ("SOCKIO", "127.0.1.1")
        return gl_info_obj
        
    def connector(self, conn_type, gl_info_obj):
        return mockClose()

    def get_global_map(self, service_id, communicator_obj):
        status = mockStatus(mockResp.SUCCESS, "")
        return mockResult(status, mockMapData())

    def get_comp_list(self, service_id, communicator_obj):
        status = mockStatus(mockResp.SUCCESS, "")
        return mockResult(status, range(1,513))        

    def comp_transfer_info(self, service_id, comp_list, communicator_obj):
        status = mockStatus(mockResp.SUCCESS, "Success")
        return mockResult(status, 'Success')

class mockClose:
    def close(self):
        pass

class mockResp:
    SUCCESS = 0

class mockStatus:
    def __init__(self, code, msg=None):
        self.__code = code
        self.__msg = msg

    def __repr__(self):
        return "(status code: %s, status message: %s)" \
        % (self.__code, self.__msg)

    def __str__(self):
        return "(status code: %s, status message: %s)" \
        % (self.__code, self.__msg)


    def get_status_code(self):
        """@desc status code getter method"""
        return self.__code

    def get_status_message(self):
        """desc status message getter method"""
        return self.__msg


class mockResult:
    def __init__(self, status, message=None):
        self.status = status
        self.message = message

class TestRingData(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.ring_data_obj = ring.RingData(OSD_DIR, debug_logger())

    def tearDown(self):
        pass

    def test_get_account_map(self):
        self.assertEquals([obj.get_id() for obj in self.ring_data_obj.get_account_map()], acc_id)
        self.assertEquals([obj.get_ip() for obj in self.ring_data_obj.get_account_map()], acc_ip)
        self.assertEquals([obj.get_port() for obj in self.ring_data_obj.get_account_map()], acc_port)

    def test_get_account_map_version(self):
        self.assertEquals(self.ring_data_obj.get_account_map_version(), "3.0")

    def test_get_container_map(self):
        self.assertEquals([obj.get_id() for obj in self.ring_data_obj.get_container_map()], cont_id)
        self.assertEquals([obj.get_ip() for obj in self.ring_data_obj.get_container_map()], cont_ip)
        self.assertEquals([obj.get_port() for obj in self.ring_data_obj.get_container_map()], cont_port)

    def test_get_container_map_version(self):
        self.assertEquals(self.ring_data_obj.get_container_map_version(), "4.0")

    def test_get_object_map(self):
        self.assertEquals([obj.get_id() for obj in self.ring_data_obj.get_object_map()], obj_id)
        self.assertEquals([obj.get_ip() for obj in self.ring_data_obj.get_object_map()], obj_ip)
        self.assertEquals([obj.get_port() for obj in self.ring_data_obj.get_object_map()], obj_port)


    def test_get_object_map_version(self):
        self.assertEquals(self.ring_data_obj.get_object_map_version(), "3.0")

    def test_get_account_updater_map(self):
        self.assertEquals([obj.get_id() for obj in self.ring_data_obj.get_account_updater_map()], au_id)
        self.assertEquals([obj.get_ip() for obj in self.ring_data_obj.get_account_updater_map()], au_ip)
        self.assertEquals([obj.get_port() for obj in self.ring_data_obj.get_account_updater_map()], au_port)


    def test_get_account_updater_map_version(self):
        self.assertEquals(self.ring_data_obj.get_account_updater_map_version(), "5.0")

    def test_get_global_map(self):
        self.assertEquals(self.ring_data_obj.get_global_map(), {
            "account-server": (acc_map, "3.0"),
            "container-server": (cont_map, "4.0"),
            "object-server": (obj_map, "3.0"),
            "accountUpdater-server": (au_map, "5.0")
            })

    def test_get_global_map_version(self):
        self.assertEquals(self.ring_data_obj.get_global_map_version(), "5.0")

    def test_get_fs_list(self):
        self.assertEquals(self.ring_data_obj.get_fs_list(), ['fs1', 'fs2', 'fs3', 'fs4', 'fs5', 'fs6', 'fs7', 'fs8', 'fs9', 'fs10'])
    
    def test_get_account_level_dir_list(self):
        self.assertEquals(self.ring_data_obj.get_account_level_dir_list(), ['a1', 'a2', 'a3', 'a4', 'a5', 'a6', 'a7', 'a8', 'a9', 'a10'])

    def test_load(self):
        self.assertRaises(NotImplementedError, self.ring_data_obj.load, (''))
    
    def test_get_container_level_dir_list(self):
        self.assertEquals(self.ring_data_obj.get_container_level_dir_list(), ['d1', 'd2', 'd3', 'd4', 'd5', 'd6', 'd7', 'd8', 'd9', 'd10'])

    def test_get_object_level_dir_list(self):
        self.assertEquals(self.ring_data_obj.get_object_level_dir_list(), ['c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7', 'c8', 'c9', 'c10', 'c11', 'c12', 'c13', 'c14', 'c15', 'c16', 'c17', 'c18', 'c19', 'c20'])

    def test_get_acc_dir_by_hash(self):
        acc_hash = hash_path('a')
        self.assertEquals(self.ring_data_obj.get_acc_dir_by_hash(acc_hash), 'a10')

    def test_get_cont_dir_by_hash(self):
        cont_hash = hash_path('a', 'c')
        self.assertEquals(self.ring_data_obj.get_cont_dir_by_hash(cont_hash), 'd6')

class TestObjectRingData(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.object_ring_data_obj = ring.ObjectRingData(OSD_DIR, debug_logger())

    def tearDown(self):
        pass

    def test_get_object_service_list(self):
        pass

    def test_load(self):
        pass

class TestContainerRingData(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.container_ring_data_obj = ring.ContainerRingData(OSD_DIR, debug_logger())

    def tearDown(self):
        pass

    def test_get_container_service_list(self):
        pass

    def test_get_components_list(self):
        pass

    def test_get_comp_container_dist(self):
        pass

    def test_load(self):
        pass

class TestAccountRingData(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)    
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.account_ring_data_obj = ring.AccountRingData(OSD_DIR, debug_logger())

    def tearDown(self):
        pass

    def test_get_account_service_list(self):
        pass

    def test_get_components_list(self):
        pass

    def test_get_comp_account_dist(self):
        pass

    def test_load(self):
        pass

class TestObjectRing(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.object_ring_obj = ring.ObjectRing(OSD_DIR, debug_logger())

    def tearDown(self):
        pass

    def create_ring_file(self):
        pass

    def test_get_fs_list(self):
        self.assertEquals(self.object_ring_obj.get_fs_list(), ['fs1', 'fs2', 'fs3', 'fs4', 'fs5', 'fs6', 'fs7', 'fs8', 'fs9', 'fs10'])

    def test_get_service_details(self):
        self.assertEquals(self.object_ring_obj.get_service_details(mockServiceInfo("HN0101_61008_obj", "1.2.3.4", "10")), \
            [{'ip': '1.2.3.4', 'port': '10'}])
        self.assertEquals(self.object_ring_obj.get_service_details(mockServiceInfo("HN0102_61008_obj", "127.0.0.1", "20")), \
            [{'ip': '127.0.0.1', 'port': '20'}])

    def test_get_object_level_directory(self):
        self.assertEquals(self.object_ring_obj.get_object_level_directory('a', 'c', 'o'), \
            (10, 'c10'))

    def test_get_obj_dir_by_hash(self):
        obj_hash = hash_path('a', 'c', 'o')
        self.assertEquals(self.object_ring_obj.get_obj_dir_by_hash(obj_hash), (10, 'c10'))

    def test_get_account_level_directory(self):
        self.assertEquals(self.object_ring_obj.get_account_level_directory('a'), \
            'a10')

    def test_get_container_level_directory(self):
        self.assertEquals(self.object_ring_obj.get_container_level_directory('a', 'c'), \
            'd6')

    def test_get_filesystem(self):
        self.assertEquals(self.object_ring_obj.get_filesystem(2), 'fs1')
        self.assertRaises(IndexError, self.object_ring_obj.get_filesystem, (50))
 
    def test_get_node(self):
        with mock.patch('osd.common.ring.ring.get_service_obj', return_value=mockServiceInfo("HN0102_61008_obj", "127.0.0.1", "20")):
            self.assertEquals(self.object_ring_obj.get_node('a', 'c', 'o'), \
                ([{'ip': '127.0.0.1', 'port': '20'}], 'fs5', 'a10/d6/c10', '5.0', None))
            with mock.patch('osd.common.ring.ring.ObjectRing.get_account_level_directory', \
                side_effect=IndexError):
                self.assertEquals(self.object_ring_obj.get_node('a', 'c', 'o'), \
                    ('', '', '', '', ''))


    def test_get_directories_by_hash(self):
        acc_hash = hash_path('a',) 
        cont_hash = hash_path('a', 'c')
        obj_hash = hash_path('a', 'c', 'o')
        self.assertEquals(self.object_ring_obj.get_directories_by_hash \
            (acc_hash, cont_hash, obj_hash), ('fs5', 'a10/d6/c10'))

    def test_get_service_ids(self):
        self.assertEquals(self.object_ring_obj.get_service_ids(), obj_id)

class TestContainerRing(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.container_ring_obj = ring.ContainerRing(OSD_DIR, debug_logger())

    def tearDown(self):
        pass

    def create_ring_file(self):
        pass

    def test_get_component(self):
        self.assertEquals(self.container_ring_obj.get_component('a', 'c'), 435)

    def test_get_container_service(self):
        self.assertEquals([self.container_ring_obj.get_container_service(0).get_id(), self.container_ring_obj.get_container_service(0).get_ip(), self.container_ring_obj.get_container_service(0).get_port()],["HN0100_61007_container", "0.0.0.0", "00"])

    def test_get_service_details(self):
        self.assertEquals(self.container_ring_obj. \
            get_service_details(mockServiceInfo("HN0101_61007_cont", "1.2.3.4", "10")), \
            [{'ip': '1.2.3.4', 'port': '10'}])

    def test_get_filesystem(self):
        self.assertEquals(self.container_ring_obj.get_filesystem(), 'fs1')

    def test_get_account_level_directory(self):
        self.assertEquals(self.container_ring_obj.get_account_level_directory('a'), 'a10')

    def test_get_container_level_directory(self):
        self.assertEquals(self.container_ring_obj.get_container_level_directory('a', 'c'), 'd6')

    def test_get_directory_by_hash(self):
        acc_hash = hash_path('a')
        cont_hash = hash_path('a', 'c')
        self.assertEquals(self.container_ring_obj.get_directory_by_hash(acc_hash, cont_hash), 'a10/d6')

    def test_get_node(self):
        with mock.patch('osd.common.ring.ring.ContainerRing.get_component', return_value=0):
            self.assertEquals(self.container_ring_obj.get_node('a', 'c'), \
                ([{'ip': '0.0.0.0', 'port': '00'}], 'fs1', 'a10/d6', '5.0', 0))
        self.assertEquals(self.container_ring_obj.get_node('a', 'c'), \
            ([{'ip': '435.435.435.435', 'port': '435435'}], 'fs1', 'a10/d6', '5.0', 435))
        with mock.patch('osd.common.ring.ring.ContainerRing.get_account_level_directory', \
            side_effect=Exception):
            self.assertEquals(self.container_ring_obj.get_node('a', 'c'), \
                ('', '', '', '', ''))

    def test_get_service_ids(self):
        self.assertEquals(self.container_ring_obj.get_service_ids(), cont_id)

class TestAccountRing(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.account_ring_obj = ring.AccountRing(OSD_DIR, debug_logger())

    def tearDown(self):
        pass

    def create_ring_file(self):
        pass
    
    def test_get_component(self):
        self.assertEquals(self.account_ring_obj.get_component('a'), 431)

    def test_get_account_service(self):
        self.assertEquals([self.account_ring_obj.get_account_service(0).get_id(), self.account_ring_obj.get_account_service(0).get_ip(), self.account_ring_obj.get_account_service(0).get_port()], ["HN0100_61006_account", "0.0.0.0", "00"])

    def test_get_service_details(self):
        self.assertEquals(self.account_ring_obj.get_service_details(mockServiceInfo("HN0101_61006_acc", "1.2.3.4", "10")), \
            [{'ip': '1.2.3.4', 'port': '10'}])

    def test_get_directory(self):
        self.assertEquals(self.account_ring_obj.get_directory('a'), 'a10')

    def test_get_filesystem(self):
        self.assertEquals(self.account_ring_obj.get_filesystem(), 'fs1')

    def test_get_node(self):
        with mock.patch('osd.common.ring.ring.AccountRing.get_component', return_value=0):
            self.assertEquals(self.account_ring_obj.get_node('a'), \
                ([{'ip': '0.0.0.0', 'port': '00'}], 'fs1', 'a10', '5.0', 0))
        self.assertEquals(self.account_ring_obj.get_node('a'), \
            ([{'ip': '431.431.431.431', 'port': '431431'}], 'fs1', 'a10', '5.0', 431))
        with mock.patch('osd.common.ring.ring.AccountRing.get_directory', \
            side_effect=Exception):
            self.assertEquals(self.account_ring_obj.get_node('a'),('', '', '', '', ''))

    def test_get_service_ids(self):
        self.assertEquals(self.account_ring_obj.get_service_ids(), acc_id)

class TestGlobalMethods(unittest.TestCase):
    @mock.patch("osd.common.ring.ring.Req", mockReq)
    @mock.patch("osd.common.ring.ring.Resp", mockResp)
    @mock.patch("osd.common.ring.ring.RingFileReadHandler", FakeRingFileReadHandler)
    @mock.patch("osd.common.ring.ring.ServiceInfo", mockServiceInfo)
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_get_service_id(self):
        self.assertEquals(ring.get_service_id("object"), "hydra042_61014_object-server")
        self.assertRaises(ValueError, ring.get_service_id, ("dummy"))

    def test_get_fs_list(self):
        self.assertEquals(ring.get_fs_list(debug_logger()), ['fs1', 'fs2', 'fs3', 'fs4', 'fs5', 'fs6', 'fs7', 'fs8', 'fs9', 'fs10'])

    def test_all_service_id(self):
        self.assertEquals(ring.get_all_service_id("object", debug_logger()), obj_id)
        self.assertEquals(ring.get_all_service_id("container", debug_logger()), cont_id)
        self.assertEquals(ring.get_all_service_id("account", debug_logger()), acc_id)
        self.assertRaises(ValueError, ring.get_all_service_id, ("dummy"), (debug_logger()))

    def test_get_ip_port_by_id(self):
        self.assertEquals(ring.get_ip_port_by_id("HN0101_61006_account", debug_logger()), {'ip': '1.1.1.1', 'port': '11'})
        self.assertEquals(ring.get_ip_port_by_id("HN0101_61007_container", debug_logger()), {'ip': '1.1.1.1', 'port': '11'})
        self.assertEquals(ring.get_ip_port_by_id("HN0101_61008_object", debug_logger()), {'ip': '1.1.1.1', 'port': '11'})

if __name__ == '__main__':
    unittest.main()
