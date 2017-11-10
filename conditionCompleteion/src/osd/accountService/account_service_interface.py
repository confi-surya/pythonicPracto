import struct
import traceback
import re
import libaccountLib
from xml.sax import saxutils
from osd.common.swob import HTTPOk
from osd.common.return_code import *
from osd.common.ring.ring import get_service_id
from osd.common.utils import json, normalize_timestamp
from osd.common.swob import HTTPNoContent, HTTPNotFound, \
                                HTTPInternalServerError
from osd.accountService.utils import AsyncLibraryHelper, HEADERS

from osd.common.request_helpers import is_sys_or_user_meta
import time

def convert_to_long(value):
    """Convert to long"""
    return struct.unpack('q', struct.pack('d', float(value)))[0]

def convert_to_normal(value):
    """Convert to normal value"""
    return struct.unpack('d', struct.pack('q', value))[0]


class AccountServiceInterface:
    """An interface class between account service and library."""
    account_lib_obj = libaccountLib.AccountLibraryImpl()
    asyn_helper = AsyncLibraryHelper(account_lib_obj)

    @classmethod
    def start_event_wait_func(cls, logger):
        logger.info('wait_event called!')
        AccountServiceInterface.asyn_helper.wait_event()

    @classmethod
    def stop_event_wait_func(cls, logger):
        logger.info('wait_event_stop called!')
        AccountServiceInterface.asyn_helper.wait_event_stop()

    @classmethod
    def get_metadata(self, req, logger):
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
                is_sys_or_user_meta('account', key))
            for key, value in metadata.iteritems():
                if key == 'x-account-read':
                    new_meta.update({'r-' : value})
                elif key == 'x-account-write':
                    new_meta.update({'w-' : value})
                else:
                    ser_key = key.split('-')[2]
                    if ser_key == 'meta':
                        new_key = '%s-%s' % ('m', key.split('-', 3)[-1])
                        new_meta.update({new_key : value})
                    elif ser_key == 'sysmeta':
                        new_key = '%s-%s' % ('sm', key.split('-', 3)[-1])
                        new_meta.update({new_key : value})
                    else:
                        logger.debug('Expected metadata not found')
            return new_meta
        except Exception as err:
            logger.error(('get_metadata failed ',
                'close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    @classmethod
    def create_AccountStat_object(cls, info):
        """An interface to create an object of AccountStat, from info map"""
        if 'account' not in info:
            info['account'] = '-1'
        if 'created_at' not in info:
            info['created_at'] = '0'
        if 'put_timestamp' not in info:
            info['put_timestamp'] = '0'
        if 'delete_timestamp' not in info:
            info['delete_timestamp'] = '0'
        if 'container_count' not in info:
            info['container_count'] = 0
        if 'object_count' not in info:
            info['object_count'] = 0
        if 'bytes_used' not in info:
            info['bytes_used'] = 0
        if 'hash' not in info:
            info['hash'] = '-1'
        if 'id' not in info:
            info['id'] = '-1'
        if 'status' not in info:
            info['status'] = '-1'
        if 'status_changed_at' not in info:
            info['status_changed_at'] = '0'
        if 'metadata' not in info:
            info['metadata'] = {}
        return libaccountLib.AccountStat(
                                    info['account'],
                                    info['created_at'],
                                    normalize_timestamp(\
                                        info['put_timestamp']),
                                    normalize_timestamp(\
                                        info['delete_timestamp']),
                                    info['container_count'],
                                    info['object_count'],
                                    info['bytes_used'],
                                    info['hash'],
                                    info['id'],
                                    info['status'],
                                    info['status_changed_at'],
                                    info['metadata']
                                    )


    @classmethod
    def create_container_record(cls, name, hash, info, deleted):
        """An interface to create an object of ContainerRecord, from info map"""
        if 'x-object-count' not in info:
            info['x-object-count'] = 0
        if not info['x-put-timestamp']:
            info['x-put-timestamp'] = '0'
        if not info['x-delete-timestamp']:
            info['x-delete-timestamp'] = '0'
        return libaccountLib.ContainerRecord(0, name, hash, \
                normalize_timestamp(str(info['x-put-timestamp'])), \
                normalize_timestamp(str(info['x-delete-timestamp'])), \
                long(info['x-object-count']), \
                long(info['x-bytes-used']), deleted)


    @classmethod
    def create_container_record_for_updater(cls, name, hash, info):
        """An interface to create an object of ContainerRecord for account \
           updater, from info map
        """
        if 'x-object-count' not in info:
            info['x-object-count'] = 0
        if not info['put_timestamp']:
            info['put_timestamp'] = '0'
        if not info['delete_timestamp']:
            info['delete_timestamp'] = '0'
        return libaccountLib.ContainerRecord(0, name, hash, \
                normalize_timestamp(str(info['put_timestamp'])), \
                normalize_timestamp(str(info['delete_timestamp'])), \
                long(info['object_count']), \
                long(info['bytes_used']), \
                info['deleted'])



    @classmethod
    def list_account(cls, temp_path, account_path, out_content_type, req, limit, marker, \
        end_marker, prefix, delimiter, logger):
        """An interface to list the containers in account."""
        logger.debug("Get account stats for path: %s" % account_path)
        container_record_list = []
        resp = libaccountLib.AccountStatWithStatus()
        AccountServiceInterface.__get_account_stat(resp, temp_path, account_path, logger)
        logger.info("Account library responded with: %s for get_account_stat \
            in GET" % resp.get_return_status())
        if resp.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
            resp_headers = {
                'X-Account-Container-Count': \
                    resp.get_account_stat().get_container_count(),
                'X-Account-Object-Count': \
                    resp.get_account_stat().get_object_count(),
                'X-Account-Bytes-Used': 
                    resp.get_account_stat().get_bytes_used(),
                'X-Timestamp': resp.get_account_stat().get_created_at(),
                'X-PUT-Timestamp': resp.get_account_stat().get_put_timestamp()
                }
            modified_meta = {}
            for key, value in resp.get_account_stat().get_metadata().iteritems():
                if key == 'r-':
                    modified_meta.update({'x-account-read' : value})
                elif key == 'w-':
                    modified_meta.update({'x-account-write' : value})
                else:
                    ser_key = key.split('-')[0]
                    if ser_key == 'm':
                        key = 'x-account-meta-' + key.split('-', 1)[1]
                    modified_meta.update({key : value})

            resp_headers.update((key, value) for (key, value) in modified_meta.iteritems())

            if limit:
                logger.debug("Calling list_container")
                resp = libaccountLib.ListContainerWithStatus()
                try:
                    AccountServiceInterface.asyn_helper.call("list_container", \
                            resp, account_path, limit, marker, \
                            end_marker, prefix, delimiter)
                except Exception, err:
                    logger.error(('list_container for %(ac_dir)s failed ' \
                        'close failure: %(exc)s : %(stack)s'), {'ac_dir' : \
                        account_path, 'exc': err, 'stack': \
                        ''.join(traceback.format_stack())})
                    raise err
                logger.info("Account library responded with: %s for \
                    list_container in GET" % resp.get_return_status())
                if resp.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
                    container_record_list = resp.get_container_record()
                    if delimiter:
                        container_list_new = []
                        for obj in container_record_list:
                            name = obj.get_name()
                            if prefix:
                                match = re.match("^" + prefix + ".*", name)
                                if match:
                                    replace = re.sub("^" + prefix, '', name)
                                    replace = replace.split(delimiter)
                                    if len(replace) >= 2:
                                        obj.set_name(prefix + replace[0] + delimiter)
                                        if marker != obj.get_name() or marker > obj.get_name():
                                            container_list_new.append((obj, (0, 1)[delimiter in obj.get_name() and \
                                                                   obj.get_name().endswith(delimiter)]))
                                    else:
                                        obj.set_name(prefix + replace[0])
                                        if marker != obj.get_name() or marker > obj.get_name():
                                            container_list_new.append((obj, 0))
                            else:
                                replace = name.split(delimiter)
                                if len(replace) >= 2:
                                    obj.set_name(replace[0] + delimiter)
                                if marker != obj.get_name() or marker > obj.get_name():
                                    container_list_new.append((obj, (0, 1)[delimiter in obj.get_name() and \
                                                           obj.get_name().endswith(delimiter)]))
                        container_record_list = container_list_new
                    else:
                        container_record_list = [(record, 0) for record in container_record_list]
        if resp.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
            logger.debug("Populating container list")
        elif resp.get_return_status() == INFO_FILE_NOT_FOUND:
            return HTTPNotFound(request=req, charset='utf-8')
        elif resp.get_return_status() == INFO_FILE_DELETED:
            headers = {'X-Account-Status': 'Deleted'}
            return HTTPNotFound(request=req, headers=headers, charset='utf-8', body='')
        else:
            return HTTPInternalServerError(request=req)
        if out_content_type == 'application/json':
            data = []
            for (container_record, is_subdir) in container_record_list:
                if is_subdir:
                    data.append({'subdir': container_record.get_name()})
                else:    
                    data.append({
                                'name': container_record.get_name(),
				'created_at':time.strftime("%a, %d %b %Y %H:%M:%S GMT",time.gmtime(float(container_record.get_put_timestamp()))),
                                'count': container_record.get_object_count(),
                                'bytes': container_record.get_bytes_used()
                                })
            container_record_list = json.dumps(data)
        elif out_content_type.endswith('/xml'):
            #Directly used req.path to get account name.
            output_list = ['<?xml version="1.0" encoding="UTF-8"?>',
                           '<account name=%s>' % \
                           saxutils.quoteattr(req.path.split('/')[3])]
            for (container_record, is_subdir) in container_record_list:
                if is_subdir:
                    output_list.append(
                        '<subdir name=%s />' % saxutils.quoteattr(container_record.get_name()))
                else:
                    item = '<container><name>%s</name><created_at>%s</created_at><count>%s</count>' \
                            '<bytes>%s</bytes></container>' % \
                            (saxutils.escape(container_record.get_name()), \
				time.strftime("%a, %d %b %Y %H:%M:%S GMT",time.gmtime(float(container_record.get_put_timestamp()))), \
                                container_record.get_object_count(), \
                                container_record.get_bytes_used()
                            )
                    output_list.append(item)
            output_list.append('</account>')
            container_record_list = '\n'.join(output_list)
        else:
            if not container_record_list:
                logger.debug("No any container in account")
                resp = HTTPNoContent(request=req, headers=resp_headers)
                resp.content_type = out_content_type
                resp.charset = 'utf-8'
                return resp
            container_record_list = '\n'.join(container_record.get_name() for \
            (container_record, is_subdir) in container_record_list) + '\n'
        ret = HTTPOk(
                    body=container_record_list,
                    request=req, headers=resp_headers
                    )
        ret.content_type = out_content_type
        ret.charset = 'utf-8'
        return ret


    @classmethod
    def create_account(cls, temp_path, account_path, account_stat, logger):
        """An interface to create an account"""
        resp = libaccountLib.Status()
        try:
            AccountServiceInterface.asyn_helper.call("create_account", \
                 resp, temp_path, account_path, account_stat)
        except Exception, err:
            logger.error(('create_account for %(ac_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'ac_dir' : account_path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err
        return resp 
            
    @classmethod
    def add_container(cls, account_path, container_record, logger):
        """An interface to add container record in account"""
        resp = libaccountLib.Status()
        try:
            AccountServiceInterface.asyn_helper.call("add_container", \
                 resp, account_path, container_record)
        except Exception, err:
            logger.error(('add_container for %(ac_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'ac_dir' : account_path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err
        return resp


    @classmethod
    def delete_container(cls, account_path, container_record, logger):
        """An interface to delete container record from account"""
        resp = libaccountLib.Status()
        try:
            AccountServiceInterface.asyn_helper.call("delete_container", \
                 resp, account_path, container_record)
        except Exception, err:
            logger.error(('delete_container for %(ac_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'ac_dir' : account_path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err
        return resp


    @classmethod
    def update_container(cls, account_path, container_data_list, logger):
        """An interface to update container entry"""
        resp = libaccountLib.Status()
        try:
            AccountServiceInterface.asyn_helper.call("update_container", \
                 resp, account_path, container_data_list)
        except Exception, err:
            logger.error(('update_container for %(ac_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'ac_dir' : account_path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err
        return resp

    @classmethod
    def get_account_stat(cls, temp_path, account_path, logger):
        """An interface to get account stat"""
        info = {}
        resp = libaccountLib.AccountStatWithStatus()
        AccountServiceInterface.__get_account_stat(resp, temp_path, account_path, logger)
        logger.debug("get_account_stat responded with return code: %s" \
            % resp.get_return_status())
        if resp.get_return_status() == INFO_FILE_OPERATION_SUCCESS:
            info['container_count'] = \
                resp.get_account_stat().get_container_count()
            info['object_count'] = resp.get_account_stat().get_object_count()
            info['bytes_used'] = resp.get_account_stat().get_bytes_used()
            info['put_timestamp'] = resp.get_account_stat().get_put_timestamp()
            info['account'] = resp.get_account_stat().get_account()
            info['created_at'] = resp.get_account_stat().get_created_at()
            info['delete_timestamp'] = \
                resp.get_account_stat().get_delete_timestamp()
            info['hash'] = resp.get_account_stat().get_hash()
            info['id'] = resp.get_account_stat().get_id()
            info['status'] = resp.get_account_stat().get_status()
            info['status_changed_at'] = \
                resp.get_account_stat().get_status_changed_at()
            #OBB-48: Library responding with incorrect value for metadata.
            #Fix: Since no metata support, returning with empty map.
            info['metadata'] = resp.get_account_stat().get_metadata()
            metadata = info['metadata']
            modified_meta = {}
            for key, value in metadata.iteritems():
                if key == 'r-':
                    modified_meta.update({'x-account-read' : value})
                elif key == 'w-':
                    modified_meta.update({'x-account-write' : value})
                else:
                    ser_key = key.split('-')[0]
                    if ser_key == 'm':
                        key = 'x-account-meta-' + key.split('-', 1)[1]
                    else:
                        key = 'x-account-sysmeta-' + key.split('-', 1)[1]
                    modified_meta.update({key : value})
            info['metadata'] = modified_meta
            logger.debug("get_account_stat from library: %s" % info)
            return resp.get_return_status(), info
        return resp.get_return_status(), None

    @classmethod
    def __get_account_stat(cls, resp, temp_path, account_path, logger):
        """
        Handles communication with account library
        for HEAD request
        """
        try:
            AccountServiceInterface.asyn_helper.call("get_account_stat", \
                resp, temp_path, account_path)
        except Exception, err:
            logger.error(('get_account_stat for %(ac_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'ac_dir' : account_path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    @classmethod
    def set_account_stat(cls, account_path, account_stat, logger):
        """An interface to set account stat"""
        resp = libaccountLib.Status()
        try:
            AccountServiceInterface.asyn_helper.call("set_account_stat", \
                 resp, account_path, account_stat)
        except Exception, err:
            logger.error(('set_account_stat for %(ac_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'ac_dir' : account_path,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err
        return resp

    @classmethod
    def delete_account(cls, temp_path, account_path, logger):
        """An interface to delete account"""
        resp = libaccountLib.Status()
        try:
            AccountServiceInterface.asyn_helper.call("delete_account", resp, \
                temp_path, account_path)
        except Exception, ex:
            logger.error(('set_account_stat for %(ac_dir)s failed '
                'close failure: %(exc)s : %(stack)s'),
                {'exc': ex, 'stack': ''.join(traceback.format_stack())})
            raise ex
        return resp
