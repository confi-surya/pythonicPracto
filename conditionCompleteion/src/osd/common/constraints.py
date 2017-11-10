# Copyright (c) 2010-2012 OpenStack Foundation
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

import os
import urllib
from urllib import unquote
from ConfigParser import ConfigParser, NoSectionError, NoOptionError

from osd.common.utils import ismount, SWIFT_CONF_FILE
from osd.common.swob import HTTPBadRequest, HTTPLengthRequired, \
    HTTPRequestEntityTooLarge, HTTPPreconditionFailed

from osd.common.helper_functions import split_path

MAX_FILE_SIZE = 8796093022207
MAX_META_NAME_LENGTH = 128
MAX_META_VALUE_LENGTH = 256
MAX_META_COUNT = 90
MAX_META_OVERALL_SIZE = 4096
MAX_HEADER_SIZE = 8192
MAX_OBJECT_NAME_LENGTH = 1024
REQ_BODY_CHUNK_SIZE = 65536

#constraint container/account listing limit value.
#Default value can be modified in configFiles/osd.conf
CONTAINER_LISTING_LIMIT = 1008
ACCOUNT_LISTING_LIMIT = 1001

MAX_ACCOUNT_NAME_LENGTH = 256
MAX_CONTAINER_NAME_LENGTH = 256
NON_ALLOWED_HEADERS = ['x-account-device', 'x-account-host', 'x-account-meta-quota-bytes', \
                       'x-account-override-deleted', \
                       'x-account-partition', 'x-container-device', \
                       'x-container-host', 'x-container-meta-access-control-allow-headers', \
                       'x-container-meta-access-control-allow-origin', \
                       'x-container-meta-access-control-expose-headers', \
                       'x-container-meta-access-control-max-age', \
                       'x-container-meta-quota-bytes', 'x-container-meta-quota-count', \
                       'x-container-meta-web-error', 'x-container-meta-web-index', \
                       'x-container-meta-web-listings', 'x-container-meta-web-listings-css', \
                       'x-container-partition', 'x-container-sync-key', \
                       'x-container-sync-to', 'x-size', \
                       'x-copied-from-last-modified', 'x-delete-after', \
                       'x-delete-at', 'x-delete-at-device', 'x-delete-at-partition', \
                       'x-delete-at-host', \
                       'x-if-delete-at', 'x-newest', 'x-object-manifest', \
                       'x-object-meta-public', \
                       'x-versions-location', 'x-dynamic-large-object', \
                       'x-last-modified', \
                       'x-remove-versions-location', 'x-web-mode', \
                       'access-control-allow-credentials', \
                       'access-control-allow-methods', \
                       'access-control-allow-origin', \
                       'access-control-expose-headers', \
                       'access-control-max-age', \
                       'access-control-request-headers', \
                       'access-control-request-method', 'origin']

DEFAULT_CONSTRAINTS = {
    'max_file_size': MAX_FILE_SIZE,
    'max_meta_name_length': MAX_META_NAME_LENGTH,
    'max_meta_value_length': MAX_META_VALUE_LENGTH,
    'max_meta_count': MAX_META_COUNT,
    'max_meta_overall_size': MAX_META_OVERALL_SIZE,
    'max_header_size': MAX_HEADER_SIZE,
    'max_object_name_length': MAX_OBJECT_NAME_LENGTH,
    'container_listing_limit': CONTAINER_LISTING_LIMIT,
    'account_listing_limit': ACCOUNT_LISTING_LIMIT,
    'max_account_name_length': MAX_ACCOUNT_NAME_LENGTH,
    'max_container_name_length': MAX_CONTAINER_NAME_LENGTH,
    'non_allowed_headers': NON_ALLOWED_HEADERS
}

SWIFT_CONSTRAINTS_LOADED = False
OVERRIDE_CONSTRAINTS = {}  # any constraints overridden by SWIFT_CONF_FILE
EFFECTIVE_CONSTRAINTS = {}  # populated by reload_constraints


def reload_constraints():
    """
    Parse SWIFT_CONF_FILE and reset module level global contraint attrs,
    populating OVERRIDE_CONSTRAINTS AND EFFECTIVE_CONSTRAINTS along the way.
    """
    global SWIFT_CONSTRAINTS_LOADED, OVERRIDE_CONSTRAINTS
    SWIFT_CONSTRAINTS_LOADED = False
    OVERRIDE_CONSTRAINTS = {}
    constraints_conf = ConfigParser()
    if constraints_conf.read(SWIFT_CONF_FILE):
        SWIFT_CONSTRAINTS_LOADED = True
        for name in DEFAULT_CONSTRAINTS:
            try:
                value = int(constraints_conf.get('osd-constraints', name))
            except (NoSectionError, NoOptionError):
                pass
            else:
                OVERRIDE_CONSTRAINTS[name] = value
    for name, default in DEFAULT_CONSTRAINTS.items():
        value = OVERRIDE_CONSTRAINTS.get(name, default)
        EFFECTIVE_CONSTRAINTS[name] = value
        # "globals" in this context is module level globals, always.
        globals()[name.upper()] = value


reload_constraints()


# Maximum slo segments in buffer
MAX_BUFFERED_SLO_SEGMENTS = 10000


#: Query string format= values to their corresponding content-type values
FORMAT2CONTENT_TYPE = {'plain': 'text/plain', 'json': 'application/json',
                       'xml': 'application/xml'}

def check_container_metadata(req, metadata_dict):
    """
    Check metadata at container service level
    :param req: request object
    :param metadata_dict: Metadata dictionary
    :returns: HTTPBadRequest with bad metadata otherwise None
    """
    prefix = 'x-container-meta-'
    meta_count = 0
    meta_size = 0
    for key, value in metadata_dict.iteritems():
        if isinstance(value, basestring) and len(value) > MAX_HEADER_SIZE:
            return HTTPBadRequest(body='Header value too long: %s' %
                                  key[:MAX_META_NAME_LENGTH],
                                  request=req, content_type='text/plain')
        if not key.lower().startswith(prefix):
            continue
        key = key[len(prefix):]
        if not key:
            return HTTPBadRequest(body='Metadata name cannot be empty',
                                  request=req, content_type='text/plain')
        meta_count += 1
        meta_size += len(key) + len(value)
        if len(key) > MAX_META_NAME_LENGTH:
            return HTTPBadRequest(
                body='Metadata name too long: %s%s' % (prefix, key),
                request=req, content_type='text/plain')
        elif len(value) > MAX_META_VALUE_LENGTH:
            return HTTPBadRequest(
                body='Metadata value longer than %d: %s%s' % (
                    MAX_META_VALUE_LENGTH, prefix, key),
                request=req, content_type='text/plain')
        elif meta_count > MAX_META_COUNT:
            return HTTPBadRequest(
                body='Too many metadata items; max %d' % MAX_META_COUNT,
                request=req, content_type='text/plain')
        elif meta_size > MAX_META_OVERALL_SIZE:
            return HTTPBadRequest(
                body='Total metadata too large; max %d'
                % MAX_META_OVERALL_SIZE,
                request=req, content_type='text/plain')
    return None


def check_metadata(req, target_type):
    """
    Check metadata sent in the request headers.

    :param req: request object
    :param target_type: str: one of: object, container, or account: indicates
                        which type the target storage for the metadata is
    :returns: HTTPBadRequest with bad metadata otherwise None
    """
    prefix = 'x-%s-meta-' % target_type.lower()
    meta_count = 0
    meta_size = 0
    for key, value in req.headers.iteritems():
        if isinstance(value, basestring) and len(value) > MAX_HEADER_SIZE:
            return HTTPBadRequest(body='Header value too long: %s' %
                                  key[:MAX_META_NAME_LENGTH],
                                  request=req, content_type='text/plain')
        if not key.lower().startswith(prefix):
            continue
        key = key[len(prefix):]
        if not key:
            return HTTPBadRequest(body='Metadata name cannot be empty',
                                  request=req, content_type='text/plain')
        meta_count += 1
        meta_size += len(key) + len(value)
        if len(key) > MAX_META_NAME_LENGTH:
            return HTTPBadRequest(
                body='Metadata name too long: %s%s' % (prefix, key),
                request=req, content_type='text/plain')
        elif len(value) > MAX_META_VALUE_LENGTH:
            return HTTPBadRequest(
                body='Metadata value longer than %d: %s%s' % (
                    MAX_META_VALUE_LENGTH, prefix, key),
                request=req, content_type='text/plain')
        elif meta_count > MAX_META_COUNT:
            return HTTPBadRequest(
                body='Too many metadata items; max %d' % MAX_META_COUNT,
                request=req, content_type='text/plain')
        elif meta_size > MAX_META_OVERALL_SIZE:
            return HTTPBadRequest(
                body='Total metadata too large; max %d'
                % MAX_META_OVERALL_SIZE,
                request=req, content_type='text/plain')
        elif ( key == "Max-Read-Bandwidth") or (key == "Max-Write-Bandwidth"):
            for val in value.split(','):
                if not val.isdigit():
                    return HTTPBadRequest(
                    body='Metadata value is not positive integer: %s%s:%s'
                    % (prefix, key, value),
                    request=req, content_type='text/plain')
    return None


def check_object_creation(req, object_name):
    """
    Check to ensure that everything is alright about an object to be created.

    :param req: HTTP request object
    :param object_name: name of object to be created
    :returns HTTPRequestEntityTooLarge: the object is too large
    :returns HTTPLengthRequired: missing content-length header and not
                                 a chunked request
    :returns HTTPBadRequest: missing or bad content-type header, or
                             bad metadata
    """
    if req.content_length and req.content_length > MAX_FILE_SIZE:
        return HTTPRequestEntityTooLarge(body='Your request is too large.',
                                         request=req,
                                         content_type='text/plain')
    if req.content_length is None and \
            req.headers.get('transfer-encoding') != 'chunked':
        return HTTPLengthRequired(request=req)
    if 'X-Copy-From' in req.headers and req.content_length:
        return HTTPBadRequest(body='Copy requests require a zero byte body',
                              request=req, content_type='text/plain')
    if len(object_name) > MAX_OBJECT_NAME_LENGTH:
        return HTTPBadRequest(body='Object name length of %d longer than %d' %
                              (len(object_name), MAX_OBJECT_NAME_LENGTH),
                              request=req, content_type='text/plain')
    if 'Content-Type' not in req.headers:
        return HTTPBadRequest(request=req, content_type='text/plain',
                              body='No content type')
    if not check_utf8(req.headers['Content-Type']):
        return HTTPBadRequest(request=req, body='Invalid Content-Type',
                              content_type='text/plain')
    return check_metadata(req, 'object')


def check_mount(root, drive):
    """
    Verify that the path to the device is a mount point and mounted.  This
    allows us to fast fail on drives that have been unmounted because of
    issues, and also prevents us for accidentally filling up the root
    partition.

    :param root:  base path where the devices are mounted
    :param drive: drive name to be checked
    :returns: True if it is a valid mounted device, False otherwise
    """
    if not (urllib.quote_plus(drive) == drive):
        return False
    path = os.path.join(root, drive)
    return ismount(path)


def check_float(string):
    """
    Helper function for checking if a string can be converted to a float.

    :param string: string to be verified as a float
    :returns: True if the string can be converted to a float, False otherwise
    """
    try:
        float(string)
        return True
    except ValueError:
        return False


def check_utf8(string):
    """
    Validate if a string is valid UTF-8 str or unicode and that it
    does not contain any null character.

    :param string: string to be validated
    :returns: True if the string is valid utf-8 str or unicode and
              contains no null characters, False otherwise
    """
    if not string:
        return False
    try:
        if isinstance(string, unicode):
            string.encode('utf-8')
        else:
            string.decode('UTF-8')
        return '\x00' not in string
    # If string is unicode, decode() will raise UnicodeEncodeError
    # So, we should catch both UnicodeDecodeError & UnicodeEncodeError
    except UnicodeError:
        return False


def check_copy_from_header(req):
    """
    Validate that the value from x-copy-from header is
    well formatted. We assume the caller ensures that
    x-copy-from header is present in req.headers.

    :param req: HTTP request object
    :returns: A tuple with container name and object name
    :raise: HTTPPreconditionFailed if x-copy-from value
            is not well formatted.
    """
    src_header = unquote(req.headers.get('X-Copy-From'))
    if not src_header.startswith('/'):
        src_header = '/' + src_header
    try:
        return split_path(src_header, 2, 2, True)
    except ValueError:
        raise HTTPPreconditionFailed(
            request=req,
            body='X-Copy-From header must be of the form'
                 '<container name>/<object name>')

def check_non_allowed_headers(req):
    """
    Checks whether the headers is allowed in OSD or not.

    :param header: Header to be checked
    :returns: HTTPBadRequest, if header is not allowed
              None, otherwise
    """
    for header in req.headers:
        if header.lower() in NON_ALLOWED_HEADERS:
            return HTTPBadRequest(request=req, body='Unsupported header %s' % header,
                              content_type='text/plain')
        for one in NON_ALLOWED_HEADERS:
            if one in header.lower():
                return HTTPBadRequest(request=req,
                                      body='Unsupported header %s' % header,
                                      content_type='text/plain')
    return None
