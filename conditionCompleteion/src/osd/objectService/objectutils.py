"""
Helper class for managing transcation locks
"""
from eventlet import Timeout

from osd.common.bufferedhttp import http_connect
from osd.common.http import is_success
from osd.common.exceptions import ConnectionTimeout
from osd.common.utils import get_global_map, get_host_from_map

from eventlet.green.httplib import BadStatusLine
from eventlet import sleep 

# TODO remove this hard coded value for timeouts.
# Need to use the values defind in conf files
CONN_TIMEOUT = 100
NODE_TIMEOUT = 500

def http_connection(host, file_system, directory, headers, method):
    """ 
    Method for sending http request to container
    param host: Host information of container
    param file_system: File system name in which object resides
    param directory: Directory name in which object resides
    param headers: Request header dictionary
    param method: Method name to be called
    return response: HTTP response
    """
    try:
        with ConnectionTimeout(CONN_TIMEOUT): 
            ip_add, port = host.rsplit(':', 1)
            full_path = headers['x-object-path']
            conn = http_connect(ip_add, port, file_system, \
            directory, method, full_path, headers)
        with Timeout(NODE_TIMEOUT):
            response = conn.getresponse()
        return response
    except (Exception, Timeout):
        raise 

class TranscationManager(object):
    """ Utility class for http communication"""
    
    def __init__(self, logger):
        """ initialize method """
        self._logger = logger

    def get_transaction_id_from_resp(self, resp):
        for key, value in resp.getheaders():
            if key == 'x-request-trans-id':
                return value

    def acquire_lock(self, host, file_system, directory, data, callback='', req='None', service_id='', account='', container='', osd_dir=''):
        """ 
        Acquire lock from container
        param host: Host information of container
        param file_system: File system name in which object resides
        param directory: Directory name in which object resides
        param data: Request header dictionary
        return response:  HTTP response body (response.body will contain
        the transcation ID for the request
        """
        method_type = 'GET_LOCK'
        retry_count = 0
        try:
            while retry_count < 3:
                retry_count += 1
                try:
                    response = http_connection(host, file_system, directory, data, \
                        method_type)
                except BadStatusLine as err:
                    self._logger.exception(err)
                    self._logger.error(('ERROR acquire_lock failed ' 
                                        'Retry Count '
                                        '%(host)s/%(fs)s/%(dir)s '
                                        '%(retry_count)s'),
                                        {'host': host, 'fs': file_system, 'dir' : directory,
                                         'retry_count' : retry_count})
                    sleep(5)
                    #Load global map and retry
                    global_map, global_map_version, conn_obj = get_global_map(service_id, self._logger, req)
                    callback(global_map, global_map_version, conn_obj)
                    host = get_host_from_map(account, container, global_map, \
                        osd_dir, self._logger)
                    self._logger.info("Setting global map:%s %s"% \
                        (global_map_version, host))
                    if retry_count == 3:
                        raise err
                    continue
                return response
        except Exception as err:
            self._logger.error((
                'ERROR acquire_lock failed '
                '%(host)s/%(fs)s/%(dir)s '),
                {'host': host, 'fs': file_system, 'dir' : directory})
            self._logger.exception(err)
            raise err

    def release_lock(self, host, file_system, directory, data):
        """ 
        Release lock from container
        param host: Host information of container
        param file_system: File system name in which object resides
        param directory: Directory name in which object resides
        param data: Request header dictionary
        return response: HTTP response
        """
        method_type = 'FREE_LOCK'
        try:
            response = http_connection(host, file_system, directory, data, \
                method_type)
            response.read()
            if not is_success(response.status):
                self._logger.error((
                    'ERROR method %(method)s failed '
                    'response status: %(status)d '),
                    {'method': method_type, 'status': response.status})
            return response
        except Exception as err:
            self._logger.error((
                'ERROR release_lock failed '
                '%(host)s/%(fs)s/%(dir)s '),
                {'host': host, 'fs': file_system, 'dir' : directory})
            self._logger.exception(err)
            raise err

class LockManager(object):
    """ Utility class for managing memory for locks acquired """

    def __init__(self, logger):
        """ creates a memory that holds acquired transcation IDs """
        self.__obj_dict = {}
        self._logger = logger

    def add(self, object_path, trans_id):
        """
        Method to add transation ID to memory
        param object_path: object path with account and container name
        param trans_id: Transcation ID for a request
        return: None
        """
        try:
            if not object_path or not trans_id:
                self._logger.error(('Dictionary update failed for '
                '%(object_path)s::%(trans_id)s'),
                {'object_path' : object_path, 'trans_id' : trans_id})
                raise ValueError('object path or transcation ID is empty ')
            trans_id_list = list()
            if object_path in self.__obj_dict.keys(): 
                if trans_id not in self.__obj_dict[object_path]:
                    trans_id_list = self.__obj_dict[object_path]
                    trans_id_list.append(trans_id)
                    self.__obj_dict.update({object_path:trans_id_list})
            else:
                trans_id_list.append(trans_id)
                self.__obj_dict.update({object_path:trans_id_list})
            self._logger.debug(('Dictionary update successful for '
                '%(object_path)s::%(trans_id)s'), 
                {'object_path' : object_path, 'trans_id' : trans_id})
        except Exception as err:
            self._logger.error(('Dictionary update failed for '
                '%(object_path)s::%(trans_id)s'), 
                {'object_path' : object_path, 'trans_id' : trans_id})
            self._logger.exception(err)
            raise err

    def delete(self, object_path, trans_id):
        """
        Method to delete transation ID from memory
        param trans_id: Transcation ID for a request
        return: None
        """
        try:
            for path, trans_id_list in self.__obj_dict.items():
                if (path == object_path) and (trans_id in trans_id_list):
                    trans_id_list.remove(trans_id)
                    if trans_id_list:
                        self.__obj_dict.update({object_path:trans_id_list})
                    else:
                        del self.__obj_dict[object_path]
                    break
            else:
                self._logger.error(('transcation ID:%(trans_id)s not found'),
                    {'trans_id':trans_id})
                self._logger.exception(KeyError)
                raise KeyError
        except Exception as err:
            self._logger.error(('transcation ID:%(trans_id)s not found'),
                {'trans_id':trans_id})
            self._logger.exception(err)
            raise err
        self._logger.debug(('transcation ID:%(trans_id)s deleted'),
                {'trans_id':trans_id})

    def is_exists(self, object_path):
        """ 
        Method to check if transcation ID exists in memory or not
        param trans_id: Transcation ID for a request
        return: True/False
        """
        if object_path in self.__obj_dict.keys():
            return True
        else:
            return False

    #jai:- to check locks
    def is_empty(self):
        """
        Method to check whether dictionary is empty/not.
        return True/False: if empty/non-empty
        """

        if len(self.__obj_dict.keys()) == 0:
            return True
        else:
            return False

