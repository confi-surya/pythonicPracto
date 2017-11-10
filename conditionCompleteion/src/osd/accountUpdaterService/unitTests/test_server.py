import logging
import mock
import unittest
import pickle
import time
from threading import Thread
from osd.accountUpdaterService.server import HttpListener, AccountUpdater
from osd.accountUpdaterService.unitTests import conf, logger, fake_http_connect, FakeHttpConnect
from osd.accountUpdaterService.unitTests import FakeGlobalVariables, FakeTransferCompMessage, FakeTransferCompResponseMessage, mockReq, mockResp, FakeRingFileReadHandler, mockServiceInfo, mockResult, mockStatus

class FakeWalker(Thread):
    def __init__(self, walker_map, conf, logger):
        Thread.__init__(self)
    def run(self):
        print "walker started"

class FakeReader(Thread):
    def __init__(self, walker_map, reader_map, conf, logger):
        Thread.__init__(self)
    def run(self):
        print "reader started"

class FakeUpdater(Thread):
    def __init__(self, walker_map, reader_map, conf, logger):
        Thread.__init__(self)
    def run(self):
        print "Updater started"

class FakeAccountSweep(Thread):
    def __init__(self, conf, logger):
        Thread.__init__(self)
    def run(self):
        print "AccountSweep started"

class FakeContainerSweeper(Thread):
    def __init__(self, walker_map, reader_map, conf, logger):
        Thread.__init__(self)
    def run(self):
        print "FakeContainerSweeper started"

class FakehealthMonitoring:
    def __init__(self, ip, acc_updater_port, ll_port, service_id):
        pass

def fake_get_logger_object():
    return logger

def fake_get_conf():
    return conf

class TestAccountUpdater(unittest.TestCase):
    @mock.patch('osd.accountUpdaterService.server.healthMonitoring', FakehealthMonitoring)
    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables)
    @mock.patch('osd.accountUpdaterService.server.SimpleLogger.get_logger_object', fake_get_logger_object)
    def setUp(self):
        self.account_updater_obj = AccountUpdater(conf, logger)

    @mock.patch('osd.accountUpdaterService.server.Walker', FakeWalker)
    @mock.patch('osd.accountUpdaterService.server.Reader', FakeReader)
    @mock.patch('osd.accountUpdaterService.server.Updater', FakeUpdater)
    @mock.patch('osd.accountUpdaterService.server.AccountSweep', FakeAccountSweep)
    @mock.patch('osd.accountUpdaterService.server.ContainerSweeper', FakeContainerSweeper)
    def test_run_forever(self):
        pass
        #self.account_updater_obj.run_forever()

class FakeWrite:
    def write(self, data):
        self.data = data

    def written_data(self):
        return self.data

class FakeRead:
    def read(self, content_length):
        self.comp_list = str(range(content_length))
        return self.comp_list

    def read_data(self):
        return self.comp_list 

class FakeBaseHTTPRequestHandler:
    def __init__(self):
        self.wfile = FakeWrite()
        self.rfile = FakeRead()
        self.headers = {'Content-Length':10}

    def send_header(self, key , value):
        self.headers[key] = value
        self.key = key
        self.value = value

    def send_response(self, code, value=None):
        self.code = code

    def end_headers(self):
        pass

    def get_headers(self):
        return self.headers

    def get_response(self):
        return self.code

class TestHttpListener(unittest.TestCase):
    @mock.patch('osd.accountUpdaterService.server.SimpleLogger.get_logger_object', fake_get_logger_object)
    @mock.patch('osd.accountUpdaterService.server.SimpleLogger.get_conf', fake_get_conf)
    @mock.patch('osd.accountUpdaterService.server.BaseHTTPRequestHandler', FakeBaseHTTPRequestHandler)
    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables) 
    def setUp(self):
        HttpListener.__bases__ = (FakeBaseHTTPRequestHandler, )
        self.http_listener_obj = HttpListener()
        self.global_variable_obj = FakeGlobalVariables(logger)

    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables) 
    def test_STOP_SERVICE(self):
        print "STOP_SERVICE"
        self.http_listener_obj.do_STOP_SERVICE()
        self.assertEquals(self.http_listener_obj.get_response(), 200)
        self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 10, 'Ownership-List': range(10, 30), 'Message-Type': 70})
        self.assertEquals(self.http_listener_obj.wfile.written_data(), range(10, 30))

    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables) 
    def test_STOP_SERVICE_exception(self): 
        print "STOP_SERVICE_exception"
        with mock.patch('osd.accountUpdaterService.server.HttpListener.send_header', side_effect=Exception):
	    self.http_listener_obj.do_STOP_SERVICE()
            self.assertEquals(self.http_listener_obj.get_response(), 500)

    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables) 
    def test_ACCEPT_COMPONENT_TRANSFER(self):
        print "ACCEPT_COMPONENT_TRANSFER"
        self.http_listener_obj.do_ACCEPT_COMPONENT_TRANSFER()
        self.assertEquals(self.http_listener_obj.get_response(), 200)
        self.assertEquals(self.http_listener_obj.rfile.read_data(), str(range(10))) 
    
    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables)
    def test_ACCEPT_COMPONENT_TRANSFER_exception(self):
        print "ACCEPT_COMPONENT_TRANSFER_exception"
        with mock.patch('osd.accountUpdaterService.server.ast.literal_eval', side_effect=Exception):
            self.http_listener_obj.do_ACCEPT_COMPONENT_TRANSFER()
            self.assertEquals(self.http_listener_obj.get_response(), 500)
   
    @mock.patch("osd.accountUpdaterService.server.Req", mockReq)
    @mock.patch("osd.accountUpdaterService.server.Resp", mockResp)
    @mock.patch("osd.accountUpdaterService.server.ServiceInfo", mockServiceInfo)
    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables)
    @mock.patch('osd.accountUpdaterService.server.TransferCompMessage', FakeTransferCompMessage)
    @mock.patch('osd.accountUpdaterService.server.TransferCompResponseMessage', FakeTransferCompResponseMessage)
    @mock.patch('osd.accountUpdaterService.server.http_connect', fake_http_connect)
    def test_TRANSFER_COMPONENTS(self):
        print "TRANSFER_COMPONENTS"
        self.http_listener_obj.do_TRANSFER_COMPONENTS()
        self.assertEquals(self.http_listener_obj.get_response(), 200)
        self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 2, 'Message-Type': 18})
        self.assertEquals(self.http_listener_obj.wfile.written_data(), ([(0, True), (1, True), (2, True), (3, True), (4, True), (5, True), (6, True), (7, True), (8, True), (9, True)], True))

    @mock.patch("osd.accountUpdaterService.server.Req", mockReq)
    @mock.patch("osd.accountUpdaterService.server.Resp", mockResp)
    @mock.patch("osd.accountUpdaterService.server.ServiceInfo", mockServiceInfo)
    @mock.patch('osd.accountUpdaterService.server.GlobalVariables', FakeGlobalVariables)
    @mock.patch('osd.accountUpdaterService.server.TransferCompMessage', FakeTransferCompMessage)
    @mock.patch('osd.accountUpdaterService.server.TransferCompResponseMessage', FakeTransferCompResponseMessage)
    @mock.patch('osd.accountUpdaterService.server.http_connect', fake_http_connect)
    def test_TRANSFER_COMPONENTS_exceptions(self):
        print "TRANSFER_COMPONENTS_exceptions"
        #Exception during de-serializing component map from GL
        with mock.patch('osd.accountUpdaterService.server.TransferCompMessage.de_serialize', side_effect=Exception):
            self.http_listener_obj.do_TRANSFER_COMPONENTS()
            self.assertEquals(self.http_listener_obj.get_response(), 500)
            self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 10, 'Message-Type': 18})

        #Exception during serializing final response
        with mock.patch('osd.accountUpdaterService.server.TransferCompResponseMessage.serialize', side_effect=Exception):
            self.http_listener_obj.do_TRANSFER_COMPONENTS()
            self.assertEquals(self.http_listener_obj.get_response(), 500)
            self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 10, 'Message-Type': 18})

        #Connection creation exception
        with mock.patch('osd.accountUpdaterService.server.http_connect', side_effect=Exception):
            self.http_listener_obj.do_TRANSFER_COMPONENTS()
            self.assertEquals(self.http_listener_obj.get_response(), 200)
            self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 2, 'Message-Type': 18})
            self.assertEquals(self.http_listener_obj.wfile.written_data(), ([(0, False), (1, False), (2, False), (3, False), (4, False), (5, False), (6, False), (7, False), (8, False), (9, False)], False))

        #HTTP Continue during connection creation
        with mock.patch('osd.accountUpdaterService.server.http_connect', return_value=FakeHttpConnect(100)):
            self.http_listener_obj.do_TRANSFER_COMPONENTS()
            self.assertEquals(self.http_listener_obj.get_response(), 200)
            self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 2, 'Message-Type': 18})
            self.assertEquals(self.http_listener_obj.wfile.written_data(), ([(0, False), (1, False), (2, False), (3, False), (4, False), (5, False), (6, False), (7, False), (8, False), (9, False)], False))

        #HTTP_INSUFFICIENT_STORAGE exception during connection creation
        with mock.patch('osd.accountUpdaterService.server.http_connect', return_value=FakeHttpConnect(507)):
            self.http_listener_obj.do_TRANSFER_COMPONENTS()
            self.assertEquals(self.http_listener_obj.get_response(), 200)
            self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 2, 'Message-Type': 18})
            self.assertEquals(self.http_listener_obj.wfile.written_data(), ([(0, False), (1, False), (2, False), (3, False), (4, False), (5, False), (6, False), (7, False), (8, False), (9, False)], False))

        #Data send failed after connection created
        with mock.patch('osd.accountUpdaterService.unitTests.FakeHttpConnect.send', side_effect=Exception):
            self.http_listener_obj.do_TRANSFER_COMPONENTS()
            self.assertEquals(self.http_listener_obj.get_response(), 200)
            self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 2, 'Message-Type': 18})
            self.assertEquals(self.http_listener_obj.wfile.written_data(), ([(0, False), (1, False), (2, False), (3, False), (4, False), (5, False), (6, False), (7, False), (8, False), (9, False)], False))

        #comp_transfer_info intermediate response failed
        with mock.patch('osd.accountUpdaterService.server.Req.comp_transfer_info', return_value=mockResult(mockStatus(mockResp.FAILURE, "Failure"), 'Failure')):
            self.http_listener_obj.do_TRANSFER_COMPONENTS()
            self.assertEquals(self.http_listener_obj.get_response(), 200)
            self.assertEquals(self.http_listener_obj.get_headers(), {'Content-Length': 2, 'Message-Type': 18})
            self.assertEquals(self.http_listener_obj.wfile.written_data(), ([(0, False), (1, False), (2, False), (3, False), (4, False), (5, False), (6, False), (7, False), (8, False), (9, False)], False))

if __name__ == '__main__':
    unittest.main()
