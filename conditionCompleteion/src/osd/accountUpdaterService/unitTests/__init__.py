import mock
import unittest
import threading
import Queue
from osd.accountUpdaterService.utils import SimpleLogger
OSD_DIR='/texport/osd_meta_config'
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

conf = {
        'user': 'root', 
        'filesystems': '/texport',
        'stat_file_location': '/remote/hydra042/anishaj/osd/objectStorage_accUpdater_logger/upfiles2',
        'file_expire_time': 900,
        'interval': 600,
        'conn_timeout': 10,
        'node_timeout': 10,
        'osd_dir': '/texport/osd_meta_config',
        'reader_max_count': 10,
        'max_update_files': 10,
        'log_file': 'accountUpdater.log',
        'log_facility': 'LOG_LOCAL6',
        'log_level': 'debug',
        'log_path': '/remote/hydra042/anishaj/osd/objectStorage_accUpdater_logger',
        'log_backup_count': 3,
        'll_port': 61014,
        'account_updater_port': 3128,
        'max_thread': 10,
        'client_timeout':10,
        'max_files_stored_in_cache':100
}
logger = SimpleLogger(conf).get_logger_object()
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
        #return ['fs1', 'fs2', 'fs3', 'fs4', 'fs5', 'fs6', 'fs7', 'fs8', 'fs9', 'fs10']
        return ['OSP_01','OSP_02','OSP_03','OSP_04','OSP_05','OSP_06', 'OSP_07','OSP_08','OSP_09','OSP_10']

    def get_account_level_dir_list(self):
        return ['a01', 'a02', 'a03', 'a04', 'a05', 'a06', 'a07', 'a08', 'a09', 'a10']

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
    FAILURE = 1

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

def fake_get_directory_by_hash(*args, **kwargs):
    return ""

class FakeTransferCompMessage:
    def de_serialize(self, comp_list):
        self.comp_list = comp_list
        self.comp_map = {}
 
    def service_comp_map(self):
        self.comp_map[mockServiceInfo('HN0102_61014_account-updater', '169.254.1.12', '61009')] = range(10)
        #self.comp_map[mockServiceInfo('HN0103_61014_account-updater', '169.254.1.13', '61009')] = range(10, 20)
        return self.comp_map

class FakeTransferCompResponseMessage:
    def __init__(self, status_list, status):
        self.status_list = status_list
        self.status = status

    def serialize(self):
        return (self.status_list, self.status)

class FakeGlobalVariables:
    def __init__(self, logger, ownership_list=range(10, 30)):
        self.logger = logger
        self.__transfer_cmp_event  = threading.Event()
        self.__complete_all_event = threading.Event()
        self.acc_updater_version = 0.0
        self.__ownership_list = ownership_list
        self.__get_comp_counter_queue = Queue.Queue()
        self.__complete_all_event = threading.Event()
        self.__transfer_cmp_event  = threading.Event()
        self.__container_sweeper_list = []
        self.counter = 1

    def create_sweeper_list(self, comp_tuple):
        self.__container_sweeper_list.append(comp_tuple)

    def get_sweeper_list(self):
        return self.__container_sweeper_list

    def pop_from_sweeper_list(self, temp_list):
        self.__container_sweeper_list = temp_list
 
    def get_transfer_cmp_event(self):
        return self.__transfer_cmp_event

    def get_complete_all_event(self):
        return self.__complete_all_event

    def load_gl_map(self, *args, **kwargs):
        return "map"
    
    def get_acc_updater_map_version(self):
        self.acc_updater_version += 1
        return self.acc_updater_version

    def get_ownershipList(self):
        if self.counter > 2:
            self.__ownership_list = range(2,10)
        self.counter += 1
        return self.__ownership_list

    def set_ownershipList(self, ownership_list):
        self.__ownership_list = ownership_list

    def put_into_Queue(self):
        self.__get_comp_counter_queue.put(True)

    def get_from_Queue(self):
        return 1

    def get_service_id(self):
        return ""

    def set_service_id(self, service_id):
        self.service_id = service_id

    def load_ownership(self):
       self.__ownership_list = range(20)
       return self.__ownership_list

    def get_my_ownership(self):
       return ""

#Fake http_connect method of bufferedhttp
def fake_http_connect(ip, port, fs, dir, method, path, headers):
    conn = FakeHttpConnect(status = 204)
    return conn

class FakeHttpConnect:
    def __init__(self, status):
        self.reader = None
        self.status = status

    def getexpect(self):
        return FakeConnection(self.status)

    def send(self, data):
        if self.status == 500:
            raise Exception
     
class FakeConnectionCreator:
    def __init__(self, conf, logger):
        pass

    def connect_container(self, method, account_name, container_name, \
                                                    container_path):
        conn = FakeConnection()
        return conn

    def get_http_connection_instance(self, info, data):
        conn = FakeConnection()
        return conn

    def get_http_response_instance(self, conn):
        resp = conn.getresponse()
        return resp

class FakeHTTPConnection:
    def __init__(self, ip, port):
        pass

    def request(self, method, path, data, headers):
        pass
 
class FakeConnection:
    def __init__(self, status=204):
        self.status = status

    def getresponse(self):
        fake_resp_obj = FakeResponse(self.status)
        return fake_resp_obj

    def close(self):
        pass

class FakeResponse:
    def __init__(self, status):
        self.status = status

    def getheaders(self):
        headers = {}
        headers['x-container'] = 'container'
        headers['x-put-timestamp'] = 'dummy'
        headers['x-container-object-count'] = 'dummy'
        headers['x-container-bytes-used'] = 'dummy'
        return headers

    def read(self):
        return ""

