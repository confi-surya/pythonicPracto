# Copyright (c) 2010-2013 OpenStack Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import time
from xml.sax import saxutils

from osd.common.swob import HTTPOk, HTTPNoContent
from osd.common.utils import json, normalize_timestamp

import eventlet
import os
import commands
from osd.common.utils import ThreadPool
from osd.common.ring.ring import get_fs_list
HEADERS = ['x-account-read', 'x-account-write']

class FakeAccountBroker(object):
    """
    Quacks like an account broker, but doesn't actually do anything. Responds
    like an account broker would for a real, empty account with no metadata.
    """
    def get_info(self):
        now = normalize_timestamp(time.time())
        return {'container_count': 0,
                'object_count': 0,
                'bytes_used': 0,
                'created_at': now,
                'put_timestamp': now}

    def list_containers_iter(self, *_, **__):
        return []

    @property
    def metadata(self):
        return {}


def account_listing_response(account, req, response_content_type, broker=None,
                             limit='', marker='', end_marker='', prefix='',
                             delimiter=''):
    if broker is None:
        broker = FakeAccountBroker()

    info = broker.get_info()
    resp_headers = {
        'X-Account-Container-Count': info['container_count'],
        'X-Account-Object-Count': info['object_count'],
        'X-Account-Bytes-Used': info['bytes_used'],
        'X-Timestamp': info['created_at'],
        'X-PUT-Timestamp': info['put_timestamp']}
    resp_headers.update((key, value)
                        for key, (value, timestamp) in
                        broker.metadata.iteritems() if value != '')

    account_list = broker.list_containers_iter(limit, marker, end_marker,
                                               prefix, delimiter)
    if response_content_type == 'application/json':
        data = []
        for (name, object_count, bytes_used, is_subdir) in account_list:
            if is_subdir:
                data.append({'subdir': name})
            else:
                data.append({'name': name, 'count': object_count,
                             'bytes': bytes_used})
        account_list = json.dumps(data)
    elif response_content_type.endswith('/xml'):
        output_list = ['<?xml version="1.0" encoding="UTF-8"?>',
                       '<account name=%s>' % saxutils.quoteattr(account)]
        for (name, object_count, bytes_used, is_subdir) in account_list:
            if is_subdir:
                output_list.append(
                    '<subdir name=%s />' % saxutils.quoteattr(name))
            else:
                item = '<container><name>%s</name><count>%s</count>' \
                       '<bytes>%s</bytes></container>' % \
                       (saxutils.escape(name), object_count, bytes_used)
                output_list.append(item)
        output_list.append('</account>')
        account_list = '\n'.join(output_list)
    else:
        if not account_list:
            resp = HTTPNoContent(request=req, headers=resp_headers)
            resp.content_type = response_content_type
            resp.charset = 'utf-8'
            return resp
        account_list = '\n'.join(r[0] for r in account_list) + '\n'
    ret = HTTPOk(body=account_list, request=req, headers=resp_headers)
    ret.content_type = response_content_type
    ret.charset = 'utf-8'
    return ret

class AsyncLibraryHelper(object):
    """Wrap Synchronous libraries"""

    def __init__(self, lib):
        self.__lib = lib
        self.__running = True
        self.__waiting = {}
        self.__next_event_id = 1

    def wait_event_stop(self):
        self.__running = False

    def wait_event(self):
        __pool = ThreadPool()
        self.__running = True #TODO: rasingh (issue : already initialised in constructor,  not required logically)
        while self.__running:
            __ret = __pool.run_in_thread(self.__lib.event_wait)
            if (__ret > 0):
                self.__waiting[__ret].send()
                del self.__waiting[__ret]

    def call(self, func_name, *args):
        func = getattr(self.__lib, func_name)
        __event_id = self.__next_event_id
        self.__next_event_id += 1
        self.__waiting[__event_id] = eventlet.event.Event()
        #ret = func(*(args + (__event_id,))) // TODO: event_id should be first parameter
        ret = func(*((__event_id,) + args))
        self.__waiting[__event_id].wait()
        return ret


class SingletonType(type):
    """ Class to create singleton classes object"""
    __instance = None

    def __call__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(SingletonType, cls).__call__(*args, **kwargs)
        return cls.__instance


def get_filesystem(logger):
    """ Returns the filesystem for info file"""
    fs_list = get_fs_list(logger)
    return fs_list

def temp_clean_up_in_comp_transfer(temp_base_path, server, comp_id, logger):
    temp_path = os.path.join(temp_base_path, server, str(comp_id), 'temp')
    tmp_files = []
    status = True
    try:
        if os.path.exists(temp_path):
            tmp_files = os.listdir(temp_path)
        for tmp_file in tmp_files:
            logger.debug('Removing temp account file: %s' % tmp_file)
            os.unlink(os.path.join(temp_path, tmp_file))
    except Exception, ex:
        logger.error("Error while clean-up : %s" % ex)
        status = False
    #remove temp directory
    #os.rmdir(temp_path)
    return status
