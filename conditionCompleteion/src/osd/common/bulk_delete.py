# Copyright (c) 2013 OpenStack Foundation
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

from osd.common import constraints
from osd.common.swob import HTTPRequestEntityTooLarge, \
    HTTPLengthRequired, HTTPBadRequest, HTTPNotImplemented
from osd.common.utils import json, split_path
from xml.sax import saxutils
from urllib import unquote

ACCEPTABLE_FORMATS = ['text/plain', 'application/json', \
                      'application/xml', 'text/xml']

def get_response_body(data_format, data_dict, error_list):
    """
    Returns a properly formatted response body according to format. Handles
    json and xml, otherwise will return text/plain. Note: xml response does not
    include xml declaration.
    :params data_format: resulting format
    :params data_dict: generated data about results.
    :params error_list: list of quoted filenames that failed
    """
    if data_format == 'application/json':
        data_dict['Errors'] = error_list
        return json.dumps(data_dict)
    if data_format and data_format.endswith('/xml'):
        output = '<delete>\n'
        for key in sorted(data_dict):
            xml_key = key.replace(' ', '_').lower()
            output += '<%s>%s</%s>\n' % (xml_key, data_dict[key], xml_key)
        output += '<errors>\n'
        output += '\n'.join(
            ['<object>'
             '<name>%s</name><status>%s</status>'
             '</object>' % (saxutils.escape(name), status) for
             name, status in error_list])
        output += '</errors>\n</delete>\n'
        return output

    output = ''
    for key in sorted(data_dict):
        output += '%s: %s\n' % (key, data_dict[key])
    output += 'Errors:\n'
    output += '\n'.join(
        ['%s, %s' % (name, status)
         for name, status in error_list])
    return output

def bulk_delete_response(data_format, data_dict, error_list):
    yield get_response_body(data_format, data_dict, error_list)


def add_obj_in_map(cont, obj, objs_to_delete_map):
    """Method to add add object in map."""
    if not obj:
        return
    if cont not in objs_to_delete_map:
        objs_to_delete_map[cont] = []
    objs_to_delete_map[cont].append(obj)

def get_objs_to_delete(req, max_deletes_per_request, logger):
    """
    Will populate objs_to_delete with data from request input.
    :params req: a Swob request
    :returns: a list of the contents of req.body when separated by newline.
    :raises: HTTPException on failures
    """
    line = ''
    data_remaining = True
    objs_to_delete_map = {}
    max_path_length = constraints.MAX_CONTAINER_NAME_LENGTH + \
    constraints.MAX_OBJECT_NAME_LENGTH + 2
    if req.content_length is None and \
            req.headers.get('transfer-encoding', '').lower() != 'chunked':
        logger.error("Content lenth is required")
        raise HTTPLengthRequired(request=req)

    while data_remaining:
        if '\n' in line:
            obj_to_delete, line = line.split('\n', 1)
            try:
                if obj_to_delete:
                    if not obj_to_delete.startswith('/'):
                        obj_to_delete = '/' + obj_to_delete
                    cont, obj = split_path(\
                    unquote(obj_to_delete.strip()), 1, 2, True)
                    add_obj_in_map(cont, obj, objs_to_delete_map)
            except ValueError, ex:
                logger.error("Error while spliting cont and obj: %s" % ex)
                raise HTTPBadRequest('Invalid container object pair')
        else:
            data = req.body_file.read(max_path_length)
            logger.debug("Read bytes max size: %s, Read Bytes: %s" \
            % (max_path_length, data))
            if data:
                line += data
            else:
                data_remaining = False
                obj_to_delete = line.strip()
                if obj_to_delete:
                    try:
                        if not obj_to_delete.startswith('/'):
                            obj_to_delete = '/' + obj_to_delete
                        cont, obj  = split_path(\
                        unquote(obj_to_delete.strip()), 1, 2, True)
                        add_obj_in_map(cont, obj, objs_to_delete_map)
                    except ValueError, ex:
                        logger.error("While spliting cont and obj: %s" % ex)
                        raise HTTPBadRequest('Invalid container object pair')
        if len(objs_to_delete_map) > 1:
            logger.error("Multiple container entries are given")
            raise HTTPNotImplemented('Multiple container entries')
        if len(line) > max_path_length * 2:
            logger.error("Invalid File Name: %s" % line)
            raise HTTPBadRequest('Invalid File Name')
    if not objs_to_delete_map:
        raise HTTPBadRequest('Empty request body')
    if len(objs_to_delete_map.values()[0]) > max_deletes_per_request:
        logger.error("max_deletes_per_request: %s, Given: %s" \
        %(max_deletes_per_request, len(objs_to_delete_map.values()[0])))
        raise HTTPRequestEntityTooLarge('Maximum Bulk Deletes: %d per request' \
        % max_deletes_per_request)
    # Uniqueify the objects passed to delete. This is
    # done to ignore the failure of BULK-DELETE due to
    # conflict on same object if lock is acquired twice.
    objs_to_delete_map[objs_to_delete_map.keys()[0]] = list(set(objs_to_delete_map.values()[0]))
    return objs_to_delete_map.popitem()


