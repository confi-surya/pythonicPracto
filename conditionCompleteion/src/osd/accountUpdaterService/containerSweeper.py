import os
import time
from threading import Thread
from walker import Walker
from reader import Reader
from updater import Updater
from osd.accountUpdaterService.monitor import WalkerMap, ReaderMap, \
GlobalVariables
MAX_UPDATE_FILE_COUNT = 4

def remove_file(file_name):
    if os.path.exists(file_name):
        os.unlink(file_name)

class ContainerSweeper(Thread):
    """ Container Updater remover from the queue """
    def __init__(self, walker_map, reader_map, conf, logger):
        Thread.__init__(self)
        self._file_location = conf['stat_file_location']
        self.__interval = conf['interval']
        self.logger = logger
        self.msg = GlobalVariables(self.logger)
        self._walker_map = walker_map
        self._reader_map = reader_map
        self.logger.info("ContainerSweeper constructed")

    def run(self):
        try:
            self.__perform_container_sweeping_task()
        except Exception as ex:
            self.logger.error("error occured : %s" %ex)
            os._exit(130)
        except:
            self.logger.error("Unknown exception occured")
            os._exit(130)

    def __perform_container_sweeping_task(self):
        while True:
            self.logger.info("Container sweeper list : %s" \
                %self.msg.get_sweeper_list())
            temp_list = self.msg.get_sweeper_list()
            try:
                for comp_tuple in temp_list:
                    #self.msg.pop_from_sweeper_list(comp_tuple)
                    remove_file("%s/%s/%s" %(self._file_location, \
                        comp_tuple[0], comp_tuple[1]))
                    if comp_tuple[0] in self._reader_map.keys():
                        for obj in self._reader_map[comp_tuple[0]].keys():
                            if obj.get_stat_file() == comp_tuple[1]:
                                #decreasing the container count
                                for account, cont_list in self._reader_map[comp_tuple[0]][obj].items():
                                    counter = len(cont_list)
                                    self._reader_map.decrementContainerCount(counter)
                                del self._reader_map[comp_tuple[0]][obj]
                    for comp, val in self._reader_map.items():
                        if not val:
                            self._reader_map.pop(comp)
                    if (comp_tuple[0] + "/" + comp_tuple[1]) in \
                        self._walker_map:
                        self._walker_map.remove(comp_tuple[0] + "/" + \
                            comp_tuple[1])
                self.logger.debug("After deletion reader map:%s , " \
                    "walker map:%s" %(self._reader_map, self._walker_map))
                temp_list = []
                self.msg.pop_from_sweeper_list(temp_list)
            except Exception as ex:
                self.logger.error("Exception occured :%s" %ex)
                temp_list = []
                self.msg.pop_from_sweeper_list(temp_list)
            time.sleep(20)

