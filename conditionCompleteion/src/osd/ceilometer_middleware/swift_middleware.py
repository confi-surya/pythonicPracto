#!/usr/bin/env python
# -*- encoding: utf-8 -*-
#
# Copyright © 2012 eNovance <licensing@enovance.com>
#
# Author: Julien Danjou <julien@danjou.info>
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


"""
Ceilometer Middleware for Swift Proxy

Configuration:

In /etc/swift/proxy-server.conf on the main pipeline add "ceilometer" just
before "proxy-server" and add the following filter in the file:

[filter:ceilometer]
use = egg:ceilometer#swift

# Some optional configuration
# this allow to publish additional metadata
metadata_headers = X-TEST

# Set reseller prefix (defaults to "AUTH_" if not set)
reseller_prefix = AUTH_
"""

from __future__ import absolute_import

from osd.common import utils
from osd.common.helper_functions import split_path
import webob

REQUEST = webob
try:
    # Swift >= 1.7.5
    import osd.common.swob
    REQUEST = osd.common.swob
except ImportError:
    pass

try:
    # Swift > 1.7.5 ... module exists but doesn't contain class.
    from osd.common.utils import InputProxy
except ImportError:
    # Swift <= 1.7.5 ... module exists and has class.
    from osd.common.middleware.proxy_logging import InputProxy

from ceilometer.openstack.common import context
from ceilometer.openstack.common import timeutils
from ceilometer import pipeline
from ceilometer import sample
from ceilometer import service


class CeilometerMiddleware(object):
    """Ceilometer middleware used for counting requests."""

    def __init__(self, app, conf):
        self.app = app
        self.publish_incoming_bytes = conf.get("publish_incoming_bytes", True)
        self.publish_outgoing_bytes = conf.get("publish_outgoing_bytes", True)
        self.publish_on_error = conf.get("publish_on_error", False)
        self.enable_filters = conf.get("enable_filters", True)
        self.error_on_status = [status.strip() for status in conf.get("error_on_status",'').split('\n')]
        self.logger = utils.get_logger(conf, log_route='ceilometer')

        self.metadata_headers = [h.strip().replace('-', '_').lower()
                                 for h in conf.get(
                                     "metadata_headers",
                                     "").split(",") if h.strip()]

        service.prepare_service([])

        self.pipeline_manager = pipeline.setup_pipeline()
        self.reseller_prefix = conf.get('reseller_prefix', 'AUTH_')
        if self.reseller_prefix and self.reseller_prefix[-1] != '_':
            self.reseller_prefix += '_'

    def __call__(self, env, start_response):
        start_response_args = [None]
        request_status = [None]
        try:
            publish_report = (True, False)[env["ceilometer-skip"] and (self.enable_filters == 'True')]
        except:
            publish_report = True
        input_proxy = InputProxy(env['wsgi.input'])
        env['wsgi.input'] = input_proxy

        def my_start_response(status, headers, exc_info=None):
            request_status[0] = status
            start_response_args[0] = (status, list(headers), exc_info)

        def publish_for_status(bytes_sent):
            if (start_response_args[0] and int(dict(start_response_args[0][1])['Content-Length']) == bytes_sent) \
                    and (request_status[0] and not request_status[0].split(' ')[0].strip() in self.error_on_status):
                return True
            return False
                
        def iter_response(iterable):
            iterator = iter(iterable)
            try:
                chunk = iterator.next()
                while not chunk:
                    chunk = iterator.next()
            except StopIteration:
                chunk = ''

            if start_response_args[0]:
                start_response(*start_response_args[0])
            bytes_sent = 0
            try:
                while chunk:
                    bytes_sent += len(chunk)
                    yield chunk
                    chunk = iterator.next()

            except GeneratorExit:
                request_status[0] = '499'
                raise

            finally:
                try:
                    if publish_report and publish_for_status(bytes_sent):
                        bytes_received = input_proxy.bytes_received if self.publish_incoming_bytes == 'True' else 0
                        bytes_sent = (0, bytes_sent)[ self.publish_outgoing_bytes == 'True']
                        self.publish_sample(env,
                                        bytes_received,
                                        bytes_sent)
                except Exception:
                    self.logger.exception('Failed to Publish samples')

        try:
            iterable = self.app(env, my_start_response)
        except Exception:
            if self.publish_incoming_bytes and self.publish_on_error:
                self.publish_sample(env, input_proxy.bytes_received, 0)
                raise
        else:
            return iter_response(iterable)

    def publish_sample(self, env, bytes_received, bytes_sent):
        req = REQUEST.Request(env)
        try:
            version, account, container, obj = split_path(req.path, 2,
                                                                4, True)
        except ValueError:
            return
        now = timeutils.utcnow().isoformat()

        resource_metadata = {
            "path": req.path,
            "version": version,
            "container": container,
            "object": obj,
        }

        for header in self.metadata_headers:
            if header.upper() in req.headers:
                resource_metadata['http_header_%s' % header] = req.headers.get(
                    header.upper())

        with self.pipeline_manager.publisher(
                context.get_admin_context()) as publisher:
            if bytes_received:
                publisher([sample.Sample(
                    name='storage.objects.incoming.bytes',
                    type=sample.TYPE_DELTA,
                    unit='B',
                    volume=bytes_received,
                    user_id=env.get('HTTP_X_USER_ID'),
                    project_id=env.get('HTTP_X_TENANT_ID'),
                    resource_id=account.partition(self.reseller_prefix)[2],
                    timestamp=now,
                    resource_metadata=resource_metadata)])

            if bytes_sent:
                publisher([sample.Sample(
                    name='storage.objects.outgoing.bytes',
                    type=sample.TYPE_DELTA,
                    unit='B',
                    volume=bytes_sent,
                    user_id=env.get('HTTP_X_USER_ID'),
                    project_id=env.get('HTTP_X_TENANT_ID'),
                    resource_id=account.partition(self.reseller_prefix)[2],
                    timestamp=now,
                    resource_metadata=resource_metadata)])

            # publish the event for each request
            # request method will be recorded in the metadata
            resource_metadata['method'] = req.method.lower()
            publisher([sample.Sample(
                name='storage.api.request',
                type=sample.TYPE_DELTA,
                unit='request',
                volume=1,
                user_id=env.get('HTTP_X_USER_ID'),
                project_id=env.get('HTTP_X_TENANT_ID'),
                resource_id=account.partition(self.reseller_prefix)[2],
                timestamp=now,
                resource_metadata=resource_metadata)])


def filter_factory(global_conf, **local_conf):
    conf = global_conf.copy()
    conf.update(local_conf)

    def ceilometer_filter(app):
        return CeilometerMiddleware(app, conf)
    return ceilometer_filter
