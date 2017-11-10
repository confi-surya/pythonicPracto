__all__ = ['FormPost', 'filter_factory', 'READ_CHUNK_SIZE', 'MAX_VALUE_LENGTH']

import hmac
import re
import rfc822
from hashlib import sha1
from time import time
from urllib import quote

from osd.common.utils import streq_const_time, register_osd_info, get_tempurl_keys_from_metadata
from osd.common.wsgi import make_pre_authed_env
from osd.common.swob import HTTPUnauthorized
from osd.proxyService.controllers.base import get_account_info, get_container_info


#: The size of data to read from the form at any given time.
READ_CHUNK_SIZE = 4096

#: The maximum size of any attribute's value. Any additional data will be
#: truncated.
MAX_VALUE_LENGTH = 4096

#: Regular expression to match form attributes.
ATTRIBUTES_RE = re.compile(r'(\w+)=(".*?"|[^";]+)(; ?|$)')


class FormInvalid(Exception):
    pass


class FormUnauthorized(Exception):
    pass


def _parse_attrs(header):
    """
    Given the value of a header like:
    Content-Disposition: form-data; name="somefile"; filename="test.html"

    Return data like
    ("form-data", {"name": "somefile", "filename": "test.html"})

    :param header: Value of a header (the part after the ': ').
    :returns: (value name, dict) of the attribute data parsed (see above).
    """
    attributes = {}
    attrs = ''
    if '; ' in header:
        header, attrs = header.split('; ', 1)
    m = True
    while m:
        m = ATTRIBUTES_RE.match(attrs)
        if m:
            attrs = attrs[len(m.group(0)):]
            attributes[m.group(1)] = m.group(2).strip('"')
    return header, attributes


class _IterRequestsFileLikeObject(object):

    def __init__(self, wsgi_input, boundary, input_buffer):
        self.no_more_data_for_this_file = False
        self.no_more_files = False
        self.wsgi_input = wsgi_input
        self.boundary = boundary
        self.input_buffer = input_buffer

    def read(self, length=None):
        if not length:
            length = READ_CHUNK_SIZE
        if self.no_more_data_for_this_file:
            return ''

        # read enough data to know whether we're going to run
        # into a boundary in next [length] bytes
        if len(self.input_buffer) < length + len(self.boundary) + 2:
            to_read = length + len(self.boundary) + 2
            while to_read > 0:
                chunk = self.wsgi_input.read(to_read)
                to_read -= len(chunk)
                self.input_buffer += chunk
                if not chunk:
                    self.no_more_files = True
                    break

        boundary_pos = self.input_buffer.find(self.boundary)

        # boundary does not exist in the next (length) bytes
        if boundary_pos == -1 or boundary_pos > length:
            ret = self.input_buffer[:length]
            self.input_buffer = self.input_buffer[length:]
        # if it does, just return data up to the boundary
        else:
            ret, self.input_buffer = self.input_buffer.split(self.boundary, 1)
            self.no_more_files = self.input_buffer.startswith('--')
            self.no_more_data_for_this_file = True
            self.input_buffer = self.input_buffer[2:]
        return ret

    def readline(self):
        if self.no_more_data_for_this_file:
            return ''
        boundary_pos = newline_pos = -1
        while newline_pos < 0 and boundary_pos < 0:
            chunk = self.wsgi_input.read(READ_CHUNK_SIZE)
            self.input_buffer += chunk
            newline_pos = self.input_buffer.find('\r\n')
            boundary_pos = self.input_buffer.find(self.boundary)
            if not chunk:
                self.no_more_files = True
                break
        # found a newline
        if newline_pos >= 0 and \
                (boundary_pos < 0 or newline_pos < boundary_pos):
            # Use self.read to ensure any logic there happens...
            ret = ''
            to_read = newline_pos + 2
            while to_read > 0:
                chunk = self.read(to_read)
                # Should never happen since we're reading from input_buffer,
                # but just for completeness...
                if not chunk:
                    break
                to_read -= len(chunk)
                ret += chunk
            return ret
        else:  # no newlines, just return up to next boundary
            return self.read(len(self.input_buffer))


def _iter_requests(wsgi_input, boundary):
    """
    Given a multi-part mime encoded input file object and boundary,
    yield file-like objects for each part.

    :param wsgi_input: The file-like object to read from.
    :param boundary: The mime boundary to separate new file-like
                     objects on.
    :returns: A generator of file-like objects for each part.
    """
    boundary = '--' + boundary
    if wsgi_input.readline().strip() != boundary:
        raise FormInvalid('invalid starting boundary')
    boundary = '\r\n' + boundary
    input_buffer = ''
    done = False
    while not done:
        it = _IterRequestsFileLikeObject(wsgi_input, boundary, input_buffer)
        yield it
        done = it.no_more_files
        input_buffer = it.input_buffer


class _CappedFileLikeObject(object):
    """
    A file-like object wrapping another file-like object that raises
    an EOFError if the amount of data read exceeds a given
    max_file_size.

    :param fp: The file-like object to wrap.
    :param max_file_size: The maximum bytes to read before raising an
                          EOFError.
    """

    def __init__(self, fp, max_file_size):
        self.fp = fp
        self.max_file_size = max_file_size
        self.amount_read = 0

    def read(self, size=None):
        ret = self.fp.read(size)
        self.amount_read += len(ret)
        if self.amount_read > self.max_file_size:
            raise EOFError('max_file_size exceeded')
        return ret

    def readline(self):
        ret = self.fp.readline()
        self.amount_read += len(ret)
        if self.amount_read > self.max_file_size:
            raise EOFError('max_file_size exceeded')
        return ret


class FormPost(object):
    """
    FormPost Middleware

    See above for a full description.

    The proxy logs created for any subrequests made will have osd.source set
    to "FP".

    :param app: The next WSGI filter or app in the paste.deploy
                chain.
    :param conf: The configuration dict for the middleware.
    """

    def __init__(self, app, conf):
        #: The next WSGI application/filter in the paste.deploy pipeline.
        self.app = app
        #: The filter configuration dict.
        self.conf = conf

    def __call__(self, env, start_response):
        """
        Main hook into the WSGI paste.deploy filter/app pipeline.

        :param env: The WSGI environment dict.
        :param start_response: The WSGI start_response hook.
        :returns: Response as per WSGI.
        """
        if env['REQUEST_METHOD'] == 'POST':
            try:
                content_type, attrs = \
                    _parse_attrs(env.get('CONTENT_TYPE') or '')
                if content_type == 'multipart/form-data' and \
                        'boundary' in attrs:
                    http_user_agent = "%s FormPost" % (
                        env.get('HTTP_USER_AGENT', ''))
                    env['HTTP_USER_AGENT'] = http_user_agent.strip()
                    status, headers, body = self._translate_form(
                        env, attrs['boundary'])
                    start_response(status, headers)
                    return body
            except (FormInvalid, EOFError) as err:
                body = 'FormPost: %s' % err
                start_response(
                    '400 Bad Request',
                    (('Content-Type', 'text/plain'),
                     ('Content-Length', str(len(body)))))
                return [body]
            except FormUnauthorized as err:
                message = 'FormPost: %s' % str(err).title()
                return HTTPUnauthorized(body=message)(
                    env, start_response)
        return self.app(env, start_response)

    def _translate_form(self, env, boundary):
        """
        Translates the form data into subrequests and issues a
        response.

        :param env: The WSGI environment dict.
        :param boundary: The MIME type boundary to look for.
        :returns: status_line, headers_list, body
        """
        keys = self._get_keys(env)
        status = message = ''
        attributes = {}
        subheaders = []
        file_count = 0
        for fp in _iter_requests(env['wsgi.input'], boundary):
            hdrs = rfc822.Message(fp, 0)
            disp, attrs = \
                _parse_attrs(hdrs.getheader('Content-Disposition', ''))
            if disp == 'form-data' and attrs.get('filename'):
                file_count += 1
                try:
                    if file_count > int(attributes.get('max_file_count') or 0):
                        status = '400 Bad Request'
                        message = 'max file count exceeded'
                        break
                except ValueError:
                    raise FormInvalid('max_file_count not an integer')
                attributes['filename'] = attrs['filename'] or 'filename'
                if 'content-type' not in attributes and 'content-type' in hdrs:
                    attributes['content-type'] = \
                        hdrs['Content-Type'] or 'application/octet-stream'
                status, subheaders, message = \
                    self._perform_subrequest(env, attributes, fp, keys)
                if status[:1] != '2':
                    break
            else:
                data = ''
                mxln = MAX_VALUE_LENGTH
                while mxln:
                    chunk = fp.read(mxln)
                    if not chunk:
                        break
                    mxln -= len(chunk)
                    data += chunk
                while fp.read(READ_CHUNK_SIZE):
                    pass
                if 'name' in attrs:
                    attributes[attrs['name'].lower()] = data.rstrip('\r\n--')
        if not status:
            status = '400 Bad Request'
            message = 'no files to process'

        headers = [(k, v) for k, v in subheaders
                   if k.lower().startswith('access-control')]

        redirect = attributes.get('redirect')
        if not redirect:
            body = status
            if message:
                body = status + '\r\nFormPost: ' + message.title()
            headers.extend([('Content-Type', 'text/plain'),
                            ('Content-Length', len(body))])
            return status, headers, body
        status = status.split(' ', 1)[0]
        if '?' in redirect:
            redirect += '&'
        else:
            redirect += '?'
        redirect += 'status=%s&message=%s' % (quote(status), quote(message))
        body = '<html><body><p><a href="%s">' \
               'Click to continue...</a></p></body></html>' % redirect
        headers.extend(
            [('Location', redirect), ('Content-Length', str(len(body)))])
        return '303 See Other', headers, body

    def _perform_subrequest(self, orig_env, attributes, fp, keys):
        """
        Performs the subrequest and returns the response.

        :param orig_env: The WSGI environment dict; will only be used
                         to form a new env for the subrequest.
        :param attributes: dict of the attributes of the form so far.
        :param fp: The file-like object containing the request body.
        :param keys: The account keys to validate the signature with.
        :returns: (status_line, headers_list, message)
        """
        if not keys:
            raise FormUnauthorized('invalid signature')
        try:
            max_file_size = int(attributes.get('max_file_size') or 0)
        except ValueError:
            raise FormInvalid('max_file_size not an integer')
        subenv = make_pre_authed_env(orig_env, 'PUT', agent=None,
                                     swift_source='FP')
        if 'QUERY_STRING' in subenv:
            del subenv['QUERY_STRING']
        subenv['HTTP_TRANSFER_ENCODING'] = 'chunked'
        subenv['wsgi.input'] = _CappedFileLikeObject(fp, max_file_size)
        if subenv['PATH_INFO'][-1] != '/' and \
                subenv['PATH_INFO'].count('/') < 4:
            subenv['PATH_INFO'] += '/'
        subenv['PATH_INFO'] += attributes['filename'] or 'filename'
        if 'content-type' in attributes:
            subenv['CONTENT_TYPE'] = \
                attributes['content-type'] or 'application/octet-stream'
        elif 'CONTENT_TYPE' in subenv:
            del subenv['CONTENT_TYPE']
        try:
            if int(attributes.get('expires') or 0) < time():
                raise FormUnauthorized('form expired')
        except ValueError:
            raise FormInvalid('expired not an integer')
        hmac_body = '%s\n%s\n%s\n%s\n%s' % (
            orig_env['PATH_INFO'],
            attributes.get('redirect') or '',
            attributes.get('max_file_size') or '0',
            attributes.get('max_file_count') or '0',
            attributes.get('expires') or '0')


        has_valid_sig = False
        for key in keys:
            sig = hmac.new(key, hmac_body, sha1).hexdigest()
            #TODO-jaivish : LOGS : print "=========== %s, %s,%s" %(key,sig,hmac_body)
            #TODO:LOGS print "Signature from attributes=========%s,%s" %(attributes.get('signature'),attributes)
            if streq_const_time(sig, (attributes.get('signature') or
                                      'invalid')):
                has_valid_sig = True
        if not has_valid_sig:
            raise FormUnauthorized('invalid signature')

        substatus = [None]
        subheaders = [None]

        def _start_response(status, headers, exc_info=None):
            substatus[0] = status
            subheaders[0] = headers

        i = iter(self.app(subenv, _start_response))
        try:
            i.next()
        except StopIteration:
            pass
        return substatus[0], subheaders[0], ''

    def _get_keys(self, env):
        """
        Fetch the tempurl keys for the account. Also validate that the request
        path indicates a valid container; if not, no keys will be returned.

        :param env: The WSGI environment for the request.
        :returns: list of tempurl keys
        """
        parts = env['PATH_INFO'].split('/', 4)
        if len(parts) < 4 or parts[0] or parts[1] != 'v1' or not parts[2] or \
                not parts[3]:
            return []

        #account_info = get_account_info(env, self.app, swift_source='FP')
        #return get_tempurl_keys_from_metadata(account_info['meta'])
        #TODO JAIVISH : Since account post is not supported container keys are searched 
        #TODO : this needs to be changes once account post is supported
        account_info = get_account_info(env, self.app, swift_source='FP')
        account_keys =  get_tempurl_keys_from_metadata(account_info['meta'])
        container_info = get_container_info(env, self.app, swift_source='FP')
        container_keys =  get_tempurl_keys_from_metadata(container_info.get('meta', []))
        #TODO : to print in logs : print "========Keys========== %s, %s" %(account_keys, container_keys)

        return account_keys + container_keys


def filter_factory(global_conf, **local_conf):
    """Returns the WSGI filter for use with paste.deploy."""
    conf = global_conf.copy()
    conf.update(local_conf)
    register_osd_info('formpost')
    return lambda app: FormPost(app, conf)

