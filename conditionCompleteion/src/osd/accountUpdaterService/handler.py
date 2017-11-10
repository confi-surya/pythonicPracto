from threading import Thread
import os
import time
import httplib
from osd.accountUpdaterService.communicator import ConnectionCreator, \
AccountServiceCommunicator
from osd.accountUpdaterService.monitor import GlobalVariables
from osd.common.ring.ring import ContainerRing


class AccountInfo:
    """
    Store the account information
    """
    def __init__(self, account_name, conf, logger):
        """
        Constructor for Account

        :param account_name: account name
        """
        self.account_name = account_name
        self.account_map = {}
        self.acc_update_failed = False
        self.__container_list = []
        self.container_ring = ContainerRing(conf['osd_dir'], logger)
        self.stat_info = {}
        self.conf = conf
        self.logger = logger
        self.record_instance = []
        #self.logger.debug("AccountInfo constructed")
        self.__stat_reader_max_count = int(conf['reader_max_count'])
        self.connection_creator = ConnectionCreator(self.conf, self.logger)

    def getAccountName(self):
        """
        Return account name
        """
        return self.account_name

    def is_container(self, container):
        """
        Checking if container exists in container_list

        :param container : container name
        """
        if container in self.__container_list:
            return True

    def add_container(self, container):
        """
        Adding container in container_list

        :param container: container name
        """
        self.__container_list.append(container)

    def remove_container(self):
        """
        Removing all containers from container_list
        """
        self.__container_list = []

    def get_container_path(self):
        """
        Getting container path for HEAD request.
        """
        for container_name in self.__container_list:
            #node, fs, dir, gl_version, comp_no  = \
            #    self.container_ring.get_node(self.account_name, container_name)
            if self.account_map.get((self.account_name, container_name)) == "success":
                continue
            fs = self.container_ring.get_filesystem()
            self.__container_path = os.path.join('/', fs, \
                self.container_ring.get_directory_by_hash(\
                self.account_name, container_name), \
                self.account_name, container_name)
            self.logger.debug('Path for account: %s and container: %s is %s' \
                %(self.account_name, container_name, self.__container_path))
            container_instance = ContainerStatReader(
                                                container_name,
                                                self.__container_path,
                                                self.account_name,
                                                self.conf,
                                                self.logger
                                                )
            self.record_instance.append(container_instance)
            self.account_map[self.account_name, container_name] = "start"
        self.logger.debug('Exit from set_account_info')

    def execute(self):
        """
        Read the container stat info.
        """
        self.logger.debug('Enter in execute')
        thread_id = []
        thread_count = 0
        for container_record in self.record_instance:
            if container_record.is_completed():
                self.logger.debug("Already performed HEAD on:%s" %(repr(container_record)))
                continue
            if thread_count <= self.__stat_reader_max_count:
                reader_thread = Thread(target=container_record.\
                    read_container_stat_info, args=(self.account_map, ))
                reader_thread.start()
                thread_id.append(reader_thread)
                thread_count = thread_count +1
                if thread_count >= self.__stat_reader_max_count:
                    for thread in thread_id:
                        if thread.is_alive():
                            thread.join()
                    thread_count = 0
        for thread in thread_id:
            if thread.is_alive():
                thread.join()
        self.logger.info('Number of container stat reads: %s' \
            % len(self.record_instance))

class ContainerDispatcher:
    """
    Dispatch the container.
    """

    def __init__(self, conf, logger):
        """
        Constructor for ContainerDispatcher
       
        """
        self.logger = logger
        #self.logger.debug("ContainerDispatcher constructed")
        self.__account_report = AccountReport(conf, self.logger)

    def dispatcher_executer(self, account_instance):
        """
        Execute the account for read the container stat info 
 
        :param account: instance of AccountInfo class
        """  
        self.logger.debug('Enter in dispatcher_executer')
        account_instance.execute()

    def account_update(self, account_instance):
        return self.__account_report.container_report_to_account_service(\
            account_instance)

class AccountReport:
    """
    Club the multiple container stat info together.
    Send the stat info the account service.
    """

    def __init__(self, conf, logger):
        """
        Constructor for PutRequestBuilder.
        """
        self.logger = logger
        #self.logger.debug("AccountReport constructed")
        self.__account_service_communicator = AccountServiceCommunicator(conf, self.logger)

    def container_report_to_account_service(self, account_instance):
        """
        Build the put request.
        
        :param account_instance: instance of AccountInfo class
        """
        self.logger.debug('Enter in container_report_to_account_service')
        account_instance.acc_update_failed = False
        self.container_stat_info = []
        for reader in account_instance.record_instance:
            self.container_stat_info.append(reader.stat_info)
        ret = self.__account_service_communicator.update_container_stat_info(\
            account_instance.account_name, self.container_stat_info)
        if ret:
            for reader in account_instance.record_instance:
                reader.stat_info = {}
            account_instance.record_instance = []
        else:
            account_instance.acc_update_failed = True
        return ret

class ContainerStatReader:
    """
    Read the container stat info from container file.
    """

    def __init__(self, container_name, container_path, account_name, conf, \
                    logger):
        """
        Constructor for RecordReader.
                
        :param container_name: container name
        :param container_path: container path
        """
        self.__container_name = container_name
        self.__container_path = container_path
        self.__account_name = account_name
        self.stat_info = {}
        self.conf = conf
        self.logger = logger
        self.msg = GlobalVariables(self.logger)
        self.retry_count = 3
        #self.logger.debug("ContainerStatReader constructed")
        self.connection_creator = ConnectionCreator(self.conf, self.logger)
        self.__is_completed = False

    def __repr__(self):
        return "container : %s account: %s" %(self.__container_name, self.__account_name)

    def is_completed(self):
        return self.__is_completed

    def read_container_stat_info(self, account_map):
        """
        Get the stat info of container using H-File interface.
        
        """
        try:
            conn = None
            self.logger.debug("Enter in read_container_stat_info for container")
            conn = self.connection_creator.connect_container('HEAD', \
                self.__account_name, self.__container_name, self.__container_path)
            resp = self.connection_creator.get_http_response_instance(conn)
            while resp.status != 204 and resp.status != 404 and self.retry_count != 0:
                self.retry_count -= 1
                if resp.status == 301 or resp.status == 307:
                    self.msg.load_gl_map()
                conn = self.connection_creator.connect_container('HEAD', \
                    self.__account_name, self.__container_name, self.__container_path)
                resp = self.connection_creator.get_http_response_instance(conn)
            if resp.status == 204:
                headers =  dict(resp.getheaders())
                self.stat_info[self.__container_name] = \
                    {'container' : headers['x-container'], \
                    'put_timestamp' : headers['x-put-timestamp'] , \
                    'object_count' : headers['x-container-object-count'], \
                    'bytes_used' : headers['x-container-bytes-used'], \
                    'delete_timestamp' : '0',
                    'deleted' : False}
                account_map[self.__account_name, self.__container_name] = \
                    "success"
                self.__is_completed = True
                self.logger.info("container stats for %s: %s" \
                %(self.__container_name, self.stat_info[self.__container_name]))
            elif resp.status == 404:
                self.logger.debug("Container info file %s not found" \
                    % self.__container_path)
                self.stat_info[self.__container_name] = \
                    {'container' : '', \
                    'put_timestamp' : '0', \
                    'object_count' : 0, \
                    'bytes_used' : 0, \
                    'delete_timestamp' : '0',
                    'deleted' : True}
                account_map[self.__account_name, self.__container_name] = \
                    "success"
                self.__is_completed = True
            else:
                self.logger.warning("Could not read stats form container: %s" \
                    % self.__container_name)
        except Exception as ex:
            self.logger.error("While getting container stats for:%s, Error: %s" \
                % (self.__container_name, ex))
        finally:
            #If container HEAD fails then update account_map dictionary to 
            #failure for that conatiner
            for key, value in account_map.items():
                if key[0] == self.__account_name and key[1] == \
                    self.__container_name and value != "success":
                    account_map[self.__account_name, self.__container_name] = \
                        "fail"
            if conn:
                conn.close()
                self.logger.debug("Connection closed account_map:%s" \
                    %account_map[self.__account_name, self.__container_name])
