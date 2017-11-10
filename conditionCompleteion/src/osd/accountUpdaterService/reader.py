import os
from threading import Thread
from osd.accountUpdaterService.monitor import WalkerMap, ReaderMap, \
GlobalVariables, StatFile
import time

MAX_CHUNK_SIZE=66000
maxContainerEntries = 1000

""" Reader class will read all the eligible file and form the structure e.g {'X8': {'2015-05-08.06:08:39': {'19fba02c397a05b2684081c3398db499': ['81081e6a45fead611d66fd4f6753e6be']}}, 'X9': {'2015-05-08.06:08:38': {}}, 'X3': {'2015-05-08.06:08:39': {'49fba02c297a05b2684081c3398db499': []}}} which will act as input to the Updater Class"""

class Reader(Thread):
    def __init__(self, walker_map, reader_map, conf, logger):
        Thread.__init__(self)
        self._walker_map = walker_map
        self._reader_map = reader_map
        self.logger = logger
        self.logger.info("Reader constructed")
        self._file_location = conf['stat_file_location']
        self.msg = GlobalVariables(self.logger)
        self._complete_all_event = self.msg.get_complete_all_event()
        self._transfer_cmp_event = self.msg.get_transfer_cmp_event()
        self.put_queue_flag = False

    def __reader_check_complete_all_event(self):
        """  Reader complete all request (Stop) Event check  """
        if self._complete_all_event.is_set():
            self.logger.info("Received complete all request event in reader")
            self._reader_map = {}
            self.msg.put_into_Queue()
            return True

    def __reader_get_new_ownership(self):
        """ Reader transfer/accept component request """
        if self._transfer_cmp_event.is_set() and not self.put_queue_flag:
            self.logger.info("Received transfer/accept request event in reader") 
            for key in self._reader_map.keys():
                if int(key) not in self.msg.get_ownershipList():
                    del self._reader_map[key]
            self.msg.put_into_Queue()
            self.put_queue_flag = True
        elif not self._transfer_cmp_event.is_set():
            self.put_queue_flag = False

    def read_stat_file(self, comp, stat_file):
        if not os.path.exists(os.path.join(self._file_location, comp, stat_file)):
            raise StopIteration
        with open(os.path.join(self._file_location, comp, stat_file), 'rt') as fd:
            while True:
                data = fd.read(MAX_CHUNK_SIZE)
                if not data:
                    break
                yield data.split(',')[:-1]

    def check_if_container_limit_exceeded(self):
        while self._reader_map.getContainerCount() > maxContainerEntries:
            self.logger.info("Container limit exceeded :%s therefore wait till"\
                "update completed" %(self._reader_map.getContainerCount()))
            time.sleep(10)

    def run(self):
        try:
            self.__perform_reader_operation()
        except Exception as ex:
            self.logger.error("error occured : %s" %ex)
            os._exit(130)
        except:
            self.logger.error("Unknown exception occured")
            os._exit(130)
        
    def __perform_reader_operation(self):
        while True:
            self.logger.debug("Reader started")
            if self.__reader_check_complete_all_event():
                break
            try:
                self.__reader_get_new_ownership()
                for entry in self._walker_map.getEntries():
                    comp, stat_file = entry.split("/")
                    self._stat_obj = StatFile(stat_file)
                    self.check_if_container_limit_exceeded()
                    self.logger.debug("Processing start for component:%s stat"
                        " files: %s" % (comp, stat_file))
                    for file_content in self.read_stat_file(comp, stat_file):
                        self._stat_obj.set_lock_flag(True)
                        for data in file_content:
                            account_name, container_name = data.split("/")
                            if comp not in self._reader_map.keys():
                                self._reader_map[comp] = {}
                            if self._stat_obj not in self._reader_map[comp].keys():
                                self._reader_map[comp][self._stat_obj] = {}
                            if account_name not in self._reader_map[comp][self._stat_obj].keys():
                                self._reader_map[comp][self._stat_obj][account_name] = []
                            self._reader_map[comp][self._stat_obj][account_name].append(container_name)
                            self._reader_map.incrementContainer()
                        self._stat_obj.set_lock_flag(False)
                    self.logger.info("Stat File:%s processing done for component:%s count:%s" \
                        %(stat_file, comp, self._reader_map.getContainerCount()))
                self.logger.debug("After completing all stat file processing, reader_map = \
                    %s" %(self._reader_map))
            except Exception as ex:
                self.logger.error("Exception occured :%s" %ex)
            self.logger.debug("Exiting Reader")
            time.sleep(35)



