""" Container service transcation manager component"""
import time
import os
import re
import types
import traceback
from datetime import datetime
from uuid import uuid4
from osd.common.request_helpers import is_sys_or_user_meta, \
    get_param, get_listing_content_type
from osd.common.utils import normalize_timestamp, \
    json, convert_to_unsigned_long
from osd.common.ring.ring import hash_path
from osd.common.swob import HTTPInternalServerError, HTTPNotFound, \
    HeaderKeyDict, HTTPException, HTTPConflict, HTTPServerError
from xml.etree.cElementTree import Element, SubElement, tostring
from libContainerLib import ContainerStat, ObjectRecord, LibraryImpl, \
    ContainerStatWithStatus, ListObjectWithStatus, Status, list_less__component_names__greater_
from osd.common.http import HTTP_NOT_FOUND, HTTP_INTERNAL_SERVER_ERROR
from osd.containerService.utils import SingletonType, AsyncLibraryHelper, \
    HEADERS, OsdExceptionCode, get_container_id, TMPDIR
from osd.common.constraints import CONTAINER_LISTING_LIMIT

REQ_BODY_CHUNK_SIZE = 65536

class ContainerOperationManager(object):
    """ Class to handle container service operations"""

    __metaclass__ = SingletonType

    def __init__(self, fs_base, journal_path, node_id,  logger):
        self.logger = logger
        self.__fs_base = fs_base
        #create library instance
        self.__cont_lib = LibraryImpl(journal_path, node_id)
        self.asyn_helper = AsyncLibraryHelper(self.__cont_lib)

    def start_event_wait_func(self):
        self.logger.debug('wait_event called!')
        self.asyn_helper.wait_event()

    def stop_event_wait_func(self):
        self.logger.debug('wait_event_stop called!')
        self.asyn_helper.wait_event_stop()

    def __create_cont(self, path, filesystem, cont_stat, component_number):
        """
        Handles communication with container library
        for new container
        param: path: hashed path of account/container
        """
        try:
            self.logger.debug('Create container interface called')
            status_obj = Status()
            cont_id = "container"
            #cont_id = get_container_id()
            tmp_path = '%s/%s/%s/%s/%s' % (self.__fs_base, \
                filesystem, TMPDIR, cont_id,component_number)
            self.asyn_helper.call("create_container", \
                tmp_path, path, cont_stat, status_obj)
            return status_obj
        except Exception as err:
            self.logger.error(('create_container for %(con_dir)s failed ',
                'close failure: %(exc)s : %(stack)s'),
                {'con_dir' : path, 
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def __update_bulk_delete_records(self, path, object_records):
        """
       Handler communication with container library in case of bulk delete
       """
        try:
            self.logger.debug('update_bulk_delete_records interface called')
            return self.asyn_helper.call \
                ("update_bulk_delete_records", path, object_records)
        except Exception as err:
            self.logger.error(('update_bulk_delete_records for %(con_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'con_dir' : path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def bulk_delete_object(self, filesystem, acc_dir, cont_dir, \
        account, container, object_names, req, resp_dict):
        """
       Handles BULK DELETE request for object
       """
        try:
            path = self.create_path(filesystem, acc_dir, cont_dir, account, container) 
            self.logger.debug(('Delete object called for path: %(path)s'),
                {'path' : path})
            deleted = 254
            # create object stat
            created_at = normalize_timestamp(req.headers['x-timestamp'])
            self.logger.debug("Received obj list: %s" % object_names)
            object_records = []
            for obj in object_names:
                obj_stat = ObjectRecord(1, obj, created_at, int(0), \
                    'application/deleted', 'noetag', deleted, int(0))
                object_records.append(obj_stat)
            status_obj = self.__update_bulk_delete_records(path, object_records)
            status = status_obj.get_return_status()
            self.logger.info(('Status of BULK_DELETE from container library comes '
                'out to be: %(status)s'),
                {'status' : status})
            if status:
                resp_dict['Number Deleted'] = len(object_names)
            else:
                resp_dict['Response Status'] = HTTPServerError().status
            return status
        except Exception as err:
            resp_dict['Response Status'] = HTTPServerError().status
            self.logger.error(('BULK DELETE interface failed for container:'
                ' %(container)s '
                'close failure: %(exc)s : %(stack)s'),
                {'container' : container,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err


    def create_path(self, filesystem, acc_dir, cont_dir, account, container):
        """
        Create hashed path for container library
        param: filesystem: Filesystem name
        param: account: Account name
        param: container: Container name
        """
        acc_path = hash_path(account)
        con_path = hash_path(account, container)
        return '%s/%s/%s/%s/%s/%s' % (self.__fs_base, \
            filesystem, acc_dir, acc_path, cont_dir, con_path)

    def create_updater_path(self, filesystem, acc_dir, cont_dir, account, container):
        """
        Create path for account updater
        param: filesystem: Filesystem name
        param: account: Account name
        param: container: Container name
        """
        return '%s/%s/%s/%s/%s/%s' % (self.__fs_base, \
            filesystem, acc_dir, account, cont_dir, container)

    def put_container(self, filesystem, acc_dir, cont_dir, \
        account, container, metadata, req):
        """
        Handles PUT request for container
        param:filesystem: Filesystem name
        param:account: Account name
        param:container: Container name
        param:req: HTTP Request object
        """
        try:
            # create path
            path = self.create_path(filesystem, acc_dir, cont_dir, account, container)
            # Remove this after container library update
            self.logger.debug(('PUT container called for path: %(path)s'),
                {'path' : path})
            if not os.path.exists(path):
                os.makedirs(path)
            timestamp = normalize_timestamp(req.headers['x-timestamp'])
            created_at = normalize_timestamp(time.time())
            # create container stat object
            cont_stat = ContainerStat(account, container, created_at, \
                timestamp, '0', 0, 0, '', str(uuid4()), 'ADDED', '', metadata)
	    #get component number
	    component_name = req.headers['x-component-number']
            # call container library to create container
            status_obj = self.__create_cont(path, filesystem, cont_stat, component_name)
            status = status_obj.get_return_status()
            self.logger.info(('Status from container library comes '
                'out to be: %(status)s'),
                {'status' : status})
            return status, cont_stat
        except Exception as err:
            self.logger.error(('PUT request failed for account/container:'
                ' %(account)s/%(container)s '
                'close failure: %(exc)s : %(stack)s'),
                {'account' : account, 'container' : container,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def __update_container(self, path, obj_stat):
        """
        Handles communication with container library
        for updating container
        param: path: hashed path of account/container
        param: obj_stat: Object stat
        """
        try:
            self.logger.debug('Update container interface called')
            return self.asyn_helper.call \
                ("update_container", path, obj_stat)
        except Exception as err:
            self.logger.error(('update_container for %(con_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'con_dir' : path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def __get_container_stat(self, path, container_stat_obj, request_from_updater = False):
        """
        Handles communication with container library
        for listing
        param: path: hashed path of account/container
        """
        try:
            self.logger.debug('Get container interface called')
            self.asyn_helper.call("get_container_stat", path, container_stat_obj, request_from_updater)
        except Exception as err:
            self.logger.error(('get_container_stat for %(con_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'con_dir' : path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def put_object(self, filesystem, acc_dir, cont_dir, account, container, obj, req):
        """
        Handles PUT request for object
        param:filesystem: Filesystem name
        param:account: Account name
        param:container: Container name
        param:obj: Object name
        param:req: HTTP Request object
        """
        try:
            # create path
            path = self.create_path(filesystem, acc_dir, cont_dir, account, container)
            self.logger.debug(('PUT object in container called for path:'
                ' %(path)s'), {'path' : path})
            # create object stat
            deleted = 1
            if 'x-duplicate-update' in req.headers:
                deleted = 3
            elif 'x-duplicate-unknown' in req.headers:
                deleted = 255
            else:
                pass
            created_at = normalize_timestamp(req.headers['x-timestamp'])
            size = int(float(req.headers['x-size']))
            old_size = int(float(req.headers.get('x-old-size', 0)))
            # create object record object
            obj_stat = ObjectRecord(1, obj, created_at, size, \
                req.headers['x-content-type'], req.headers['x-etag'], \
		deleted, old_size)
            # call container library to update container
            status_obj = self.__update_container(path, obj_stat)
            status = status_obj.get_return_status()
            self.logger.info(('Status from container library comes'
                'out to be: %(status)s'),
                {'status' : status})
            return status
        except Exception as err:
            self.logger.error(('container update failed for new object PUT',
                'close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def get_metadata(self, req):
        """
        Creates metadata string from request object and
        container stat if present
        param: Request object
        Returns metadata dictionary
        """
        try:
            new_meta = {}
            metadata = {}
            # get metadata from request headers
            metadata.update(
                (key.lower(), value)
                for key, value in req.headers.iteritems()
                if key.lower() in HEADERS or
                is_sys_or_user_meta('container', key))
            for key, value in metadata.iteritems():
                if key == 'x-container-read':
                    new_meta.update({'r-' : value})
                elif key == 'x-container-write':
                    new_meta.update({'w-' : value})
                else:
                    ser_key = key.split('-')[2]
                    if ser_key == 'meta':

                        #Supported a single word key till first '-' 
                        #in the entire metadata header as X-Container-Meta-A
                        #new_key = '%s-%s' % ('m', key.split('-')[3])
                        
                        #SANCHIT: This supports multi-part key for metadata 
                        #such as X-Container-Meta-A-B-C
                        new_key = '%s-%s' % ('m', key.split('-', 3)[-1])
                        new_meta.update({new_key : value})
                    elif ser_key == 'sysmeta':
                        #new_key = '%s-%s' % ('sm', key.split('-')[3])
                        new_key = '%s-%s' % ('sm', key.split('-', 3)[-1])
                        new_meta.update({new_key : value})
                    else:
                        self.logger.debug('Expected metadata not found')
            return new_meta
        except Exception as err:
            self.logger.error(('get_metadata failed ',
                'close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def post(self, filesystem, acc_dir, cont_dir, account, container, metadata):
        """
        Handles POST request for container
        param:filesystem: Filesystem name
        param:account: Account name
        param:container: Container name
        """
        try:
            # create path
            path = self.create_path(filesystem, acc_dir, cont_dir, account, container)
            self.logger.debug(('POST container called for path: %(path)s'),
                {'path' : path})
            cont_stat = ContainerStat(account, container, '0', \
                '0', '0', 0, 0, '', '', 'POST', '', metadata)
            self.logger.debug('Called set container stat interface of library')
            status_obj = Status()
            self.asyn_helper.call("set_container_stat", path, cont_stat, status_obj)
            status = status_obj.get_return_status()
            self.logger.info(('Status from container library comes '
                'out to be: %(status)s'),
                {'status' : status})
            return status
        except Exception as err:
            self.logger.error(('POST request failed for account/container:',
                ' %(account)s/%(container)s '
                'close failure: %(exc)s : %(stack)s'),
                {'account' : account, 'container' : container,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def delete_container(self, filesystem, acc_dir, cont_dir, account, container):
        """
        Handles DELETE request for container
        param:filesystem: Filesystem name
        param:account: Account name
        param:container: Container name
        """
        try:
            # create path
            path = self.create_path(filesystem, acc_dir, cont_dir, account, container)
            self.logger.debug(('DELETE container called for path: %(path)s'),
                {'path' : path})
            # call container library to confirm if container is empty or not
            self.logger.debug('Called list container interface of library')
            list_obj = ListObjectWithStatus()
            self.asyn_helper.call("list_container", \
                path, list_obj, CONTAINER_LISTING_LIMIT, '', '', '', '')
            status = list_obj.get_return_status()
            self.logger.info(('Status from container library comes '
                'out to be: %(status)s'), {'status' : status})
            if status != OsdExceptionCode.OSD_OPERATION_SUCCESS:
                return status
            container_list = list_obj.object_record
            self.logger.debug('Got container list')
            if container_list:
                self.logger.debug('object list found in container!')
                raise HTTPConflict()
            # call container library to delete container
            self.logger.debug('Called delete container interface of library')
            status_obj = Status()
            self.asyn_helper.call("delete_container", path, status_obj)
            status = status_obj.get_return_status()
            self.logger.info(('Status from container library comes '
                'out to be: %(status)s'),
                {'status' : status})
            return status
        except Exception as err:
            self.logger.error(('container DELETE failed for account/container:'
                ' %(account)s/%(container)s '
                'close failure: %(exc)s : %(stack)s'),
                {'account' : account, 'container' : container,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def delete_object(self, filesystem, acc_dir, cont_dir, \
        account, container, obj, req):
        """
        Handles DELETE request for object
        param:filesystem: Filesystem name
        param:account: Account name
        param:container: Container name
        param:obj: Object name
        param:req: HTTP Request object
        """
        try:
            # create path
            path = self.create_path(filesystem, acc_dir, cont_dir, account, container) 
            self.logger.debug(('Delete object called for path: %(path)s'),
                {'path' : path})
            deleted = 2
            if 'x-duplicate-unknown' in req.headers:
                deleted = 254
            size = 0
            if 'x-size' in req.headers:
                size = int(float(req.headers['x-size']))
            # create object stat
            created_at = normalize_timestamp(req.headers['x-timestamp'])
            # create object record object
            old_size = int(float(req.headers.get('x-old-size', 0)))
            obj_stat = ObjectRecord(1, obj, created_at, size, \
                'application/deleted', 'noetag', deleted, old_size)
            # call container library to update container
            status_obj = self.__update_container(path, obj_stat)
            status = status_obj.get_return_status()
            self.logger.info(('Status from container library comes '
                'out to be: %(status)s'),
                {'status' : status})
            return status
        except Exception as err:
            self.logger.error(('DELETE object in container failed for:'
                ' %(obj)s '
                'close failure: %(exc)s : %(stack)s'),
                {'obj' : obj,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def get_cont_stat(self, path, request_from_updater = False):
        """
        Helper function to convert object to headers
        param:path: Hashed account/container path
        """
        try:
            self.logger.debug('Called get container stat interface of library')
            container_stat_obj = ContainerStatWithStatus()
            self.__get_container_stat(path, container_stat_obj, request_from_updater)
            status = container_stat_obj.get_return_status()
            self.logger.info(('Status from container library comes '
                'out to be: %(status)s'),
                {'status' : status})
            if status == OsdExceptionCode.OSD_INTERNAL_ERROR:
                self.logger.debug('Internal error raised from library')
                return HTTPInternalServerError
            elif status == OsdExceptionCode.OSD_FILE_OPERATION_ERROR:
                self.logger.debug('File operatiopn error raised from library')
                return HTTPInternalServerError
            elif status == OsdExceptionCode.OSD_NOT_FOUND:
                self.logger.debug('File not found error raised from library')
                return HTTPNotFound
            else:
                pass
            cont_stat = container_stat_obj.container_stat
            return {'account' : cont_stat.account, \
                'container' : cont_stat.container, \
                'created_at' : cont_stat.created_at, \
                'put_timestamp' : cont_stat.put_timestamp , \
                'delete_timestamp' : cont_stat.delete_timestamp, \
                'object_count' : cont_stat.object_count, \
                'bytes_used' : cont_stat.bytes_used, \
                'hash' : cont_stat.hash, 'id' : cont_stat.id, \
                'status' : cont_stat.status, \
                'status_changed_at' : cont_stat.status_changed_at, \
                'metadata' : cont_stat.metadata}
        except Exception as err:
            self.logger.exception(err)
            raise err

    def __head_or_get(self, path):
        """
        Provide headers for HEAD and GET requests
        param:path: Hashed account/container path
        """
        try:
            info = self.get_cont_stat(path)
            if not isinstance(info, types.DictType):
                raise info()
            headers = HeaderKeyDict({
                'X-Container-Object-Count': info['object_count'],
                'X-Container-Bytes-Used': info['bytes_used'],
                'X-Timestamp': info['created_at'],
                'X-PUT-Timestamp': info['put_timestamp'],
            })
            metadata = info['metadata']
            for key, value in metadata.iteritems():
                if key == 'r-':
                    headers.update({'x-container-read' : value})
                elif key == 'w-':
                    headers.update({'x-container-write' : value})
                else:
                    ser_key = key.split('-')[0]
                    if ser_key == 'm':
                        #Supported a single word key till first '-' 
                        #in the entire metadata header as X-Container-Meta-A
                        #key = 'x-container-meta-' + key.split('-')[1]
                        
                        #SANCHIT: This supports multi-part key for metadata 
                        #such as X-Container-Meta-A-B-C
                        key = 'x-container-meta-' + key.split('-', 1)[1]
                    else:
                        #key = 'x-container-sysmeta-' + key.split('-')[1]
                        key = 'x-container-sysmeta-' + key.split('-', 1)[1]
                    headers.update({key : value})
            return headers
        except HTTPException as error:
            self.logger.exception(error)
            return error.status_int
        except Exception as err:
            self.logger.exception(err)
            return HTTP_INTERNAL_SERVER_ERROR

    def __updater_headers(self, path, req_from_updater):
        """
        Provide headers for HEAD requests from account updater
        param:path: Hashed account/container path
        """
        try:
            self.logger.info("Request from account-updater")
            info = self.get_cont_stat(path, req_from_updater)
            if not isinstance(info, types.DictType):
                raise info()
            headers = HeaderKeyDict({
                'X-Container-Object-Count': info['object_count'],
                'X-Container-Bytes-Used': info['bytes_used'],
                'X-DELETE-Timestamp': info['delete_timestamp'],
                'X-PUT-Timestamp': info['put_timestamp'],
                'X-Container' : info['container']
            })
            return headers
        except HTTPException as error:
            self.logger.exception(error)
            return error.status_int
        except Exception as err:
            self.logger.exception(err)
            return HTTP_INTERNAL_SERVER_ERROR

    def head(self, filesystem, acc_dir, cont_dir, account, container, req):
        """
        Handles HEAD request for container
        param:filesystem: Filesystem name
        param:account: Account name
        param:container: Container name
        param:req: HTTP Request object
        """
        try:
            # create path
            path, headers = '', ''
            if 'x-updater-request' in req.headers:
                path = self.create_updater_path(filesystem, acc_dir, cont_dir, account, container)
                self.logger.debug(('HEAD container called for path: %(path)s'),
                    {'path' : path})
                # get headers for updater request
                headers = self.__updater_headers(path , True) #pass True in updater request
            else:
                path = self.create_path(filesystem, acc_dir, cont_dir, account, container)
                self.logger.debug(('HEAD container called for path: %(path)s'),
                    {'path' : path})
                # get headers for request
                headers = self.__head_or_get(path)
            if headers == HTTP_INTERNAL_SERVER_ERROR:
                self.logger.debug('Internal error raised from library')
                raise HTTPInternalServerError(request=req)
            if headers == HTTP_NOT_FOUND and 'x-updater-request' in req.headers:
                self.logger.debug('File not found error raised from library: updater case')
                raise HTTPNotFound(request=req)
            elif headers == HTTP_NOT_FOUND:
                self.logger.debug('File not found error raised from library')
                raise HTTPNotFound(request=req)
            else:
                out_content_type = get_listing_content_type(req)
                headers['Content-Type'] = out_content_type
            return headers
        except HTTPException as error:
            self.logger.exception(error)
            raise error
        except Exception as err:
            self.logger.error \
                (('HEAD request failed for container: %(container)s '),
                {'container' : container})
            self.logger.exception(err)
            raise err

    def __update_data_record(self, obj):
        """
        Converts created time to iso timestamp.
        param:obj: object entry record
        """
        if obj.content_type == 'None':
            return {'subdir': obj.get_name()}
        response = {'bytes': obj.get_size(), 'hash': obj.get_etag(), \
                   'name': obj.get_name(), \
                   'content_type': obj.content_type}
        last_modified = datetime.utcfromtimestamp \
            (float(obj.get_creation_time())).isoformat()
        # python isoformat() doesn't include msecs when zero
        if len(last_modified) < len("1970-01-01T00:00:00.000000"):
            last_modified += ".000000"
        response['last_modified'] = last_modified
        #override_bytes_from_content_type(response, logger=self.logger)
        return response

    def __get_cont_list(self, path, container, req):
        """
        Helper function to get list of objects in container
        param:path: Hashed account/container path
        param:account: Account name
        param:container: Container name
        param:req: HTTP Request object
        """
        ret = ''
        marker = get_param(req, 'marker', '')
        end_marker = get_param(req, 'end_marker', '')
        limit = get_param(req, 'limit', str(CONTAINER_LISTING_LIMIT))
        if limit:
            limit = int(limit)
        else:
            limit = CONTAINER_LISTING_LIMIT
        prefix = get_param(req, 'prefix', '')
        delimiter = get_param(req, 'delimiter', '')
        out_content_type = get_listing_content_type(req)
        # get list of objects
        container_list = []
        if limit > 0:
            self.logger.debug('Called list container interface of library')
            list_obj = ListObjectWithStatus()
            self.asyn_helper.call("list_container", path, list_obj, limit, \
                marker, end_marker, prefix, delimiter)
            status = list_obj.get_return_status()
            self.logger.info(('Status from container library comes '
                'out to be: %(status)s'),
                {'status' : status})
            if status != OsdExceptionCode.OSD_OPERATION_SUCCESS:
                return status
            container_list = list_obj.object_record
            self.logger.debug('Got container list')
        # modify the list for delimiter
        if delimiter:
            container_list_new = []
            for obj in container_list:
                name = obj.get_name()
                if prefix:
                    match = re.match("^" + prefix + ".*", name)
                    if match:
                        replace = re.sub("^" + prefix, '', name)
                        replace = replace.split(delimiter)
                        if len(replace) == 2 or len(replace) > 2:
                            obj.set_name(prefix + replace[0] + delimiter)
                        else:
                            obj.set_name(prefix + replace[0])
                else:
                    replace = name.split(delimiter)
                    if len(replace) >= 2:
                        obj.set_name(replace[0] + delimiter)
                if delimiter in obj.get_name() and obj.get_name().endswith(delimiter):
                    obj.content_type = "None"
                if marker != obj.get_name() or marker > obj.get_name():
                    container_list_new.append(obj)
            container_list = container_list_new
        # Get body of response
        if out_content_type == 'application/json':
            ret = json.dumps([self.__update_data_record(record)
                                   for record in container_list])
        elif out_content_type.endswith('/xml'):
            doc = Element('container', name=container.decode('utf-8'))
            for obj in container_list:
                record = self.__update_data_record(obj)
                if 'subdir' in record:
                    name = record['subdir'].decode('utf-8')
                    sub = SubElement(doc, 'subdir', name=name)
                    SubElement(sub, 'name').text = name
                else:
                    obj_element = SubElement(doc, 'object')
                    for field in ["name", "hash", "bytes", "content_type",
                                  "last_modified"]:
                        SubElement(obj_element, field).text = str(
                            record.pop(field)).decode('utf-8')
                    for field in sorted(record):
                        SubElement(obj_element, field).text = str(
                            record[field]).decode('utf-8')
            ret = tostring(doc, encoding='UTF-8').replace(
                "<?xml version='1.0' encoding='UTF-8'?>",
                '<?xml version="1.0" encoding="UTF-8"?>', 1)
        else:
            if not container_list:
                self.logger.debug('No object list found!')
                return ret
            ret = '%s\n' % ('\n'.join([obj.get_name() \
                for obj in container_list]))
        return ret

    def get(self, filesystem, acc_dir, cont_dir, account, container, req):
        """
        Handles GET request for container
        param:filesystem: Filesystem name
        param:account: Account name
        param:container: Container name
        param:req: HTTP Request object
        """
        try:
            # create path
            path = self.create_path(filesystem, acc_dir, cont_dir, account, container)
            self.logger.debug(('GET container called for path: %(path)s'),
                {'path' : path})
            # get headers for response
            headers = self.__head_or_get(path)
            if headers == HTTP_INTERNAL_SERVER_ERROR:
                self.logger.debug('Internal error raised from library in HEAD')
                raise HTTPInternalServerError(request=req)
            elif headers == HTTP_NOT_FOUND:
                self.logger.debug('File not found error raised from '
                    'library in HEAD')
                raise HTTPNotFound(request=req)
            else:
                out_content_type = get_listing_content_type(req)
                headers['Content-Type'] = out_content_type
            # get response to be send
            ret = self.__get_cont_list(path, container, req)
            if ret == OsdExceptionCode.OSD_INTERNAL_ERROR:
                self.logger.debug('Internal error raised from library in GET')
                raise HTTPInternalServerError(request=req)
            elif ret == OsdExceptionCode.OSD_NOT_FOUND:
                self.logger.debug('File not found error raised from '
                    'library in GET')
                raise HTTPNotFound(request=req)
            else:
                pass
            return ret, headers
        except HTTPException as error:
            self.logger.exception(error)
            raise error
        except Exception as err:
            self.logger.error(('GET request failed for '
                'container: %(container)s '),
                {'container' : container})
            self.logger.exception(err)
            raise err

    def start_recovery_cont_lib(self,my_components):
        """
        Start recovery of container library
        """
        try:
            self.logger.debug('Called start recovery of Container Library')
            return self.__cont_lib.start_recovery(my_components)
        except Exception as err:
            self.logger.exception(err)
            return False

    def __accept_container_data(self, object_records, x_recovery_request, component_list):
        try:
            self.logger.debug('accept_container_data interface called with X-Recovery-Request is %s' %x_recovery_request)
            self.logger.debug('accept_container_data interface called with Component_list : %s' %component_list)
            return self.asyn_helper.call \
                ("accept_container_data", object_records, bool(x_recovery_request), component_list)
        except Exception as err:
            self.logger.error('accept_container_data failed with exception err: %s' %err)
            self.logger.error('accept_container_data failed '.join(traceback.format_stack()))
	    raise err

    def accept_components_cont_data(self,object_records, x_recovery_request, component_list):
        try:
            status_obj = self.__accept_container_data(object_records, x_recovery_request, component_list)
            status = status_obj.get_return_status()
            return status
        except Exception as err:
            self.logger.error(('accept_container_data failed'
                'close failure: %(exc)s : %(stack)s'),
                {'obj' : status_obj,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err


    def commit_recovered_data(self,components, base_version_changed):
	#Call the commit_recovered_data function of container library
        try:
            self.logger.debug('commit_recovered_data interface called')
            status_obj = self.asyn_helper.call('commit_recovered_data', components, base_version_changed)
	    status = status_obj.get_return_status()
	    return status
        except Exception as err:
            self.logger.error('commit_recovered_data failed'.join(traceback.format_stack()))
	    raise err

    def extract_container_data(self,components):
        try:
            return self.__cont_lib.extract_container_data(components)
        except Exception as err:
            self.logger.error('extract_container_data failed'.join(traceback.format_stack()))
	    raise err

    def flush_all_data(self):
	try:
            status_obj =  self.asyn_helper.call \
                            ("flush_all_data")	
            return status_obj.get_return_status()
        except Exception as err:
            self.logger.error('flush_all_data failed'.join(traceback.format_stack()))
            raise err

    def revert_back_cont_data(self,container_entries, memory_flag):
        try:
            self.__cont_lib.revert_back_cont_data(container_entries, memory_flag)
        except Exception as err:
            self.logger.error('revert_back_cont_data failed'.join(traceback.format_stack()))
            raise err

    def remove_transferred_record(self,component_list):
        try:
            comp_list = list_less__component_names__greater_()
            for comp in component_list:
                comp_list.append(comp)
            self.__cont_lib.remove_transferred_records(comp_list)
        except Exception as err:      
            self.logger.error('remove_transferred_record failed'.join(traceback.format_stack()))
            raise err
