"""Tests for health monitoring library"""
import sys
sys.path.append("../../../../bin/release_prod_64")
sys.path.append("../../../../src")
import os
import unittest
import mock
from libHealthMonitoringLib import healthMonitoring
from ll_controller import fake_ll
import time

class TestHealthMonitoringController(unittest.TestCase):

    def setUp(self):
        self.__ll_instance = fake_ll()
        self.__ll_instance.start()
	print "Setup Called!"

    def tearDown(self):
        self.__ll_instance.stop()
	self.__ll_instance.join()
	print "Tear down called!"

    def test_send_strm_hrbt(self):
        ip = "127.0.0.1"
        port = 61007
        ll_port = 11
        service_id = "HN0101_61011_container-service"
        print "Calling health monitoring!!!!"
        health = healthMonitoring(ip, port, ll_port, service_id)
        time.sleep(120)
        self.assertEqual(health, False)
        self.__ll_instance.stop()
        self.__ll_instance.join()

    def test_send_strm_hrbt_proxy(self):
        ip = "127.0.0.1"
        port = 61007
        ll_port = 11
        service_id = "HN0101_61011_container-service"
        print "Calling health monitoring!!!!"
        health = healthMonitoring(ip, port, ll_port, service_id, True)
        time.sleep(120)

    def test_ll_not_available(self):
        ip = "127.0.0.1"
        port = 61007
        ll_port = 123
        service_id = "HN0101_61011_container-service"
        print "Calling health monitoring!!!!"
        health = healthMonitoring(ip, port, ll_port, service_id)
        time.sleep(120)

if __name__ == '__main__':
    unittest.main()
