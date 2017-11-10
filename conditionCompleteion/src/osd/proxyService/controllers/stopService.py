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
from eventlet import sleep
from osd import gettext_ as _
from osd.common.utils import public, loggit
from osd.proxyService.controllers.base import Controller
from osd.common.swob import HTTPServerError, HTTPOk
from osd.glCommunicator.message_type_enum_pb2 import *
from osd.proxyService.controllers.base import delay_denial

class StopController(Controller):
    """WSGI controller for stop service requests"""

    def __init__(self, app, **kwargs):
        Controller.__init__(self, app)
        self.app.logger.info("STOP_SERVICE constructor")

    @public
    @delay_denial
    @loggit('proxy-server')
    def STOP_SERVICE(self, req):
        """Handler for HTTP STOP_SERVICE requests."""
        '''
        Set stop_service parameter to True 
        :param http request
        return HTTPOk when all ongoing operation completed
        '''
        try:
            self.app.logger.info("STOP_SERVICE request received")
            self.app.request_block()
            self.app.stop_service_flag = True
            while self.app.ongoing_operation_list:
                self.app.logger.info("Some requests are ongoing, waiting for their completion")
                sleep(30)                      #TODO how much wait time
            resp_header = {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK}
            return HTTPOk(headers = resp_header)
        except Exception as err:
            self.app.logger.exception(err)
            self.app.logger.debug('Exception raised in STOP_SERVICE :%s' %err)
            return HTTPServerError(headers = {'Message-Type' : typeEnums.BLOCK_NEW_REQUESTS_ACK})
