import os
from threading import Thread
import time
import httplib
from osd.accountUpdaterService.communicator import \
ServiceLocator, HtmlHeaderBuilder
#from osd.accountUpdaterService.updateAccountContainer import AccountClass, \
#ContainerClass
from osd.accountUpdaterService.monitor import WalkerMap, ReaderMap, \
GlobalVariables
from osd.accountUpdaterService.handler import AccountInfo, ContainerDispatcher 

MAX_UPDATE_FILE_COUNT = 4

class Updater(Thread):
    def __init__(self, walker_map, reader_map, conf, logger):
        Thread.__init__(self)
        self.conf = conf
        self.logger = logger
        self.logger.info("Updater constructed")
        self.msg = GlobalVariables(self.logger)
        self.__complete_all_request = False
        self._walker_map = walker_map
        self._reader_map = reader_map
        self._complete_all_event = self.msg.get_complete_all_event()
        self._transfer_cmp_event = self.msg.get_transfer_cmp_event()
        self.__container_dispatcher = ContainerDispatcher(self.conf, self.logger)
        self._updater_map = {}
        self.__other_file_list = []
        self.put_queue_flag = False

    def populate_updater_map(self):
        self.__other_file_list = []
        for component, stat_data in self._reader_map.items():
            for stat_obj, data in stat_data.items():
                if not stat_obj.get_lock_flag():
                    stat_file = stat_obj.get_stat_file()
                    if not self._updater_map.has_key((component, stat_file)):
                        self._updater_map[(component, stat_file)] = {}
                    if self._reader_map.get(component, None):
                        self._updater_map[(component, stat_file)] = self._reader_map[component][stat_obj]
                    self.__other_file_list.append((component, stat_file))
        self.logger.info("Updater map : %s" %self._updater_map)

    def __updater_check_complete_all_event(self):
        """  Updater complete all request (Stop) Event check  """
        if self._complete_all_event.is_set():
            self.logger.info("Received complete all request event in updater")
            self._updater_map = {}
            self.msg.put_into_Queue()
            return True

    def __updater_get_new_ownership(self):
        """ Updater transfer/accept component request """
        if self._transfer_cmp_event.is_set() and not self.put_queue_flag:
            self.logger.info("Received transfer/accept request event in updater")
            for comp_tuple in self._updater_map.keys():
                if int(comp_tuple[0]) not in self.msg.get_ownershipList():
                     del self._updater_map[comp_tuple]
            self.msg.put_into_Queue()
            self.put_queue_flag = True
        elif not self._transfer_cmp_event.is_set():
            self.put_queue_flag = False


    def __other_file(self, comp, file):
        listOfAccounts = []
        index = self.__other_file_list.index((comp, file))
        rest_comp_tuple_list = self.__other_file_list[:index] + self.__other_file_list[index+1:]
        for comp_tuple in rest_comp_tuple_list:
            listOfAccounts.extend(self._updater_map[comp_tuple].keys())
            #self.logger.debug("Current component :%s and other stat files having accounts %s" %(comp, listOfAccounts))
        return listOfAccounts

    def run(self):
        try:
            while True:
                self.logger.debug("Updater started")
                self.populate_updater_map()
                self.__updater_get_new_ownership()
                if self.__updater_check_complete_all_event():
                    self.__complete_all_request = True
                    break
                waitList = []
                delete_list=[]
                accObjectDict = {}
                container_list = [] #Container list contains AccountInfo objects where HEAD has been done on Accounts 
                for comp_tuple, map_ in self._updater_map.items():              #NOTE: compTuple = (componentNum, Filename)
                                                                                #NOTE: map_ = {account name : [container name]}
                    self.logger.info("Processing start for stat file : %s, map : %s" %(comp_tuple, map_))
                    #self.logger.debug("Modified account list: %s" % comp_tuple)
                    if self.__updater_check_complete_all_event():
                        break
                    for account, cont_list in map_.items():
                        #NOTE: loop to perform container head for a account and lastly perform account update
                        self.logger.info("Processing account: %s, container_list:%s" %(account, cont_list))
                        if account not in accObjectDict.keys():
                            #NOTE: in case account object is already created, no need to re-create the object
                            account_instance = AccountInfo(account, self.conf, self.logger)
                            accObjectDict[account] = account_instance
                        else:
                            account_instance = accObjectDict[account]
                        for container in cont_list:
                            #NOTE: for all container in the account perform container head request
                            if not account_instance.is_container(container):
                                #NOTE: in case container is already existing that means container head has already been performed.
                                #contObj = ContainerClass(container)
                                account_instance.add_container(container)
                        try:    
                            #NOTE: perform container HEAD
                            self.logger.debug("Processing for container HEAD request")
                            account_instance.get_container_path()
                            self.__container_dispatcher.dispatcher_executer(account_instance)
                            #If any container HEAD fails then do not perform account update operation and iterate next stat file
                            if "fail" in account_instance.account_map.values():
                                self.logger.info("Container HEAD is failing on %s, \
                                    skipping stat file %s" %(account, comp_tuple))
                                break
                            #NOTE: if the account is present in the other stat files than we should not perform account updater
                            # in this iteration. We should add it to waitList and will update account at the end.
                            if account_instance.getAccountName() in self.__other_file(\
                                comp_tuple[0], comp_tuple[-1]):
                                if account_instance.getAccountName() not in waitList:
                                    self.logger.info("Adding account :%s in the waitlist"\
                                    %account_instance.getAccountName())
                                    waitList.append(account_instance.getAccountName())
                                continue
                            #Performing account update operation if this account is not present in other stat files
                            if self.__container_dispatcher.account_update(account_instance):
                                account_instance.remove_container()
                            else:
                                self.logger.info("Account %s can not be modified" % account)
                                break
                        except Exception as err:
                            self.logger.error("Exception occured :%s", err)    
                    # do not create sweeper list of stat_file if any account update(wait list) of that stat_file remains
                    for account in map_.keys():
                        if account in waitList or ('fail' in accObjectDict[account].account_map.values()) or account_instance.acc_update_failed:
                            break
                    else:
                        self.msg.create_sweeper_list(comp_tuple)
                        del self._updater_map[comp_tuple]
                        self.__other_file_list.remove(comp_tuple)
                # Sending account update request for accounts exist in waitList at the end
                for comp_tuple, map_ in self._updater_map.items():
                    for account in map_.keys():
                        if account in waitList:
                            if self.__container_dispatcher.account_update(accObjectDict[account]):
                                accObjectDict[account].remove_container()
                                waitList.remove(account)
                            else:
                                self.logger.info("Account %s can not be modified" % account)
                                break
                        elif not accObjectDict.get(account):
                            continue
                        elif ('fail' in accObjectDict[account].account_map.values()):
                            self.logger.info("Container HEAD is failing on %s, \
                                skipping stat file %s" %(account, comp_tuple))
                            break
                        elif not accObjectDict[account].acc_update_failed:
                            self.logger.info("Account : %s already updated" %account)
                        elif accObjectDict[account].acc_update_failed:
                            self.logger.info("Account update for %s failed, \
                                skipping stat file %s" %(account, comp_tuple))
                            break
                    else:
                        self.msg.create_sweeper_list(comp_tuple)
                        del self._updater_map[comp_tuple]
                        self.__other_file_list.remove(comp_tuple)
                #self._updater_map = {} # Will be clear after one time processing
                self.logger.debug("Exiting Updater")
                time.sleep(40)

        except Exception as ex:
            self.logger.error("exception occured : %s" %ex)
            os._exit(130)
        except:
            self.logger.error("unkown exception occured")
            os._exit(130)

