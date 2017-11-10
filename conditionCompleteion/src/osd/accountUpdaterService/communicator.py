import os
import httplib
from eventlet.hubs import use_hub
from osd.accountUpdaterService.monitor import GlobalVariables
from osd.common.exceptions import ConnectionTimeout
from eventlet import Timeout
from osd.common.ring.ring import ContainerRing, AccountRing
from osd.common.ring.calculation import Calculation

use_hub("selects")

class HtmlHeaderBuilder:
    """
    Build the Headers.
    """

    def __init__(self):
        """
        Constructor for UpdaterHeaders.
        """
        pass

    def __build_headers(self, header_info):
        """
        Set the Headers.
        
        :param headers: Headers
        """
        headers = {
                   'X-Account': header_info,
                   'user-agent': os.getpid()}
        return headers

    def get_headers(self, header_info):
        """
        :return headers: Headers.
        """
        return self.__build_headers(header_info) 


class ServiceLocator:
    """
    Give the node information. 
    """

    def __init__(self, osd_dir, logger):
        """
        Constructor for UpdaterRing.
        """
        self.container_ring = ContainerRing(osd_dir, logger)
        self.account_ring = AccountRing(osd_dir, logger)
        self.logger = logger
        self.msg = GlobalVariables(self.logger)
        self.shift_param = 512

    def get_service_details(self, service_obj):
        node = {}
        node['ip'] = service_obj.get_ip()
        node['port'] = service_obj.get_port()
        return node
 
    def get_container_from_ring(self, account_hash, container_hash):
        """get container node info from ring"""
        #TODO: Needs to modify to get complete node info(i.e. fs, dir)
        comp_no = Calculation.evaluate(container_hash, self.shift_param) - 1
        node = self.get_service_details(\
            self.msg.get_container_map()[comp_no])
        node['fs'] = self.container_ring.get_filesystem() 
        node['dir'] = self.get_cont_dir_by_hash(\
            account_hash, container_hash)
        return node
    
    def get_account_from_ring(self, account_hash):
        """get account node info from ring"""
        #TODO: Needs to modify to get complete node info(i.e. fs)
        comp_no = Calculation.evaluate(account_hash, self.shift_param) - 1
        node = self.get_service_details(\
            self.msg.get_account_map()[comp_no])
        node['fs'] = self.account_ring.get_filesystem()
        node['account'] = account_hash
        node['dir'] = self.get_acc_dir_by_hash(account_hash)
        return node

    def get_acc_dir_by_hash(self, key):
        return self.account_ring.get_directory_by_hash(key)

    def get_cont_dir_by_hash(self, acc_hash, cont_hash):
        return self.container_ring.get_directory_by_hash(acc_hash, cont_hash)

class AccountUpdaterTimeout:
    """
    Connection timeout for account or container service.
    """ 
    
    def __init__(self):
        """
        Constructor for UpdaterConnectionTimeout.
        
        :param timeout: connectipn timeout .
        """ 
        pass 
     
    def get_connection_timeout(self, timeout):
        """
        Return Connection timeout.
        """ 
        return ConnectionTimeout(timeout) 

    def get_node_timeout(self, timeout):
        """
        Return node timeout. 
        """
        return Timeout(timeout)

class ConnectionCreator:
    """
    Established the connection.
    """
    def __init__(self, conf, logger):
        """
        Constructor for Communicator.
        """
        self.__service_locator = ServiceLocator(conf['osd_dir'], logger)
        self.__account_updater_timeout = AccountUpdaterTimeout()
        self.__html_header_builder = HtmlHeaderBuilder()
        self.__conn_timeout = int(conf['conn_timeout'])
        self.__node_timeout = int(conf['node_timeout'])
        self.conf = conf
        self.logger = logger
        self.msg = GlobalVariables(self.logger)

    def connect_container(self, method, account_name, container_name, \
                                                    container_path):
        self.logger.debug('Enter in connect_container')
        use_hub("selects")
        node = self.__service_locator.get_container_from_ring(\
            account_name, container_name)
        conn = None
        headers = None
        shift_param = 512
        with self.__account_updater_timeout.get_connection_timeout(\
            self.__conn_timeout):
            headers = self.__html_header_builder.get_headers(None)
            headers['x-updater-request'] = True
            headers['x-component-number'] = Calculation.evaluate(\
                  container_name, shift_param) - 1
            headers['x-global-map-version'] = self.msg.get_global_map_version()
            conn = httplib.HTTPConnection(node['ip'], node['port'])
            conn.request(method, container_path, '', headers)
        return conn

    def get_http_connection_instance(self, info, data):
        """
        :return conn: connection object to service.
        """
        self.logger.debug('Enter in get_http_connection_instance')
        headers = None
        conn = None
        shift_param = 512
        if info['flag']:
            node = self.__service_locator.get_container_from_ring(\
                        info['account_name'], data.keys()[0])
        else:
            node = self.__service_locator.get_account_from_ring(
                                                      info['account_name'])
        headers = self.__html_header_builder.get_headers(None)
        info['entity_name'] = info['entity_name'].encode("utf-8")
        with self.__account_updater_timeout.get_connection_timeout(\
            self.__conn_timeout):
            self.logger.debug('Connecting to : node: %s, fs: %s, account: \
                %s, message body size: %s' % (node, node['fs'], \
                info['account_name'], info['body_size']))
            headers['account'] = info['account_name']
            headers['filesystem'] = node['fs']
            headers['dir'] = node['dir']
            headers['x-component-number'] = Calculation.evaluate(\
                  info['account_name'], shift_param) - 1
            headers['x-global-map-version'] = self.msg.get_global_map_version()
            conn = httplib.HTTPConnection(node['ip'], node['port'])
            conn.request("PUT_CONTAINER_RECORD", '', data, headers)
        self.logger.debug('Exit from get_http_connection_instance')
        return conn

    def get_http_response_instance(self, conn):
        """
        Get the response from service.

        :return resp: response from service.
        """
        self.logger.debug('Enter in get_http_response_instance')
        with self.__account_updater_timeout.get_node_timeout(\
            self.__node_timeout):
            resp = conn.getresponse()
            return resp

class AccountServiceCommunicator:
    """
    Communicate with the account service.
    """

    def __init__(self, conf, logger):
        """
        Constructor of AccountServiceCommunicator.
        """
        self.logger = logger
        #self.logger.debug("AccountServiceCommunicator constructed")
        self.__conn_creator = ConnectionCreator(conf, self.logger)
        self.msg = GlobalVariables(self.logger)
    
    def __send_http_request_to_account_service(self, account_name, \
                                                stat_info):
        """
        Send the request to the account service.
        
        :param conn_timeout: connection time to account service
        :param node_timeout: response time to account service
        :return resp: response from account service
        """
        info = {
             'recovery_flag': False,
             'method_name': 'PUT_CONTAINER_RECORD',
             'entity_name': '',
             'body_size': len(str(stat_info)),
             'account_name': account_name,
             'flag':  False}
        return self.__conn_creator.get_http_connection_instance(info, \
            str(stat_info))

    def update_container_stat_info(self, account_name = None, \
                               stat_info = None):
        """ 
        Update the container information to account service.
        """
        updated = False
        retry_count = 3
        self.logger.debug('Sending http request to account service for updation.')
        conn = None
        try:
            conn = self.__send_http_request_to_account_service(
                                                account_name, stat_info)
            resp = self.__conn_creator.get_http_response_instance(conn)
            self.logger.info("Response from account service: status: %s," \
                " message: %s" % (resp.status, resp.read()))
            while resp.status != 202 and resp.status != 404 and retry_count != 0:
                retry_count -= 1
                if resp.status == 301 or resp.status == 307: 
                    self.msg.load_gl_map()
                conn = self.__send_http_request_to_account_service(
                                                account_name, stat_info)
                resp = self.__conn_creator.get_http_response_instance(conn)
                self.logger.info("Response from account service: status: %s," \
                    " message: %s" % (resp.status, resp.read()))
            if resp.status == 202 or resp.status == 404:
                updated = True
        except Exception as ex:
            self.logger.error("Error while updating container stat info: %s" % ex)
        finally:
            if conn:
                conn.close()
        return updated


