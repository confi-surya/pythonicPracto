import time
import sys

from osd.glCommunicator.response import *
from osd.glCommunicator.req import Req 
from osd.glCommunicator.service_info import ServiceInfo
from osd.glCommunicator.communication.communicator import IoType
from osd.glCommunicator.unitTest.dummy_data import *

def test_get_comp_list(my_id, gl_id, gl_ip, gl_port):
    request = Req()
    node = ServiceInfo(gl_id, gl_ip, gl_port)
    con = request.connector(IoType.EVENTIO, node)
    resp = request.get_comp_list(my_id, con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
    else:
        print "Failed"
    con.close()

def test_get_gl_map(my_id, gl_id, gl_ip, gl_port):
    request = Req()
    node = ServiceInfo(gl_id, gl_ip, gl_port)
    con = request.connector(IoType.EVENTIO, node)
    resp = request.get_global_map(my_id, con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
        #print resp.message.map()
        #print resp.message.version()
    else:
        print "Failed"
    con.close()

def comp_transfer_info(my_id, gl_id, gl_ip, gl_port):
    request = Req()
    node = ServiceInfo(gl_id, gl_ip, gl_port)
    con = request.connector(IoType.EVENTIO, node)
    resp = request.comp_transfer_info(my_id, get_trnsfr_copm_list(), con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
        #print resp.message.map()
        #print resp.message.version()
    else:
        print "Failed"
    con.close()

def comp_transfer_final_stat(my_id, gl_id, gl_ip, gl_port):
    request = Req()
    node = ServiceInfo(gl_id, gl_ip, gl_port)
    con = request.connector(IoType.EVENTIO, node)
    resp = request.comp_transfer_final_stat(my_id, get_comp_status_list(), get_status(), con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
        #print resp.message.map()
        #print resp.message.version()
    else:
        print "Failed"
    con.close()

def test_node_addition(my_id, gl_id, gl_ip, gl_port):
    request = Req()
    node = ServiceInfo(gl_id, gl_ip, gl_port)
    con = request.connector(IoType.EVENTIO, node)
    nodes = [ServiceInfo("", "10.0.0.45", 2222), ServiceInfo("", "10.0.0.45", 2222), ServiceInfo("", "10.0.0.45", 2222)]
    resp = request.node_addition(nodes, con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
        #print resp.message.node_ack_list()
    else:
        print "Failed"
    con.close()

def test_node_stop(node_id, ip, port):
    request = Req()
    node = ServiceInfo(node_id, ip, port)
    con = request.connector(IoType.EVENTIO, node)
    resp = request.node_stop(node_id, con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
    else:
        print "Failed"
    con.close()

def test_local_node_status(dest_id, dest_ip, dest_port):
    request = Req()
    node = ServiceInfo(dest_id, dest_ip, dest_port)
    con = request.connector(IoType.EVENTIO, node)
    resp = request.local_node_status(dest_id, con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
        #print "status_code: %s, Msg: %s, enum: %s" % (resp.status.get_status_code(), resp.status.get_status_message(), resp.message.get_node_status())
    else:
        print "Failed: status_code: %s, msg: %s" %(resp.status.get_status_code(), resp.status.get_status_message())
    con.close()

def test_node_deletion(node_id, ip, port):
    request = Req()
    node = ServiceInfo(node_id, ip, port)
    dest = ServiceInfo("Rajnikant", "192.168.123.13", 87654)
    con = request.connector(IoType.SOCKETIO, node)
    resp = request.node_deletion(dest, con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
    else:
        print "Failed"
    con.close()

def test_get_osd_status(node_id, ip, port):
    request = Req()
    node = ServiceInfo(node_id, ip, port)
    service_id = "cli"
    con = request.connector(IoType.SOCKETIO, node)
    resp = request.get_osd_status(service_id, con)
    if resp.status.get_status_code() == Resp.SUCCESS:
        print "Success"
        #mp =  resp.message
        #print mp
        #for i in mp:
        #    print i, mp[i] 
    else:
        print "Failed"
        print resp.status.get_status_code()
        print resp.status.get_status_message()
    con.close()



print "############################Get Component List Testing#######################################"
test_get_comp_list(my_id="HN0101_1234_container-server",
                       gl_id="Rajnikant",
                       gl_ip="192.168.123.13",
                       gl_port=87654)

print "############################Component Transfer Info##########################################"
comp_transfer_info(my_id="HN0101_1234_container-server",
                     gl_id="Rajnikant",
                     gl_ip="192.168.123.13",
                     gl_port=87654)

print "############################Component Transfer final stat####################################"
comp_transfer_final_stat(my_id="HN0101_1234_container-server",
                     gl_id="Rajnikant",
                     gl_ip="192.168.123.13",
                     gl_port=87654)


print "############################Get Global Map Testing###########################################"
test_get_gl_map(my_id="HN0101_1234_container-server",
                gl_id="Rajnikant",
                gl_ip="192.168.123.13",
                gl_port=87654)

print "############################Node Addition Testing############################################"
test_node_addition(my_id="HN0101_1234_container-server",
                       gl_id="Rajnikant",
                       gl_ip="192.168.123.13",
                       gl_port=87654)

print "############################Node Deletion####################################################"
test_node_deletion(node_id="Rajnikant", ip="192.168.123.13", port=87654)

print "############################Local Node Status Testing########################################"
test_local_node_status(dest_id="Rajnikant", dest_ip="192.168.123.13", dest_port=87654)

print "############################## Get Osd cluster status########################################"
test_get_osd_status(node_id="Rajnikant", ip="192.168.123.13", port=87654)

