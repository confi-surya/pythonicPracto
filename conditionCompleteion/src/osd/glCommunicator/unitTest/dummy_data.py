from osd.glCommunicator.messages import GlobalMapMessage, \
    GetServiceComponentAckMessage, Status, NodeAdditionCliAckMessage, \
    NodeStatusAckMessage, NodeDeletionCliAckMessage, Status, \
    GetClusterStatusAckMessage

from osd.glCommunicator.service_info import ServiceInfo

service_data = {"account" : 1111, "container" : 2222, "object": 3333, "accountUpdater": 4444}

ll_node_id_list = ["HN0101_8888", "HN0102_9999"]

test_gl_map = {}


def get_gl_map_message():
    for service_name, port in service_data.iteritems():
        lst = []
        for i in range(512):
            if i < 256:
                lst.append(ServiceInfo(ll_node_id_list[0]+ service_name + "-server", "10.0.0.44", port))
            else:
                lst.append(ServiceInfo(ll_node_id_list[1]+ service_name + "-server", "10.0.0.45", port))
        test_gl_map[service_name + "-server"] = (lst, 3.0)
    gl_map_obj = GlobalMapMessage(test_gl_map, 5.0)
    return gl_map_obj


def get_comp_list():
    cmp_list = [1, 2, 3, 4, 5, 6]
    status = Status(0, 'Done By Dk')
    return GetServiceComponentAckMessage(cmp_list, status)

def get_trnsfr_copm_list():
    return [(2, ServiceInfo("HN0101_8888_container-server", "10.0.0.44", 2222)),
            (3, ServiceInfo("HN0102_8888_container-server", "10.0.0.45", 2222)),
            (4, ServiceInfo("HN0103_8888_container-server", "10.0.0.46", 2222))]



def get_comp_status_list():
    return [(1, True), (2, True), (3, True), (4, True), (5, True)]

def get_status():
    return True

def get_added_node_message():
    node_ack_list = [(Status(0, "Done By Dk"), ServiceInfo("HN0101_8888", "10.0.0.44", 2222)), (Status(0, "Done By Dk"), ServiceInfo("HN0101_8888", "10.0.0.44", 2222))]
    return NodeAdditionCliAckMessage(node_ack_list)

def get_node_status():
    return NodeStatusAckMessage(10)

def get_node_deletion_ack(node_info):
    return NodeDeletionCliAckMessage(node_info.get_id(), Status(17, "Dummy Value"))

def get_cluster_status_ack():
    cluster_status_map = {"HN0101": 10, "HN0102": 20, "HN0103": 30, "HN0104": 10}
    return GetClusterStatusAckMessage(cluster_status_map, Status(0, "Success"))


