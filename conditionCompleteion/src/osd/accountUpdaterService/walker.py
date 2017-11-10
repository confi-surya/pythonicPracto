import os
import socket
from threading import Thread
import threading, Queue
import time
from osd.accountUpdaterService.utils import mergesorting, respondsecondsDiff 
from osd.accountUpdaterService.updater import Updater
from osd.accountUpdaterService.reader import Reader
from osd.accountUpdaterService.monitor import WalkerMap, ReaderMap, \
GlobalVariables, SweeperQueueList

""" Walker class find the oldest eligible file along with comnponent e.g ['X5/2015-05-08.06:08:38', 'X4/2015-05-08.06:08:38', 'X9/2015-05-08.06:08:38'] which will act as input for Reader Class """

class Walker(Thread):
    def __init__(self, walker_map, conf, logger):
        Thread.__init__(self)
        #self.__conf = conf # TODO : once the complete configuration file will be created correct entries would be fetched
        self._file_location = conf['stat_file_location']
        self._max_update_files = conf['max_update_files']
        self._max_files_stored_in_cache = conf['max_files_stored_in_cache']
        self._expire_delta_time = conf['updaterfile_expire_delta_time']
        self.logger = logger
        self.msg = GlobalVariables(self.logger)
        #self.__MainPath = '/remote/hydra042/spsingh/Latest/objectStorage/src/osd/NewUpdater/devesh/sps/upfiles1' #Todo: will be fetched from configuration file 
        #self.__ownershipList = ['X9','X3','X4','X5','X6','X8'] #TODO: dummy value, will be set at service start
        self._ownership_list = self.msg.get_ownershipList()
        self._walker_map = walker_map
        self.logger.info("Walker constructed")
        self._complete_all_event = self.msg.get_complete_all_event()
        self._transfer_cmp_event = self.msg.get_transfer_cmp_event()
        self.put_queue_flag = False

    """ Service Stop Event Handling """

    def __walker_check_complete_all_event(self):
        """  Walker complete all request (Stop) Event check  """
        if self._complete_all_event.is_set():
            self.logger.info("Received complete all request event in walker") 
            self._walker_map = []
            self.msg.put_into_Queue()
            return True

    """ Get Component Map Event Handling """

    def __walker_get_new_ownership(self):
        """ Walker transfer/accept component request """
        if self._transfer_cmp_event.is_set() and not self.put_queue_flag:
            self.logger.info("Received transfer/accept request event in walker")
            self._ownership_list = self.msg.get_ownershipList()
            self.msg.put_into_Queue()
            self.put_queue_flag = True
        elif not self._transfer_cmp_event.is_set():
            self.put_queue_flag = False

    def run(self):
        try:
            self.__perform_walker_operation()
        except Exception as ex:
            self.logger.error("error occured : %s" %ex)
            os._exit(130)
        except:
            self.logger.error("Unknown exception occured")
            os._exit(130)

    def __perform_walker_operation(self):
        old_stat_files = {}
        while True:
            self.logger.debug("Walker started")
            temp_structure = []
            self._ownership_list = self.msg.get_ownershipList()
            if self.__walker_check_complete_all_event():
                break
            self.__walker_get_new_ownership()
            try:
                if not old_stat_files:
                    old_stat_files = {} 
                    all_stat_files = {}
                    for comp in self._ownership_list:
                        if os.path.exists(os.path.join(self._file_location, str(comp))):
                            for file in os.listdir('%s/%s' %(self._file_location, comp)):
                                all_stat_files[os.path.join(self._file_location, str(comp), file)] = \
                                    os.path.getmtime(os.path.join(self._file_location, str(comp), file))
                            for old_file, modtime in sorted(all_stat_files.items(), key=lambda file: \
                                file[1])[:-1][:self._max_files_stored_in_cache]:
                                old_stat_files[os.path.join(old_file.split('/')[-2:-1][0], \
                                    os.path.basename(old_file))] = all_stat_files[old_file]

                            if len(os.listdir('%s/%s' %(self._file_location, comp))) == 1:
                                left_file = os.listdir('%s/%s' %(self._file_location, comp))[0]
                                totalsecond_diff = respondsecondsDiff(left_file)
                                if totalsecond_diff > self._expire_delta_time:
                                    old_stat_files[os.path.join(str(comp),left_file)] = \
                                       os.path.getmtime(os.path.join(self._file_location, str(comp), left_file))
                                else:
                                    self.logger.debug("Stat file %s not Expired: " \
                                       %os.path.join(self._file_location, str(comp), left_file))

                            all_stat_files = {}
                        else:
                            self.logger.debug("Directory :%s not exists" %os.path.join(self._file_location, str(comp)))                       
                    for stat_file in old_stat_files.keys():
                        if os.stat(os.path.join(self._file_location, stat_file)).st_size == 0:
                            comp_tuple = (stat_file.split('/')[0], stat_file.split('/')[1])
                            self.logger.info("Stat file :%s is empty therefore deleting it" \
                                 %os.path.join(self._file_location, stat_file))
                            self.msg.create_sweeper_list(comp_tuple)
                            del old_stat_files[stat_file]
                
                for stat_file, modtime in sorted(old_stat_files.items(), key=lambda file : \
                    file[1])[:self._max_update_files]:
                    if stat_file not in temp_structure:
                        temp_structure.append(stat_file)
                    del old_stat_files[stat_file]

                self._walker_map[:] = temp_structure[:self._max_update_files]
                self.logger.debug("State files to be processed at walker :%s" %self._walker_map)
            except Exception as ex:
                self.logger.error("Exception occured :%s", ex)
            self.logger.debug("Exiting walker")
            time.sleep(30)


