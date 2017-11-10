"""
File to handle self recovery of container service
"""
import os
import glob
import traceback
import time
import shutil
import socket
import httplib
from eventlet import Timeout
#from osd.common.utils import GreenAsyncPile 
from osd.common.swob import HeaderKeyDict
from osd.common.http import is_success
from osd.common.ring.ring import ObjectRing, ContainerRing, get_service_obj
from osd.containerService.utils import RING_DIR, \
    TMPDIR, get_container_id, MAX_EXP_TIME, \
    get_all_container_id, get_filesystem
from osd.common.bufferedhttp import http_connect
from osd.common.exceptions import ConnectionTimeout

def get_container_details(logger):
    """
    Function to get ip and port of container service
    Returns: ip, port
    """
    ip, port = '', ''
    try:
        cont_service_id = get_container_id()
        cont_ring = ContainerRing(RING_DIR)
        data = cont_ring.get_service_details(cont_service_id)[0]
        ip = data['ip']
        port = data['port']
    except Exception as err:
        logger.exception(err)
    return ip, port

def conn_for_obj(inner_dict, conn_time, node_timeout, logger, object_map):
    """
    Make connection to object service for expired IDs
    Returns: None
    """
    try:
        service_id = inner_dict['x-server-id']
        full_path = inner_dict['x-object-path']
        # get ip, port of object service
        logger.debug("object_map: %s" % object_map)
        data = {}
        data['ip'] = object_map[service_id][0]
        data['port'] = object_map[service_id][1]
        # update dict for container service id
        #TODO:(jaivish):just look whether this needs to passed as default parameter, 
        #else no need to  change
        cont_service_id = get_container_id()
        inner_dict['x-server-id'] = cont_service_id
        #TODO
        # make connection
        with ConnectionTimeout(conn_time):
            logger.debug('Connection to object service started')
            conn = http_connect(data['ip'], data['port'], '/', '/', \
                'RECOVER', full_path, inner_dict)
            logger.debug('Connection to object service completed: path:%s , dict: %s' % \
                         (full_path, inner_dict))
        with Timeout(node_timeout):
            response = conn.getresponse()
            response.read()
            if not is_success(response.status):
                logger.error((
                'ERROR connection to object service failed for '
                'service-id: %(service_id)s'
                'HTTP response status: %(status)d '),
                {'service_id': service_id,
                'status': response.status})
    except (Exception, Timeout) as err:
        logger.error('Exception occurred in sending'
            'request to object service')
        logger.exception(err)


def recover_conflict_expired_id(req, trans_list, conn_time, node_timeout, logger, object_map, service_id, global_map_version):
    """
    Method to recover conflicted lock request 
    Returns: None
    """
    try:
        #(Async)pile = GreenAsyncPile(100) # for Async changes
        logger.info("free lock conflict called with params:%s -- %s -- %s -- %s -- %s" \
                    % (req.headers, trans_list, object_map.keys(), service_id, global_map_version))
        for item in trans_list:
            obj_serv_id, full_path, operation = '', '', ''
            inner_dict = HeaderKeyDict({})
            try:
                obj_serv_id = item.get_object_id()
                trans_id = item.get_transaction_id()
                full_path = item.get_object_name()
                operation = item.get_request_method()
            except:
                logger.error('recover_conflict_expired_id Request doesnot contain'
                             ' the correct headers: %s %s %s' % \
                             (req.headers, trans_id, obj_serv_id))    	
            if obj_serv_id not in object_map.keys():
                tmp = service_id.split("_")
                obj_serv_id = tmp[0] + "_" + tmp[1] + "_" + "object-server"
            inner_dict.update({'x-server-id' : obj_serv_id, 
                'x-object-path' : full_path, 'x-operation' : operation,
                'x-request-trans-id' : trans_id, 'x-global-map-version' : global_map_version})
            logger.info(('conflict object recovery called for service_id:- '
                '%(id)s, object_name: %(name)s, operation: %(op)s'
                ' and transaction Id:- %(trans)s'),
                {'id' : obj_serv_id, 'name' : full_path, 
                'trans' : trans_id, 'op' : operation})
            # call object recover function
            conn_for_obj(inner_dict, conn_time, node_timeout, logger, object_map)
            #(Async)pile.spawn(conn_for_obj(inner_dict, conn_time, node_timeout, logger, object_map))
        #(Async)counter = 0 
        #(Async)for response_item in pile:
        #(Async)    counter += 1
        #(Async)    logger.debug("Async resp %s returned " % counter)
    except (Exception, Timeout) as err:
        logger.error((
            'ERROR Could not complete recovery process for '
            'object server'
            'close failure: %(exc)s : %(stack)s'),
            {'exc': err, 'stack': ''.join(traceback.format_stack())})
    logger.debug('Exit: As all IDs are processed')


def recover_object_expired_id(trans_list, conn_time, node_timeout, logger, object_map, service_id, global_map_version):
    """
    Method to recover list of object transaction IDs
    which are expired or timed out
    Returns: None
    """
    try:
        for item in trans_list:
            final_data = item.split(',')
            record_time = final_data[4].split('####')[0]
            time_now = time.time()
            if (time_now - float(record_time)) < MAX_EXP_TIME:
                continue
            trans_id = final_data[0]
            full_path = final_data[1]
            obj_serv_id = final_data[2]
            operation = final_data[3]
            if (operation == 'BULK_DELETE'):
                continue
            inner_dict = HeaderKeyDict({})
            if obj_serv_id not in object_map.keys():
                tmp = service_id.split("_")
                obj_serv_id = tmp[0] + "_" + tmp[1] + "_" + "object-server"
            inner_dict.update({'x-server-id' : obj_serv_id, 
                'x-object-path' : full_path, 'x-operation' : operation,
                'x-request-trans-id' : trans_id, 'x-global-map-version' : global_map_version})
            logger.debug(('object recovery called for service_id:- '
                '%(id)s, object_name: %(name)s, operation: %(op)s'
                ' and transaction Id:- %(trans)s'),
                {'id' : obj_serv_id, 'name' : full_path, 
                'trans' : trans_id, 'op' : operation})
            # call object recover function
            conn_for_obj(inner_dict, conn_time, node_timeout, logger, object_map)
    except (Exception, Timeout) as err:
        logger.error((
            'ERROR Could not complete recovery process for '
            'object server'
            'close failure: %(exc)s : %(stack)s'),
            {'exc': err, 'stack': ''.join(traceback.format_stack())})
    logger.debug('Exit: As all IDs are processed')

def rename_tmp_dir(tmp_dir, current_node, logger):
    """
    Rename the old tmp directory to new tmp dir
    according to current node
    """
    try:
        cont_dirs = '%s/%s' % (tmp_dir, '*container-server')
        tmp_list = glob.glob(cont_dirs)
        for name in tmp_list:
            name = name.split('tmp/')[1]
            node = name.split('_')[0]
            if current_node != node:
                old_dir = '%s/%s_%s' % (tmp_dir, node, 'container-server')
                new_dir = '%s/%s_%s' % (tmp_dir, current_node, 'container-server')
                if os.path.exists(new_dir):
                    old_tmpdir = '%s/%s' % (old_dir, 'temp')
                    new_tmpdir = '%s/%s' % (new_dir, 'temp')
                    for root, dirs, files in os.walk(old_tmpdir):
                        for infoFile in files:
                            move_file = os.path.join(root, infoFile)
                            shutil.move(move_file, new_tmpdir)
                    shutil.rmtree(old_dir)
                    logger.debug('infoFiles copied to new tmp dir')
                else:
                    os.rename(old_dir, new_dir)
                    logger.debug('Rename successful!')
    except Exception as err:
        raise err

def container_recovery(fs_base, logger, component_list):
    """
    Search file in container tmp directory and delete the files
    Returns: None
    """
    try:
        fs_name = get_filesystem(logger)
        tmp_dir = os.path.join(fs_base, fs_name[0], TMPDIR)
        current_service_id = get_container_id()
        current_node = current_service_id.split('_')[0]
        #rename_tmp_dir(tmp_dir, current_node, logger)
        #service_id = '%s_%s_%s' % (current_node, ll_port, 'container-server')
        service_id = 'container'
        for comp in component_list:
            new_dir = os.path.join(fs_base, fs_name[0], TMPDIR, service_id, str(comp))
            logger.debug(('Processing temp dir:- %(tmp)s'), {'tmp' : new_dir})
            list_files = '%s/%s' % (new_dir, '*.infoFile')
            files = glob.glob(list_files)
            for tmp_file in files:
                if os.path.exists(tmp_file):
                    # Delete the file
                    os.remove(tmp_file)
                    logger.debug(('Tempory file deleted:- %(tmp_file)s'),
                    {'tmp_file' : tmp_file})
        return True
    except Exception as err:
        logger.error((
            'ERROR Could not complete startup recovery process '
            ' close failure: %(exc)s : %(stack)s'),
            {'exc': err, 'stack': ''.join(traceback.format_stack())})
        return False

def process_bulk_delete_transctions(obj_list, conn_time, node_timeout, obj_version, ll_port, logger):
    """
    Method to call BULK_DELETE interface of object service
    """
    try:
        service_id = socket.gethostname() + '_' + str(ll_port) +'_object-server'
        #obj_ring = ObjectRing(RING_DIR, logger)
        #data = obj_ring.get_service_details(service_id)[0]
        service_obj = get_service_obj('object', logger)
        cont_service_id = get_container_id()
        headers = {'x-server-id':cont_service_id, \
        'X-GLOBAL-MAP-VERSION': obj_version}
        with ConnectionTimeout(conn_time):
            logger.debug('Connection to object service started')
            # add the obj_list in the baody of request
            conn = httplib.HTTPConnection(service_obj.get_ip(), \
            service_obj.get_port())
            if conn:
                conn.request("BULK_DELETE", '', str(obj_list),headers)
            else:
                logger.debug("Can not create connection to object server ")
        logger.debug('Connection to object service completed')
        with Timeout(node_timeout):
            response = conn.getresponse()
            response.read()
            if not is_success(response.status):
                logger.error((
                'ERROR connection to object service failed for '
                'service-id: %(service_id)s'
                'HTTP response status: %(status)d '),
                {'service_id': service_id,
                'status': response.status})
    except (Exception, Timeout) as err:
        logger.error('Exception occurred in sending'
            'request to object service')
        logger.exception(err)

