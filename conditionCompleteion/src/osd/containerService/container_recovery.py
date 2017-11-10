'''Container Recovery'''
#!/usr/bin/python
import procname
import os
import sys
import pickle
import time
from hashlib import md5

import argparse
import ConfigParser
import eventlet
from eventlet.queue import Queue as queue
from eventlet import GreenPile, GreenPool
from eventlet.timeout import Timeout

import libraryUtils
import libContainerLib
import libTransactionLib
from libContainerLib import LibraryImpl


from osd.glCommunicator.req import Req
from osd.glCommunicator.communication.communicator import IoType
from osd.glCommunicator.response import Resp

from osd.common.utils import ismount
import osd_utils
from osd import gettext_ as _
from osd.common.utils import get_logger, get_global_map_version
from osd.common.constraints import check_mount
from osd.common.swob import HTTPNotFound
from osd.common.utils import ContextPool , GreenthreadSafeIterator, \
    GreenAsyncPile
from osd.common.bufferedhttp import http_connect
from osd.common.exceptions import ChunkReadTimeout, \
    ChunkWriteTimeout, ConnectionTimeout
from osd.common.http import is_success, HTTP_CONTINUE, \
    HTTP_INTERNAL_SERVER_ERROR, HTTP_SERVICE_UNAVAILABLE, \
    HTTP_INSUFFICIENT_STORAGE

EXPORT = "/export/"

class NodeList(Exception):
    def __init__(self, msg, *args):
        self.msg = msg

class RecoveryMgr:
    def __init__(self, conf, global_map_object, service_id,
            communicator_object, Request_handler, logger=None, source_service_obj=None):

        self.logger = logger or get_logger(conf,
                        log_route='container-recovery')
        libraryUtils.OSDLoggerImpl("container-library").initialize_logger()
        self.logger.info("running RecoveryMgr ")
        self.communicator_object = communicator_object
        self.Request_handler = Request_handler
        self.conf = conf
        self.__trans_port = int(conf['child_port'])
        self.global_map_object = global_map_object
        self.source_service_obj = source_service_obj
        self.service_id = service_id
        self.id = self.service_id.split('_')[0]
        self.local_leader_id = self.service_id.split('_')[1]
 
        self.__cont_journal_path = ''.join('.' + self.id +"_" +self.local_leader_id \
                                    + "_container_journal")
        self.__trans_journal_path = ''.join('.' + self.id + "_" +self.local_leader_id \
                                    + "_transaction_journal")
        self.__cont_path = EXPORT + self.__cont_journal_path
        self.__trans_path = EXPORT + self.__trans_journal_path
        self.logger.debug("Initiaization entries: %s %s %s %s %s %s " % \
            (self.global_map_object, \
             self.service_id, self.__cont_journal_path, \
             self.__trans_journal_path, self.__cont_path, self.__trans_path))

        #get object gl version from global leader
        self._object_gl_version = self.__get_object_gl_version()
        self.logger.debug("Object version received: %s"% self._object_gl_version)

        #Map from gl
        self.map_gl = self.global_map_object.dest_comp_map()
        self.logger.debug("Map from Gl Received %s" % self.map_gl)

        #start heart beat
        self.logger.debug("Made heart beat thread")
        #self.heart_beat_thread.spawn(self.start_heart_beat, self.Request_handler)
        self.heart_beat_gthread = eventlet.greenthread.spawn(self.start_heart_beat, self.Request_handler)
        self.logger.debug("Started heart beat thread func")

    #Changed:7_oct
    def __create_con_obj(self):
        gl_info_obj = self.Request_handler.get_gl_info()
        return self.Request_handler.connector(IoType.EVENTIO, gl_info_obj)

    def clean_journals(self):
        #mount filesystem
        ret_cont = 0
        ret_trans = 0
        try:
           #check journal mount and the its existence before cleaning.
            if ismount(self.__cont_path):
                if os.path.exists(self.__cont_path):
                    self.logger.info("cleaning container journal")
                    self.clean_container_journals()
                    #ret_cont = os.system('rm -rf %s/*' % self.__cont_path)
                self.logger.info("journal: %s" % self.__cont_path)
            else:
                self.logger.info("Path not mounted: %s" % self.__cont_path)
            if ismount(self.__trans_path):
                if os.path.exists(self.__trans_path):
                    self.logger.info("cleaning transaction journal")
                    ret_trans = os.system('rm -rf %s/*' % self.__trans_path)
                self.logger.info("journal: %s"% self.__trans_path)
            else:
                self.logger.info("Path not mounted: %s" % self.__trans_path)
            self.logger.info("Exit status: cont->%s, trans->%s"% \
                (ret_cont, ret_trans))
        except Exception as err:
            self.logger.error("Exception in Journal cleanup %s"% err)

    def clean_container_journals(self):
        """
        Function to clean container journal file and create a blank file
        """
        journal_files = os.listdir(self.__cont_path)
        tmp_id = 0
        #Finding the latest journal id
        for file in journal_files:
            try:
                if tmp_id < int(file.split('.')[2]):
                    tmp_id = int(file.split('.')[2])
            except Exception as ex:
                self.logger.error("un wanted file found : %s" %file)

        tmp_id += 1
        self.logger.info("New journal file ID in case of container recovery %s" %tmp_id)
        #Creating a new journal file with an increamental id
        new_journal_file = self.__cont_path + "/" + "journal.log." + str(tmp_id)
        os.system('touch %s' % new_journal_file)

        #removing old journals
        for file in journal_files:
            os.system('rm -f %s' %(self.__cont_path + "/" + file))

    def __get_object_gl_version(self):
        '''
        get major object version from GL
        '''
        retry_count = 3
        counter = 0
        con_obj = self.__create_con_obj()
        #con_obj = self.communicator_object
        while counter < retry_count:
            ret =  self.Request_handler.get_object_version(self.service_id, con_obj)
            if ret.status.get_status_code() in [Resp.SUCCESS]:
                self.logger.debug("major object version value from \
                    GL %s" % ret.message.get_version())
                return ret.message.get_version()
            else:
                self.logger.info("get_object_version retry..")
                eventlet.sleep(1)
            counter += 1
        return None


    def start_heart_beat(self, comm_obj):
        """start heart beat """
        global finish_recovery_flag
        global list_of_tuple
        self.logger.info("started sending heartbeat")
        self.logger.debug("started sending heartbeat")
        sequence_counter = 1
        #start heart beat
        while not finish_recovery_flag:
            self.logger.info("Queue--length : %s" % list_of_tuple.qsize())
            value = None
            #value = list_of_tuple.get()
            self.logger.info("Finished flag False value in queue: %s"% value)
            if not list_of_tuple.empty():
                self.logger.info("Getting value from Queue")
                value = list_of_tuple.get()
                self.logger.debug("Value retrieved from Queue: %s"% value)

            if value:
                self.logger.info("Sending data to gl-----")
                self.__send_transfer_status(comm_obj, value)
                #eventlet.sleep(5)
            else:
                self.logger.info("Heart beat seqence %s" % sequence_counter)
                ret = comm_obj.send_heartbeat(self.service_id, \
                                sequence_counter, self.communicator_object)
                self.logger.debug("Sending heart beat")
                eventlet.sleep(10)
                if ret.status.get_status_code() in [Resp.BROKEN_PIPE, \
                        Resp.CONNECTION_RESET_BY_PEER, Resp.INVALID_SOCKET]:
                    continue
                sequence_counter += 1

        #required
        if finish_recovery_flag:
            self.logger.info("Finish_recovery_flag Set")
     
        self.logger.info("Data to gl sending complete, sending end_monitoring")

        self.__end_monitoring(comm_obj)
        finish_recovery_flag = False 
        self.logger.info("End_monitoring sent")


    def __end_monitoring(self, comm_obj):
        self.logger.info("parameters sent %s %s %s %s" % (self.service_id, \
                final_recovery_status_list, final_status, \
                comm_obj))
        ret = comm_obj.comp_transfer_final_stat(self.service_id, \
                final_recovery_status_list, \
                final_status, \
                self.communicator_object)
        self.logger.info("end_monitoring status :%s" % ret)

    def __send_transfer_status(self, comm_obj, val):
        #self._recovery_data_obj.get_transfered_comp_list() -> to be defined.
        retry_count = 3
        counter = 0
        complete = False
        self.logger.info("in send transfer status")
        while counter < retry_count:
            #ret = comm_obj.comp_transfer_info(self.source_service_obj, \
            ret = comm_obj.comp_transfer_info(self.service_id, \
                                        val, \
                                        self.communicator_object, \
                                        local_timeout=300)

            self.logger.info("returned status after transfer--------")
            #list_of_tuple.task_done()
            if ret.status.get_status_code() in [Resp.SUCCESS]:
                self.logger.info("container server recovery server_id : %s \
                    Component transfer Completed" % self.service_id)
                complete = True
                break
            elif ret.status.get_status_code() in [Resp.BROKEN_PIPE,
                    Resp.CONNECTION_RESET_BY_PEER, Resp.INVALID_SOCKET]:
                self.logger.info("container server recovery server_id : %s, \
                    Connection resetting" % self.service_id)
            else:
                self.logger.info("container server recovery server_id : %s \
                    Component transfer Failed, Retrying... " % self.service_id)
            counter += 1

        if complete:
            self.logger.info("send_transfer_complete")
        else:
            self.logger.info("Intermediate GL Writing failed..EXITING")
            sys.exit(130)


    def _get_map_restructure(self):
        """Get Map for component and location"""
        Map_from_gl = self.map_gl
        self.logger.info("Dest comp map from GL: %s" % Map_from_gl)
        service_location_map = {}
        for key in Map_from_gl.keys():
            service_location_map[key.get_ip()]=str(key.get_port())
        self.logger.info("1....")

        service_component_map  = {}
        for key in Map_from_gl.keys():
            ip_port = "%s:%s"% (key.get_ip(), str(key.get_port()))
            service_component_map[ip_port] = Map_from_gl[key]

        self.logger.debug("service_location_map: %s, service_component_map \
                        %s"% (service_location_map, service_component_map))
        return service_location_map, service_component_map


    def start_recovery(self):
        """Recovery process start"""
        #check Mount status
        global finish_recovery_flag
        ret = self._journal_mount()
        if ret:
            self.logger.info("Mount Successfull")
        else:
            sys.exit(130)

        #extract Dictionary
        try:
            location_map, component_map = self._get_map_restructure()
            self.logger.info("Extracted dictionary from Gl object")
        except Exception as err:
            self.logger.info("Exception raised: %s" % err)

        #extract Data from container and transaction Journal
        container_entries = self.read_container_journal(component_map)
        transaction_entries = self.read_transaction_journal(component_map)

        #Make new dictionary and send to recovery Process
        transaction_dictionary = self.restructure_entries(component_map, \
                                transaction_entries)

        container_dictionary = self.restructure_entries(component_map, \
                                container_entries)

        self.logger.info("Transaction_dictionary: %s, \
                         Container_dictionary: %s" % \
                         (transaction_dictionary, container_dictionary))

        eventlet.sleep(3)
        #context-switch


        #Send to Recovery Process
        method = "ACCEPT_COMPONENT_CONT_DATA"
        self.logger.info("Constructing Recovery class")
        try:
            Recover = Recovery(self.conf, container_dictionary, \
                           self.communicator_object, \
                           component_map, self.service_id, \
                           self.Request_handler, \
                           self.logger, self._object_gl_version, self.map_gl)
        except Exception as err:
            self.logger.info("Error while constructing Recovery class: %s" % err)

        #Store status, reason, body in list.
        self.logger.info("Recovery class object created")

        eventlet.sleep(3)
        #context-switch


        try:
            status, reason, body, component_list = Recover.recovery_container(method, False)
        except NodeList as err:
            self.__end_monitoring(self.Request_handler)
            self.logger.info("Error raised : %s" % err)
            sys.exit(130)

   
        self.logger.info("First recovery is completed")
        transaction_dict = {}
        for i in component_list:
            transaction_dict[i] = transaction_dictionary[i]

        method = "ACCEPT_COMPONENT_TRANS_DATA"
        self.logger.info("Constructing trans recovery %s"% transaction_dict)
        Recover = Recovery(self.conf, transaction_dict, \
                           self.communicator_object, \
                           component_map, self.service_id, \
                           self.Request_handler, \
                           self.logger, self._object_gl_version, self.map_gl)

        try:

            status, reason, body, component_list = \
                Recover.recovery_container(method, True)
            self.logger.info("Second recovery is completed")

        except NodeList as err:
            self.__end_monitoring(self.Request_handler)
            self.logger.info("Error raised : %s" % err)
            sys.exit(130)
            

        while not list_of_tuple.empty():
            self.logger.info("trying  to make list_of_tuple == empty")
            eventlet.sleep(2)
            
        finish_recovery_flag = True
        self.logger.info("marking finish_recovery_flag = True")

        #Join Heart beat Thread
        self.heart_beat_gthread.wait()
        self.logger.info("Container server id : %s Recovery exiting" % \
                        self.service_id)


    def read_container_journal(self, component_map):
        '''
        :param container_map
        return container_entries
        (dictionary=[{component_no:[list_of_records],...}])
        >>>{'x1':[1,2,3],'x2':[4,5,6]}
        '''
        try:
            self.logger.info("reading_container journal %s %s"% \
                    (component_map, self.__cont_journal_path))

            node_hash = md5(self.id + "_" + self.local_leader_id)
            node_id = int(node_hash.hexdigest()[:8], 16) % 256

            #Performance
            node_id = -1

            self.logger.info("Node ID for this set of services is %r %r"% \
                                (node_id, self.__cont_journal_path))

            lib_impl = LibraryImpl(self.__cont_path, \
                                    node_id)
            comp_entries = libContainerLib.list_less__component_names__greater_()
            for i in component_map.values():
                for comp in i:
                    comp_entries.append(comp)
            self.logger.info("component : %s"% comp_entries)
            container_entries = lib_impl.extract_cont_journal_data(comp_entries)
            self.logger.info(_("Container Entries: %(cont_entries)s"), \
                {'cont_entries': container_entries})
            return container_entries
        except Exception as err:
            self.logger.error("Reading container Journal Failed: %s"% err)
            sys.exit(130)

    def read_transaction_journal(self, component_map):
        '''
        :param transaction_journal_file system-
        return transaction_entries(dictionary:{'comp':[data]})
        '''
        try:
            self.logger.info("reading transaction_journal")
            compo_entries = \
                libContainerLib.list_less__component_names__greater_()
            for i in component_map.values():
                for comp in i:
                    compo_entries.append(comp)

            node_hash = md5(self.id + "_" + self.local_leader_id)
            node_id = int(node_hash.hexdigest()[:8], 16) % 256

            #Performance
            node_id = -1
            self.logger.info("Node ID for this set of services is %r"% \
                                node_id)
            transObj = \
                libTransactionLib.TransactionLibrary(self.__trans_path,
                        node_id, self.__trans_port)

            #read data from journal
            self.logger.info("component : %s"% compo_entries)
            transaction_entries = transObj.extract_trans_journal_data(compo_entries)
            self.logger.info(_("Transaction Entries: %(transaction_entries)s"),
                {'transaction_entries': transaction_entries})
            return transaction_entries
        except Exception as err:
            self.logger.error("Reading transaction Journal Failed: %s"% err)
            sys.exit(130)




    def restructure_entries(self, component_map, container_entries):
        '''
        >>> dictionary_from_gl =
        >>> {'IP:PORT':['x1','x3'],'IP1:PORT2':['x2','x4']}
        >>> container_entries =
        >>> {'x1':[1,2,3],'x2':[4,5,6],'x3':[7,8],'x4':[9]}
        >>> restructured_map
        {'IP:PORT': [1, 2, 3, 7, 8], 'IP1:PORT2': [4, 5, 6, 9]}
        '''
        self.logger.info("Restructure Data")
        restructured_map = {}
        try :
            if not container_entries:
                for i in  component_map.keys():
                    restructured_map[i] = []

            else:
               
                for service_id in component_map.keys():
                    temp_list = []
                    for component_name in component_map[service_id]:
                        temp_list += container_entries.get(component_name,'')

                    #[temp_list.append(i) for i in \
                    #    container_entries[component_name]]
                    restructured_map[service_id] = temp_list
        #except KeyError:
        #    self.logger.info("Key Not found \
        #            container_entries:%s,gl_dictionary:%s"% \
        #            container_entries, component_map)
        except Exception as err:
            self.logger.error("Error while restructuring entries: %s" % err)

        self.logger.debug("Restructured data: %s" % restructured_map )
        return restructured_map

    def _journal_mount(self):
        """Unmount from remote node and mount on local node"""
        c_unmount_status = 0
        c_status = 0
        t_unmount_status = 0
        t_status = 0
        str_to_match = "No route to host"
        retrying_cont = 0 
        retrying_trans = 0 
        #unmount filesystem
        try:
            while retrying_cont < 2:
                if not ismount(self.__cont_path):
                    self.logger.info("unmount container")
                    out, err_msg, c_unmount_status = \
                        osd_utils.unmount_filesystem('.' + self.service_id.split('_')[0] + \
                                "_" + self.service_id.split('_')[1] + \
                                "_container_journal", \
                                str(self.service_id.split('_')[0]), cont_recovery=True)
                    self.logger.info("out:%s, err_msg:%s, c_unmount_status: %s" % (out, err_msg, c_unmount_status))
                    if str_to_match in err_msg:
                        self.logger.error("Node Down Possibility with %s"% err_msg)
                        c_unmount_status = 0
                        break
                    if c_unmount_status == 12:
                        self.logger.info("Exit status 12")
                        c_unmount_status = 0
                        break
                    if c_unmount_status == 255:
                        self.logger.info("Exit status 255")
                        c_unmount_status = 0
                        break
                    if (c_unmount_status == 9) and (retrying_cont < 1):
                        self.logger.info("Exit status 9")
                        eventlet.sleep(120)
                    if (c_unmount_status == 9) and (retrying_cont == 1):
                        self.logger.info("Exit status 9 second time")                        
                        c_unmount_status = 0
                        break
                retrying_cont += 1

            while retrying_trans < 2:
                if not ismount(self.__trans_path):
                    self.logger.info("unmount transaction")
                    out, err_msg, t_unmount_status = \
                        osd_utils.unmount_filesystem('.' + self.service_id.split('_')[0]+ \
                                "_" + self.service_id.split('_')[1] + \
                                "_transaction_journal", \
                                str(self.service_id.split('_')[0]), cont_recovery=True)
                    self.logger.info("out:%s, err_msg:%s, t_unmount_status: %s" % (out, err_msg, t_unmount_status))
                    if t_unmount_status == 12:
                        self.logger.info("Exit status 12")
                        t_unmount_status = 0
                        break
                    if str_to_match in err_msg:
                        self.logger.error("Node Down Possibility with %s"% err_msg)
                        t_unmount_status = 0
                        break
                    if t_unmount_status == 255:
                        self.logger.info("Exit status 255")
                        t_unmount_status = 0
                        break
                    if (t_unmount_status == 9) and (retrying_trans < 1):
                        self.logger.info("Exit status 9")
                        eventlet.sleep(120)
                    if (t_unmount_status == 9) and (retrying_trans == 1):
                        self.logger.info("Exit status 9 second time")
                        t_unmount_status = 0
                        break

                retrying_trans += 1

            if not (c_unmount_status == 0 and t_unmount_status == 0):
                self.logger.info("Containerjournalunmountstatus:%s, \
                              Transjournalunmountstatus:%s"% (c_unmount_status, \
                              t_unmount_status))
                sys.exit(130)
    
            self.logger.debug("Containerjournalunmountstatus:%s, \
                              Transjournalunmountstatus:%s"% (c_unmount_status, \
                              t_unmount_status ))

        except Exception as err:
            self.logger.debug("unmount failed %s " % err)
            sys.exit(130)

        #mount filesystem
        try:
            local_hostname = osd_utils.get_current_node()
            if not ismount(self.__cont_path):
                self.logger.info("mount cont journal")
                c_status = \
                    osd_utils.mount_filesystem('.' + self.service_id.split('_')[0] + \
                            "_" + self.service_id.split('_')[1] + \
                            "_container_journal", ['-O'], \
                             str(local_hostname))
                self.logger.info("c_status: %s" % c_status)
                self.logger.info("service_id:%s: container_journal" % self.service_id) 
            if not ismount(self.__trans_path):
                self.logger.info("mount cont journal")
                t_status = \
                    osd_utils.mount_filesystem('.' + self.service_id.split('_')[0] + \
                            "_" + self.service_id.split('_')[1] + \
                            "_transaction_journal", ['-O'], \
                             str(local_hostname))
                self.logger.info("t_status: %s" % t_status)
                self.logger.info("service_id:%s: transaction_journal" %self.service_id) 
            if not (c_status == 0 and t_status == 0):
                self.logger.info("Containerjournalmountstatus:%s, \
                              Transjournalmountstatus:%s"% (c_status, t_status))
                sys.exit(130)

            self.logger.info("Containerjournalmountstatus:%s, \
                         Transjournalmountstatus:%s"% (c_status, t_status ))
            return True

        except Exception as err:
            self.logger.info("Exception in Journal mount %s"% err)
            return False



class Recovery:
    """Recover data"""
    def __init__(self, conf, dictionary_new, communicator_obj, \
            service_component_map, service_id, gl_communication_obj=None, \
            logger=None, object_gl_version=None, gl_map=None):
        self.logger = logger or get_logger({}, log_route='recovery-container')
        self.pile_size = len(dictionary_new.keys())
        self.logger.info("Started Recovery after restructuring...")
        self.dictionary_new = dictionary_new

        #destination map of gl
        self.Map_from_gl = gl_map
        self.logger.info("map from gl in recovery %s" % self.Map_from_gl)
        self.conf = conf
        self.container_node_list = \
        [ {'ip':"%s" % key.split(':')[0], 'port':"%s" % \
            key.split(':')[1]}for key \
            in dictionary_new.keys()]
        self.logger.debug("Container node list:%s ,Dictionary passed : %s" % \
            (self.container_node_list, self.dictionary_new))

        self.service_component_map = service_component_map
        self.communicator_obj = communicator_obj
        self.service_id = service_id
        self.gl_communication_obj = gl_communication_obj
        self.__object_gl_version =  object_gl_version


    def recovery_container(self, method, send_intermediate_resp = False):
        """Make connections and get response"""
        self.logger.info("running recovery_container")
        global finish_recovery_flag
        try:
            client_timeout = int(self.conf.get('CLIENT_TIMEOUT', 20))
        except Exception as err:
            self.logger.info("Error is %s" % err)

        #Check Destinations to send
        if not self.container_node_list:
            self.logger.info("No Container Node List Found %s" % \
                self.container_node_list)
            raise NodeList("No node list found")
 
        self.logger.info("self.container_node_list: not empty")
        node_iter = GreenthreadSafeIterator(self.container_node_list)
        self.logger.info("pile size:%s" % self.pile_size)

        #Make Threads corresponding to destinations
        pile =  GreenPile(self.pile_size)
        for node in node_iter:
            nheaders = {}
            #self.pile.spawn(self._connect_put_node, node, \
            pile.spawn(self._connect_put_node, node, \
                nheaders, self.logger.thread_locals, method)

        eventlet.sleep(3)
        #for context switches

        #Get Alive connections
        conns = [conn for conn in pile if conn]
        self.logger.info("%s/%s connections made " % (conns, self.pile_size))

        statuses = [conn.resp.status for conn in conns if conn.resp]
        try:
            with ContextPool(self.pile_size) as pool:
                for conn in conns:
                    conn.failed = False
                    key = "%s:%s" % (conn.node['ip'], conn.node['port'])

                    self.logger.info("Reading data")
                    conn.reader = pickle.dumps(self.dictionary_new[key])

                    self.logger.debug("Sending Data List: %s" % \
                        self.dictionary_new[key])
                    pool.spawn(self._send_file, conn, '/recovery_process')
                pool.waitall()


            conns = [conn for conn in conns if not conn.failed]
            #Make a list of alive connections

            self.logger.info("Alive Connections: %s" % conns)
        except ChunkReadTimeout as err:
            self.logger.warn(
                 _('ERROR Client read timeout (%ss)'), err.seconds)
            self.logger.increment('client_timeout')
            conn.close()
        except ChunkWriteTimeout as err:
            self.logger.warn(
                _('ERROR  write timeout (%ss)'), err.seconds)
            self.logger.increment('client_timeout')
            conn.close()
        except Exception as ex:
            self.logger.error("Exception raised: %s" % ex)
            conn.close()
        except Timeout as ex:
            self.logger.info("Timeout occured : %s" % ex)
            self.logger.error(
                _('ERROR Exception causing client disconnect'))
            conn.close()

        eventlet.sleep(3)
        #for context switches

        statuses, reasons, bodies, comp_list_sent = \
            self._get_put_responses(conns, self.container_node_list, send_intermediate_resp)
        self.logger.info("Returned status:%s,reason:%s,bodies:%s" % \
            (statuses,reasons,bodies))
        
        eventlet.sleep(3)
        #for context switches

        if send_intermediate_resp:
            self.logger.info("send_intermediate_res: ")
            #finish_recovery_flag = True
         #set flag for finish recovery to send end_strm.

        return (statuses, reasons, bodies, comp_list_sent)

    def _connect_put_node(self, node, headers, logger_thread_locals,
            method="ACCEPT_COMPONENT_CONT_DATA"):
        """Method for a connect"""
        self.logger.thread_locals = logger_thread_locals
        try:
            client_timeout = int(self.conf.get('CLIENT_TIMEOUT', 500))
            conn_timeout = float(self.conf.get('CONNECTION_TIMEOUT', 30))
            node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
        except Exception as err:
            self.logger.info("error :%s"%err)
       
        self.logger.info("Entering connect_put method")

        #Some values are needed for connections.
        filesystem = 'export'
        directory = 'OSP'
        path = '/recovery_process/'
        #Parameters are fixed, could be changes according to requirement.

        #Making headers 
        headers['Expect'] = '100-continue'
        headers['X-Timestamp'] = time.time()
        headers['Content-Type'] = 'text/plain'
        headers['X-Object-Gl-Version-Id'] = self.__object_gl_version
        try:
            header_content_key = "%s:%s" % (node['ip'], node['port'])

            #Content-length is assumed to be in integer.
            Content_length = \
                len(pickle.dumps(self.dictionary_new.get(header_content_key)))
            headers['Node-ip'] = header_content_key
            headers['Content-Length'] = Content_length
            #headers['X-GLOBAL-MAP-VERSION'] = -1 (for now send actual map version)
            #Receive map version from GL.(service_id,logger)
            try:
                global_map_version = get_global_map_version(service_id_, logger_obj)
                if global_map_version:
                    logger_obj.info("Map version received: %s" % global_map_version)
                else:
                    logger_obj.debug("Map version not received so exit")
                    sys.exit(130)
            except Exception as err:
                logger_obj.debug("Map version exception raised, Exit :%s"% err)
                sys.exit(130)

            headers['X-GLOBAL-MAP-VERSION'] = global_map_version
            headers['X-Recovery-Request'] = True
            if self.service_component_map.has_key(header_content_key):
                headers['X-COMPONENT-NUMBER-LIST'] = self.service_component_map[header_content_key]
            else:
                headers['X-COMPONENT-NUMBER-LIST'] = []
            
            self.logger.info("Header Sent: %s" % headers)
            start_time = time.time()
            with ConnectionTimeout(conn_timeout):
                conn = http_connect(
                    node['ip'], node['port'], filesystem, directory, method,
                    path, headers)
            with Timeout(node_timeout):
                resp = conn.getexpect()
            if resp.status == HTTP_CONTINUE:
                conn.resp = None
                conn.node = node
                self.logger.info("HTTP continue %s" % resp.status)
                return conn
            elif is_success(resp.status):
                conn.resp = resp
                conn.node = node
                self.logger.info("Successfull status:%s" % resp.status)
                return conn
            elif resp.status == HTTP_INSUFFICIENT_STORAGE:
                self.logger.error(_('%(msg)s %(ip)s:%(port)s'),
                      {'msg': _('ERROR Insufficient Storage'), 
                      'ip': node['ip'],
                      'port': node['port']})
        except (Exception, Timeout) as err:
            self.logger.exception(
                _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                '%(info)s'),
                {'type': "Container", 'ip': node['ip'], 'port': node['port'],
                'info': "Expect: 100-continue on "})
            #raise could be added if needed.


    def _send_file(self, conn, path):
        """Method for sending data"""
        self.logger.info("send_data started")
        node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
        bytes_transferred = 0
        chunk = ""
        while conn.reader:
            chunk = conn.reader[:65536]
            bytes_transferred += len(chunk)

            if not conn.failed:
                try:
                    with ChunkWriteTimeout(node_timeout):
                        self.logger.debug('Sending chunk%s' % chunk)
                        conn.send(chunk)
                        conn.reader = conn.reader[65536:] #asrivastava: I think a send byte size should be checked.
                except (Exception, ChunkWriteTimeout):
                    conn.failed = True
                    self.logger.exception(
                        _('ERROR with %(type)s server %(ip)s:%(port)s/ re: '
                        '%(info)s'),
                        {'type': "Container", 'ip': conn.node['ip'], 'port':
                        conn.node['port'],
                        'info': "send file failed "})

        self.logger.info("Bytes transferred: %s" % bytes_transferred)
        self.logger.info("Sent File completed")


    def _get_put_responses(self, conns, nodes, send_intermediate_resp):
        post_quorum_timeout = float(self.conf.get('POST_QUORUM_TIMEOUT', 0.5))
        global final_recovery_status_list
        global final_status

        statuses = []
        reasons = []
        bodies = []
        self.logger.info("running get_put_response ..")
        def get_conn_response(conn):
            node_timeout = int(self.conf.get('NODE_TIMEOUT', 600))
            try:
                with Timeout(node_timeout):
                    if conn.resp:
                        self.logger.info("conn.resp returned")
                        return (conn.resp, conn)
                    else:
                        self.logger.info("conn.getresponse()")
                        return (conn.getresponse(), conn)
            except (Exception, Timeout):
                self.logger.exception(_("connection: %(conn)s \
                    get_put_response: %(status)s" ), \
                    {'conn': conn.node, \
                    'status': _('Trying to get final status ')})

        #Get response on Async.
        pile = GreenAsyncPile(len(conns)) 
        #pile = GreenPile(len(conns)) 
        for conn in conns:
            self.logger.info("Spawning get_conn_response for:%s" % conn.node)
            pile.spawn(get_conn_response, conn)

        component_list_sent = []
        for (response, conn) in pile:
        #Return responses from here to Gl.
            global list_to_gl
            if response:
                self.logger.info("response.status:%s" % type(response.status))
                #Send Intermediate response to Global Leader
                #for service_id, components in self.Map_from_gl.iteritems():

                for components in \
                    self.service_component_map['%s:%s' % (conn.node['ip'], \
                        conn.node['port'])]:
                    #Need to change 200 in int or str depends.
                    if response.status == 200:
                        self.logger.debug("Component no: %s" % components)
                        obj_id = ''
                        for key in self.Map_from_gl.keys():
                            if (key.get_ip() == str(conn.node['ip']))  and (str(key.get_port()) == str(conn.node['port'])):
                                obj_id = key
                        list_to_gl.append((components, obj_id))
                        #self.logger.info("Blocked before list to gl appended: %s" % obj_id)

                        eventlet.sleep()
                        #context-switch

                    else:
                        if (components, obj_id) in list_to_gl:
                            list_to_gl.remove((components, obj_id))

                component_list_sent.append('%s:%s'%(conn.node['ip'], \
                        conn.node['port']))

                eventlet.sleep(3)
                #context-switch

                self.logger.info("Component List: %s, list_to_gl: %s"% \
                    (list(set(component_list_sent)), list_to_gl))


                statuses.append((response.status, \
                    "%s:%s"%(conn.node['ip'], conn.node['port'])))
                reasons.append(response.reason)
                bodies.append(response.read())
                if response.status >= HTTP_INTERNAL_SERVER_ERROR:
                    self.logger.error("Error:%s returned from ip:%s port:%s" %\
                        (response.status, conn.node['ip'], \
                        conn.node['port']))
                elif is_success(response.status):
                    self.logger.info("success status returned")

        #pile.waitall(post_quorum_timeout)

        if send_intermediate_resp:
            self.logger.info("Filling Data in Queue")
            try:
                #list_of_tuple.put(list_to_gl, block=False)
                list_of_tuple.put_nowait(list(set(list_to_gl)))
            except Exception as err:
                self.logger.info("Queue Full exception: %s" % err)
            #list_of_tuple.put(list_to_gl)

            self.logger.info("Queue Put successfull")


        while len(statuses) < self.pile_size:
            statuses.append(HTTP_SERVICE_UNAVAILABLE)
            reasons.append('')
            bodies.append('')

        #send Final Response to GL.
        global clean_journal_flag
        for i in self.service_component_map:
            for j in self.service_component_map[i]:
                if i in component_list_sent:
                    final_recovery_status_list.append((j, True))
                else:
                    final_recovery_status_list.append((j, False))
                    final_status = False
                    clean_journal_flag = False # Do not clean journal if anything failed

        self.logger.info("final recovery status: %s"% final_recovery_status_list)
        self.logger.info("Returned status:%s, reason:%s, bodies:%s" % \
            (statuses, reasons, bodies))
        return statuses, reasons, bodies, component_list_sent




if __name__ == '__main__':

    #argument parser
    parser = argparse.ArgumentParser(description="Recovery process option \
                                     parser")
    parser.add_argument("server_id", help="Service id from Global leader.")
    args = parser.parse_args()
    service_id_ = args.server_id


    #Change Process name of Script.
    script_prefix = "RECOVERY_"
    proc_name = script_prefix + service_id_
    procname.setprocname(proc_name)


    #config file reader
    CONFIG_PATH = \
        "/opt/HYDRAstor/objectStorage/configFiles/container-recovery.conf"
    config = ConfigParser.ConfigParser()
    config.read(CONFIG_PATH)
    config_dict = dict(config.items('DEFAULT'))

    #Global variables for intermediated response handling
    list_of_tuple = queue(maxsize=1000)
    finish_recovery_flag = False
    final_status = True
    final_recovery_status_list = list()
    list_to_gl = []
    clean_journal_flag = True


    #logger initialize
    logger_obj = get_logger(config_dict, log_route='recovery')
    logger_obj.info("Parsed aguments:%s, configfile: %s, proc %s" % \
                    (service_id_, config_dict, proc_name))

    #Get Global map and retry 2 times.
    logger_obj.info("Start global map request")
    Request_handler_ = Req(logger_obj, timeout=0)
    gl_info_obj = Request_handler_.get_gl_info()

    retry_count = 0
    while retry_count < 3:
        retry_count += 1
        communicator_object_ = Request_handler_.connector(IoType.EVENTIO, gl_info_obj)
        if communicator_object_:
            logger_obj.debug("CONN object success")
            break

    if not communicator_object_:
        logger_obj.debug("No Connection object Found : Exiting")
        sys.exit(130)

    logger_obj.debug("conn obj:%s, retry_count:%s " % (communicator_object_, retry_count))

    gl_map_object = Request_handler_.recv_pro_start_monitoring(service_id_, \
        communicator_object_)

    #retry 2 times
    retry = 0
    while retry < 2:
        if gl_map_object.status.get_status_code() in [Resp.SUCCESS]:
            break
        elif gl_map_object.status.get_status_code() in [Resp.BROKEN_PIPE,
                Resp.CONNECTION_RESET_BY_PEER, Resp.INVALID_SOCKET]:
            communicator_object_ = Request_handler_.connector(IoType.EVENTIO, gl_info_obj)
            if communicator_object_:
                gl_map_object = \
                    Request_handler_.recv_pro_start_monitoring(service_id_,
                    communicator_object_)
            logger_obj.info("Got Conn object: %s"% communicator_object_)
            retry += 1
        else:
            logger_obj.info("Exiting due to failure in start_monitoring")
            sys.exit(130)

    if not communicator_object_:
        logger_obj.debug("No Connection object Found : Exiting")
        sys.exit(130)

    logger_obj.info("global map received with parameters:%s, %s, %s" % \
                    (Request_handler_, gl_info_obj, gl_map_object))

    # start recovery: RecoveryMgr().start_recovery()
    logger_obj.info("Starting Recovery Process")
    recv = RecoveryMgr(config_dict, gl_map_object.message, service_id_,
        communicator_object_, Request_handler_, logger_obj, gl_map_object.message.source_service())
    resp = eventlet.spawn(recv.start_recovery)
    resp.wait()

    #Cleaning journals
    if clean_journal_flag:
        logger_obj.info("Cleaning journal started")
        recv.clean_journals()
        logger_obj.info("Journals cleaned")
    else:
        logger_obj.info("Some components were not transfered so not cleaning Journals")

    #recv.start_recovery()
    logger_obj.info("Finished Recovery Process")
