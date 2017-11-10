from math import ceil
import os
import socket
import time
from hashlib import md5
from osd.common.ring.calculation import Calculation
from osd.common.macros import Macros
from osd.common.helper_functions import float_comp
from eventlet import sleep
from libOsmService import RingFileReadHandler
from osd.glCommunicator.req import Req
from osd.glCommunicator.response import Resp
from osd.glCommunicator.communication.communicator import IoType
from osd.glCommunicator.service_info  import ServiceInfo
from osd.common.exceptions import UpdateGlobalMapTimeout

OSD_DIR = '/export/.osd_meta_config'
fs_list_count = 10
account_dir_count = 10
container_dir_count = 10
object_dir_count = 20

def hash_path(account, container=None, object=None, raw_digest=False):
    """
    Get the canonical hash for an account/container/object

    :param account: Account
    :param container: Container
    :param object: Object
    :param raw_digest: If True, return the raw version rather than a hex digest
    :returns: hash string
    """
    if object and not container:
        raise ValueError('container is required if object is provided')
    paths = [account]
    if container:
        paths.append(container)
    if object:
        paths.append(object)
    if raw_digest:
        return md5(Macros.getValue("HASH_PATH_PREFIX") + '/' + '/'.join(paths)
                   + Macros.getValue("HASH_PATH_SUFFIX")).digest()
    else:
        return md5(Macros.getValue("HASH_PATH_PREFIX") + '/' + '/'.join(paths)
                   + Macros.getValue("HASH_PATH_SUFFIX")).hexdigest()

def get_service_id(server):
    """
    Get service id of service
    Ex . HN0101_container-server 

    :param type: Type of service
    """
    SERVERS = ['object', 'container', 'account']
    if server  in SERVERS:
        host_name = socket.gethostname()
        return host_name + "_61014_" + server +"-" + "server"
    raise ValueError("Type of server is not correct: %s" %server)

def get_own_service_id():
    """
    Get service id of proxy
    Ex . HN0101_61005(port)_proxy-server

    :param type: Type of service
    """
    host_name = socket.gethostname()
    return host_name + "_61014_" + "proxy-server"	#TODO need to remove hard code

def get_service_obj(server, logger):
    """
    Get service obj of service

    :param type: Type of service
    """
    host_name = socket.gethostname()
    ring_data = RingData(OSD_DIR, logger) 
    if server == 'object':
        if not ring_data.get_object_map():
            logger.debug("Object map is empty: %s" %ring_data.get_object_map())
            ring_data.update_object_map()
        map_updated = check_object_map_updated(ring_data, host_name, logger)
        while not map_updated:
            map_updated = check_object_map_updated(ring_data, host_name, \
                logger)
            sleep(20)
        for obj in ring_data.get_object_map():
            if host_name in obj.get_id():
                service_obj = obj
        return service_obj
    raise ValueError("Type of server is not correct: %s" %server)

def check_object_map_updated(ring_data, host_name, logger):
    for obj in ring_data.get_object_map():
        if host_name in obj.get_id():
            logger.debug("Current node :%s exists in object map: %s" \
                %(host_name, ring_data.get_object_map()))
            return True
    else:
        logger.warning("Current node: %s does not exist in object_map: %s" \
            %(host_name, ring_data.get_object_map()))
        ring_data.update_object_map()
        return False

def get_fs_list(logger):
    """
    Returns available filesystem list
    """
    ring = ObjectRing(OSD_DIR, logger)
    return ring.get_fs_list()

def get_all_service_id(server_name, logger):
    """
    Returns all service id for any server
    :param server_name: name of the server
    :returns: list of service ids
    """
    if server_name == 'object':
        ring = ObjectRing(OSD_DIR, logger)
    elif server_name == 'container':
        ring = ContainerRing(OSD_DIR, logger)
    elif server_name == 'account':
        ring = AccountRing(OSD_DIR, logger)
    else:
        raise ValueError('Invalid server name: %s' % server_name)
    return ring.get_service_ids()

def get_object(service_id, logger, ring_data):
    service_obj = None
    if 'object' in service_id:
        ring = ObjectRing(OSD_DIR, logger)
        for obj in ring_data.get_object_map():
            if service_id == obj.get_id():
                service_obj = obj
                break
    elif 'container' in service_id:
        ring = ContainerRing(OSD_DIR, logger)
        for obj in ring_data.get_container_map():
            if service_id == obj.get_id():
                service_obj = obj
                break
    else:
        ring = AccountRing(OSD_DIR, logger)
        for obj in ring_data.get_account_map():
            if service_id in obj.get_id():
                service_obj = obj
                break
    return service_obj, ring

def get_ip_port_by_id(service_id, logger):
    """
    Gets service IP and Port for the passed service_id
    :param service_id: service id
    :returns: dictionary containing IP and port of the service
    """
    data = {}
    retry = 3
    counter = 0
    ring_data = RingData(OSD_DIR, logger)
    service_obj, ring = get_object(service_id, logger, ring_data)
    try:
        if service_obj:
            data = ring.get_service_details(service_obj)[0]
            return data
    except IndexError:
        return {'ip': '', 'port': ''}

    while (not service_obj and counter < retry):
        logger.warning("Map is not updated as does not contain service id: %s" \
            " retry:%s" %(service_id, counter))
        ring_data.update_global_map()
        service_obj, ring = get_object(service_id, logger, ring_data)
        try:
            if service_obj:
                data = ring.get_service_details(service_obj)[0]
                return data
            else:
                data = {'ip': '', 'port': ''}
        except IndexError:
            return {'ip': '', 'port': ''}
        if not data['ip'] or not data['port']:
            sleep(120)
        counter += 1
    return data

class SingletonType(type):
    """ Class to create singleton classes object"""
    __instance = None

    def __call__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(SingletonType, cls).__call__(*args, **kwargs)
        return cls.__instance


class RingData(object):
    __metaclass__ = SingletonType

    def __init__(self, file_location, logger=None):
        self.logger = logger
        self.__ring_obj = RingFileReadHandler(file_location)
        self._fs_list = []
        self._account_level_dir_list = []
        self._container_level_dir_list = []
        self._object_level_dir_list = []
        self._global_map = {}
        self._global_map_version = 0.0
        self._account_service_map = []
        self._account_service_map_version = None
        self._container_service_map = []
        self._container_service_map_version = None
        self._object_service_map = []
        self._object_service_map_version = None
        self._account_updater_map = []
        self._account_updater_map_version = None 
        self._update_map_request = False
        self.gl_map_status = False
        self._result_gl_obj = None
        self.conn_obj = None
         

        for i in range(fs_list_count):
            self._fs_list.append(self.__ring_obj.get_fs_list()[i])

        for i in range(account_dir_count):
            self._account_level_dir_list.append(self.__ring_obj.\
                                     get_account_level_dir_list()[i])

        for i in range(container_dir_count):
            self._container_level_dir_list.append(self.__ring_obj.\
                                     get_container_level_dir_list()[i])

        for i in range(object_dir_count):
            self._object_level_dir_list.append(self.__ring_obj.\
                                     get_object_level_dir_list()[i])

        #GL map maintainer
        self.request_handler = Req(self.logger)
        self._service_id = get_own_service_id()
        self._result_gl_obj = self.load_global_map()
        if self.gl_map_status:
            self._global_map = self._result_gl_obj.message.map()
            self._global_map_version = self._result_gl_obj.message.version()
            self._account_service_map = self._global_map['account-server'][0]
            self._account_service_map_version = \
                self._global_map['account-server'][1]
            self._container_service_map = \
                self._global_map['container-server'][0]
            self._container_service_map_version = \
                self._global_map['container-server'][1]
            self._object_service_map = self._global_map['object-server'][0]
            self._object_service_map_version = \
                self._global_map['object-server'][1]
            self._account_updater_map = \
                self._global_map['accountUpdater-server'][0]
            self._account_updater_map_version = \
                self._global_map['accountUpdater-server'][1]

            self.logger.debug("Global map : %s, Global map version : %s " \
                %(self._global_map, self._global_map_version))
            if self.conn_obj:
                self.conn_obj.close()

    def load_global_map(self):
        retry = 3
        counter = 0
        self.logger.debug("Sending request for retrieving global map")
        self.gl_map_status = False
        while counter < retry:
            counter += 1
            try:
                self.gl_info_obj = self.request_handler.get_gl_info()
                self.conn_obj = self.request_handler.connector(IoType.EVENTIO, \
                    self.gl_info_obj)
                if self.conn_obj != None:
                    self._result_gl_obj = self.request_handler.get_global_map(\
                        self._service_id, self.conn_obj)
                    if self._result_gl_obj.status.get_status_code() == \
                        Resp.SUCCESS:
                        self.gl_map_status = True
                        return self._result_gl_obj
                    else:
                        self.logger.warning("get_global_map() error code:%s msg:%s"\
                            % (self._result_gl_obj.status.get_status_code(), \
                            self._result_gl_obj.status.get_status_message()))
                        self.conn_obj.close()
            except Exception as ex:
                self.logger.exception("Exception occured during get_global_map"
                    ":%s" %ex)
                if self.conn_obj:
                    self.conn_obj.close()
            self.logger.debug("Failed to get global map, Retry: %s" % counter)
        self.logger.error("Failed to load global map")
        return self._result_gl_obj

    def update_all_maps_in_memory(self):
        self._global_map = self.updated_map_obj.message.map()
        self._global_map_version = \
            self.updated_map_obj.message.version()

        self._account_service_map = \
            self._global_map['account-server'][0]
        self._account_service_map_version = \
            self._global_map['account-server'][1]

        self._container_service_map = \
            self._global_map['container-server'][0]
        self._container_service_map_version = \
            self._global_map['container-server'][1]

        self._object_service_map = \
            self._global_map['object-server'][0]
        self._object_service_map_version = \
            self._global_map['object-server'][1]

        self._account_updater_map = \
            self._global_map['accountUpdater-server'][0]
        self._account_updater_map_version = \
            self._global_map['accountUpdater-server'][1]

        self.logger.info("Updated Global map : %s, Updated Global"\
            "map version : %s " %(self._global_map, \
             self._global_map_version))

    def update_global_map(self):
        """
        update global map
        """
        if not self._update_map_request:
            self.logger.debug("Sending request for global map update " \
                "old_map_version:%s" %self._global_map_version)
            self._update_map_request = True
            self.updated_map_obj = self.load_global_map()
            if self.gl_map_status:
                map_diff = float_comp(self._global_map_version, \
                    self.updated_map_obj.message.version())
                if map_diff < 0:
                    self.update_all_maps_in_memory()
                else:
                    self.logger.info("Global map is not updated yet old map" \
                        " version :%s current map version:%s" 
                        %(self._global_map_version, 
                        self.updated_map_obj.message.version()))
                if self.conn_obj:
                    self.conn_obj.close()            
            self._update_map_request = False
        else:
            self.logger.info("Global map update request is already in progress")
            sleep(10)	#need to change 

    def update_object_map(self):
        if not self._update_map_request:
            self.logger.debug("Sending request for global map update " \
                "old_map_version:%s" %self._global_map_version) 
            self._update_map_request = True
            self.updated_map_obj = self.load_global_map()
            if self.gl_map_status:
                map_diff = float_comp(self._global_map_version, \
                    self.updated_map_obj.message.version())
                object_service_map_diff = float_comp(\
                    self._object_service_map_version,
                    self.updated_map_obj.message.map()['object-server'][1])
                if map_diff < 0:
                    self.update_all_maps_in_memory()
                elif object_service_map_diff < 0:
                    self.logger.debug("Object Service map version updated")
                    self.update_all_maps_in_memory()
                else:
                    self.logger.info("Global map is not updated yet old map" \
                        " version :%s current map version:%s"
                        %(self._global_map_version,
                        self.updated_map_obj.message.version()))
                if self.conn_obj:
                    self.conn_obj.close()
            self._update_map_request = False
        else:
            self.logger.info("Global map update request is already in progress")
            sleep(10)   #need to change 

    def get_account_map(self):
        """
        Returns account map list
        """
        return self._account_service_map

    def get_account_map_version(self):
        """
        Returns account map version
        """
        return self._account_service_map_version

    def get_container_map(self):
        """
        Return container map list
        """
        return self._container_service_map

    def get_container_map_version(self):
        """
        Returns container map version
        """
        return self._container_service_map_version

    def get_object_map(self):
        """
        Returns object map list
        """
        return self._object_service_map

    def get_object_map_version(self):
        """
        Returns object map version
        """
        return self._object_service_map_version

    def get_account_updater_map(self):
        """
        Returns account-updater map list
        """
        return self._account_updater_map

    def get_account_updater_map_version(self):
        """
        Returns account-updater map version
        """
        return self._account_updater_map_version

    def get_global_map(self):
        """
        Returns global map
        """
        return self._global_map

    def get_global_map_version(self):
        """
        Returns global map version
        """
        return self._global_map_version

    def get_fs_list(self):
        """
        Returns file system list
        """
        return self._fs_list

    def get_container_level_dir_list(self):
        """
        Returns container level directory list
        """
        return self._container_level_dir_list

    def get_account_level_dir_list(self):
        """
        Returns account level directory list
        """
        return self._account_level_dir_list

    def get_object_level_dir_list(self):
        """
        Returns object level directory list
        """
        return self._object_level_dir_list

    def get_acc_dir_by_hash(self, acc_hash):
        """
        Returns account level directory for object storage
        :param acc_hash: acc hash
        :returns: account level directory
        """
        directories = self._account_level_dir_list
        shift_param = len(directories)
        dir_no = Calculation.evaluate(acc_hash, shift_param)
        try:
            dir_name = directories[dir_no - 1]
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name

    def get_cont_dir_by_hash(self, cont_hash):
        """
        Returns container level directory for object storage
        :param cont_hash: container hash
        :returns: container level directory
        """
        directories = self._container_level_dir_list
        shift_param = len(directories)
        dir_no = Calculation.evaluate(cont_hash, shift_param)
        try:
            dir_name = directories[dir_no - 1]
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name

    @classmethod
    def load(cls, file_location):
        raise NotImplementedError

class ObjectRingData(RingData):
    def __init__(self, file_location, logger=None):
        #super(ObjectRingData, self).__init__(file_location)
        #Commented above line as this is preventing RingData to Singleton
        self.object_ring_data = RingData(file_location, logger)

    def get_object_service_list(self):
        """
        Returns object services list
        """
        pass

    @classmethod
    def load(cls, file_location, logger):
        """
        Load ring data from file

        :returns: A ObjectRingData instance containing the loaded data.
        """
        try:
            ring_data = ObjectRingData(file_location, logger)
        except (IOError, Exception) as err:
            logger.exception("Exception occured :%s" %err)
            raise err
        return ring_data.object_ring_data

class ContainerRingData(RingData):
    def __init__(self, file_location, logger=None):
        #super(ContainerRingData, self).__init__(file_location)
        #Commented above line as this is preventing RingData to Singleton
        self.container_ring_data = RingData(file_location , logger)

    def get_container_service_list(self):
        """
        Returns container services list
        """
        pass
    
    def get_components_list(self):
        """
        Returns components list
        """
        pass

    def get_comp_container_dist(self):
        """
        Returns account component distribution list
        """
        pass

    @classmethod
    def load(cls, file_location, logger):
        """
        Load ring data from file

        :returns: A ContainerRingData instance containing the loaded data.
        """
        try:
            ring_data = ContainerRingData(file_location, logger)
        except (IOError, Exception) as err:
            logger.exception("Exception occured :%s" %err)
            raise err
        return ring_data.container_ring_data

class AccountRingData(RingData):
    def __init__(self, file_location, logger=None):
        #super(AccountRingData, self).__init__(file_location)
        #Commented above line as this is preventing RingData to Singleton
        self.account_ring_data = RingData(file_location, logger)

    def get_account_services_list(self):
        """
        Returns account services list
        """
        pass

    def get_components_list(self):
        """
        Returns components list
        """
        pass

    def get_comp_account_dist(self):
        """
        Returns account component distribution list
        """
        pass

    @classmethod
    def load(cls, file_location, logger):
        """
        Load ring data from file

        :returns: A AccountRingData instance containing the loaded data.
        """
        try:
            ring_data = AccountRingData(file_location, logger)
        except (IOError, Exception) as err:
            logger.exception("Exception occured :%s" %err)
            raise err
        return ring_data.account_ring_data

class Ring(object):
    def __init__(self):
        pass

    def get_service_details(self, service_obj):
        """
        Returns details of a specific service object
        :param service_obj: object of that specific service
        :returns: dictionary containing details of specific service
        """
        data = {}
        data['ip'] = service_obj.get_ip()
        data['port'] = service_obj.get_port()
        return [data]

    def get_service_ids(self):
        raise NotImplementedError

class ObjectRing(Ring):
    __metaclass__ = SingletonType

    def __init__(self, file_location, logger=None, node_timeout=600):
        self.ring_data = RingData(file_location, logger)
        self.logger = logger
        self.node_timeout = node_timeout

    def get_fs_list(self):
        """
        Returns available file system list
        """
        return self.ring_data.get_fs_list()

    def get_object_level_directory(self, account, container, obj):
        """
        Returns object level directory for object storage location
        :param account: account name
        :param container: container name
        :param obj: object name
        :returns: directory no and name
        """
        key = hash_path(account, container, obj)
        try:
            dir_no, dir_name = self.get_obj_dir_by_hash(key)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_no, dir_name

    def get_obj_dir_by_hash(self, obj_hash):
        """
        Returns object level directory for object storage location
        :param obj_hash: object hash
        :returns: directory no and name
        """
        directories = self.ring_data.get_object_level_dir_list()
        shift_param = len(directories)
        dir_no = Calculation.evaluate(obj_hash, shift_param)
        try:
            dir_name = directories[dir_no - 1]
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_no, dir_name

    def get_account_level_directory(self, account):
        """
        Returns account level directory for object storage location
        :param account: account name
        :returns: directory name
        """
        key = hash_path(account)
        try:
            dir_name = self.ring_data.get_acc_dir_by_hash(key)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name

    def get_container_level_directory(self, account, container):
        """
        Returns container level directory for object storage location
        :param account: account name
        :param container: container name
        :returns: directory name
        """
        key = hash_path(account, container)
        try:
            dir_name = self.ring_data.get_cont_dir_by_hash(key)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name


    def get_filesystem(self, directory):
        """
        Returns filesystem on which directory lies
        :param directory: directory no
        :returns: file system on which directory lies
        """
        if not isinstance(directory, int):
            raise TypeError('directory no must be an integer')
        fs_no = int(ceil(float(directory)/2))
        try:
            fs_name = self.ring_data.get_fs_list()[fs_no - 1]
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return fs_name


    def get_node(self, account, container, obj, only_fs_dir_flag= False):
        """
        Returns object service details, filesystem and 
        directory for an object
        :param account: account name
        :param container: container name
        :param obj: object name
        :returns:
                  dictionary containing IP and port of object service
                  filesystem
                  string containing account level, container
                  and object level directory
                  global map version
                  component number
        """
        try:
            account_level_directory = self.get_account_level_directory(account)
            container_level_directory = self.get_container_level_directory( \
                account, container)
            dir_no, object_level_directory = self.get_object_level_directory( \
                account, container, obj)
            filesystem = self.get_filesystem(dir_no)
            directory = os.path.join(account_level_directory, \
                container_level_directory, object_level_directory)
            #Added in case of failing node addition HYD-7271
            if only_fs_dir_flag:
                self.logger.debug("Filesystem: %s dir: %s returned" % (filesystem, directory))
                return filesystem, directory
            with UpdateGlobalMapTimeout(self.node_timeout):
                object_service_obj = get_service_obj('object', self.logger)
            #written a new get_service_obj instead of get_service_id as get_service_id is used by other components like account service container service,  need to check the purpose and then write a common function
            service_details = self.get_service_details( \
                                               object_service_obj)
            global_map_version = self.ring_data.get_global_map_version()
            comp_no = None
            self.logger.info("Service details : %s, Filesystem : %s, \
                Directory : %s, Global map version : %s, Component: %s" \
                %(service_details, filesystem, directory, global_map_version, \
                 comp_no))
        except UpdateGlobalMapTimeout:
            self.logger.exception("Timeout occured during global map update")
            if self.ring_data._update_map_request: 
                self.ring_data._update_map_request = False
                self.ring_data.conn_obj.close()
            if only_fs_dir_flag:
                return '', ''
            return '', '', '', '', ''
        except Exception as err:
            self.logger.exception("Exception occured :%s" % err)
            if only_fs_dir_flag:
                return '', ''
            return '', '', '', '', ''
        return service_details, filesystem, directory, global_map_version,\
               comp_no	#TODO what would be comp_no in case of object service

    def get_directories_by_hash(self, acc_hash, cont_hash, obj_hash):
        """
        Returns filesystem and directory for an object
        :param acc_hash: hash of account
        :param cont_hash: hash of account, container
        :param obj_hash: hash of account, container, obj
        :returns:
                  filesystem
                  string containing both account level, container
                  and object level directory
        """
        try:
            acc_dir = self.ring_data.get_acc_dir_by_hash(acc_hash)
            cont_dir = self.ring_data.get_cont_dir_by_hash(cont_hash)
            dir_no, obj_dir = self.get_obj_dir_by_hash(obj_hash)
            filesystem = self.get_filesystem(dir_no)
            directory = os.path.join(acc_dir, cont_dir, obj_dir)
        except Exception as err:
            self.logger.exception("Exception occured :%s" %err)
            return '', ''
        return filesystem, directory

    def get_service_ids(self):
        """
        Returns list of object service objects
        """
        return [obj.get_id() for obj in self.ring_data.get_object_map()]



class AccountRing(Ring):
    __metaclass__ = SingletonType

    def __init__(self, file_location, logger=None, node_timeout=600):
        self.ring_data = RingData(file_location, logger)
        self.logger = logger
        self.node_timeout = node_timeout

    def get_component(self, account):
        """
        Returns the component responsible for the given account
        :param account: Account name
        :returns: component number responsible for account
        """
        key = hash_path(account)
        while not self.ring_data.get_account_map():
            self.logger.debug("Account map is empty:%s" 
                %self.ring_data.get_account_map())
            self.ring_data.update_global_map()
            sleep(20)
        shift_param = len(self.ring_data.get_account_map())
        comp_no = Calculation.evaluate(key, shift_param) - 1
        return comp_no

    def get_account_service(self, comp_no):
        """
        Returns the service object responsible for the given component.
        :param component: component number
        :returns: account service object
        """
        account_service_obj = self.ring_data.get_account_map()[comp_no]
        return account_service_obj
        
    def get_filesystem(self):
        """
        Returns filesystem on which account info file will be stored
        :param account: account name
        :returns: file system name
        """
        return self.ring_data.get_fs_list()[0]

    def get_directory(self, account):
        """
        Returns account level directory for account info file
        :param account: account name
        :returns: account level directory
        """
        key = hash_path(account)
        try:
            dir_name = self.ring_data.get_acc_dir_by_hash(key)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name

    def get_directory_by_hash(self, key):
        """
        Returns account level directory for account info file
        :param account: account hash
        :returns: account level directory
        """
        try:
            dir_name = self.ring_data.get_acc_dir_by_hash(key)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name


    def get_node(self, account):
        """
        Returns account service details, filesystem and 
        directory for an account
        :param account: account name
        :returns:
                  dictionary containing IP and port of account service
                  filesystem
                  directory
                  global map version
                  component number
        """
        try:
            with UpdateGlobalMapTimeout(self.node_timeout):
                comp_no = self.get_component(account)
            account_service_obj = self.get_account_service(comp_no)
            service_details = self.get_service_details(account_service_obj)
            filesystem = self.get_filesystem()
            directory = self.get_directory(account)
            global_map_version = self.ring_data.get_global_map_version()
            self.logger.info("Service details : %s, Filesystem : %s, \
                Directory : %s, Global map version : %s, Component: %s" \
                %(service_details, filesystem, directory, global_map_version, \
                 comp_no))

        except UpdateGlobalMapTimeout:
            self.logger.exception("Timeout occured during global map update")
            if self.ring_data._update_map_request:
                self.ring_data._update_map_request = False
                self.ring_data.conn_obj.close()
            return '', '', '', '', ''
        except Exception as err:
            self.logger.exception("Exception occured :%s" %err)
            return '', '', '', '', ''
        return service_details, filesystem, directory, global_map_version,\
               comp_no

    def get_service_ids(self):
        """
        Returns list of account service objects
        """
        return [acc_obj.get_id() for acc_obj in \
                self.ring_data.get_account_map()]
        

class ContainerRing(Ring):
    __metaclass__ = SingletonType

    def __init__(self, file_location, logger=None, node_timeout=600):
        self.ring_data = RingData(file_location, logger)
        self.logger = logger
        self.node_timeout = node_timeout

    def get_service_list(self, acc_hash, cont_hash):
        """
        Returns list of container service
        """
        try:
            while not self.ring_data.get_container_map():
                self.logger.debug("Container map is empty:%s" 
                    %self.ring_data.get_container_map())
                self.ring_data.update_global_map()
                sleep(20)
            shift_param = len(self.ring_data.get_container_map())
            comp_no = Calculation.evaluate(cont_hash, shift_param) - 1
            container_service_obj = self.get_container_service(comp_no)
            service_details = self.get_service_details(container_service_obj)
            filesystem = self.get_filesystem()
            directory = self.get_directory_by_hash(acc_hash, cont_hash)
            self.logger.info("Container service details :%s, filesystem :%s, directory :%s"\
            %(service_details, filesystem, directory))
        except Exception as err:
            self.logger.exception("Exception occured :%s" %err)
            return '', '', ''
        return service_details, filesystem, directory

    def get_component(self, account, container):
        """
        Returns the component responsible for the given container
        :param account: account name
        :param container: container name
        :returns: component responsible for container
        """
        key = hash_path(account, container)
        while not self.ring_data.get_container_map():
            self.logger.debug("Container map is empty:%s" 
                %self.ring_data.get_container_map())
            self.ring_data.update_global_map()
            sleep(20)
        shift_param = len(self.ring_data.get_container_map())
        comp_no = Calculation.evaluate(key, shift_param) - 1
        return comp_no

    def get_container_service(self, comp_no):
        """
        Returns the service object responsible for the given component.
        :param component: component number
        :returns: container service object
        """
        container_service_obj = self.ring_data.get_container_map()[comp_no]
        return container_service_obj        

    def get_filesystem(self):
        """
        Returns filesystem on which container info file will be stored
        :param container: container name
        :returns: file system name
        """
        return self.ring_data.get_fs_list()[0]

    def get_account_level_directory(self, account):
        """
        Returns account level directory location for account info file 
        :param account: account name
        :returns: directory
        """
        key = hash_path(account)
        try:
            dir_name = self.ring_data.get_acc_dir_by_hash(key)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name

    def get_container_level_directory(self, account, container):
        """
        Returns container level directory location for account info file 
        :param account: account name
        :param container: container name
        :returns: directory
        """
        key = hash_path(account, container)
        try:
            dir_name = self.ring_data.get_cont_dir_by_hash(key)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name

    def get_directory_by_hash(self, acc_hash, cont_hash):
        """
        Returns directory location for container info file 
        :param acc_hash: account hash
        :param cont_hash: container hash
        :returns: directory
        """
        try:
            dir_name_acc = self.ring_data.get_acc_dir_by_hash(acc_hash)
            dir_name_cont = self.ring_data.get_cont_dir_by_hash(cont_hash)
            dir_name = os.path.join(dir_name_acc, dir_name_cont)
        except (IndexError, Exception) as err:
            self.logger.exception("Exception occured :%s" %err)
            raise err
        return dir_name

    def get_node(self, account, container):
        """
        Returns container service details, filesystem and 
        directory for a container
        :param account: account name
        :param container: container name
        :returns:
                  dictionary containing IP and port of container service
                  filesystem
                  directory
                  global map version
                  component number
        """
        try:
            with UpdateGlobalMapTimeout(self.node_timeout):
                comp_no = self.get_component(account, container)
            container_service_obj = self.get_container_service(comp_no)
            service_details = self.get_service_details(container_service_obj)
            filesystem = self.get_filesystem()
            acc_dir = self.get_account_level_directory(account)
            cont_dir = self.get_container_level_directory(account, container)
            directory = os.path.join(acc_dir, cont_dir)
            global_map_version = self.ring_data.get_global_map_version()
            self.logger.info("Service details : %s, Filesystem : %s, \
                Directory : %s, Global map version : %s, Component: %s" \
                %(service_details, filesystem, directory, global_map_version, \
                 comp_no))

        except UpdateGlobalMapTimeout:
            self.logger.exception("Timeout occured during global map update")
            if self.ring_data._update_map_request:
                self.ring_data._update_map_request = False
                self.ring_data.conn_obj.close()
            return '', '', '', '', ''
        except Exception as err:
            self.logger.exception("Exception occured :%s" %err)
            return '', '', '', '', ''
        return service_details, filesystem, directory, global_map_version,\
               comp_no
       
    def get_service_ids(self):
        """
        Returns list of container service objects
        """
        return [cont_obj.get_id() for cont_obj in \
                self.ring_data.get_container_map()]
 
