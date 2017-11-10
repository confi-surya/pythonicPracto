from threading import Thread
import os
import logging
from osd.common.ring.ring import get_fs_list, get_service_id, AccountRing
from osd.accountUpdaterService.monitor import WalkerMap, ReaderMap, \
GlobalVariables
from subprocess import Popen, PIPE
import time
FS = "OSP_01"

class AccountSweep(Thread):
    """
    Sweep to deleted account
    """
    def __init__(self, conf, logger):
        Thread.__init__(self)
        self._file_location = os.path.join(conf['filesystems'], FS, 'tmp', \
            'account')
        self.del_path = 'del'
        self.conf = conf
        self.logger = logger
        self.logger.info("AccountSweep constructed")
        self.root = conf['filesystems']
        self.msg = GlobalVariables(self.logger)
        self._ownership_list = self.msg.get_ownershipList()
        self.put_queue_flag = False
        self._complete_all_event = self.msg.get_complete_all_event()
        self._transfer_cmp_event = self.msg.get_transfer_cmp_event()
        self._account_ring = AccountRing(conf['osd_dir'], self.logger)

    def _get_del_files(self):
        """Get the list of deleted temp account file"""
        del_files = {}
        try:
            for comp in self._ownership_list:
                if os.path.exists(os.path.join(self._file_location, \
                    str(comp), self.del_path)):
                    #self.logger.debug("listing deleted tmp account files on " \
                    #    "location: %s/%s/%s" \
                    #    % (self._file_location, str(comp), self.del_path))
                    del_files[comp] = os.listdir(os.path.join(\
                        self._file_location, str(comp), self.del_path))
            return del_files
        except Exception as ex:
            self.logger.error("While listing directory %s : %s" % \
                (self._file_location, ex))
            return del_files

    def _get_account_name(self, account_file):
        """Split name of account from file"""
        return account_file.split('.info')[0]

    def _delete_dir(self, path):
        """Delete dir"""
        if os.path.exists(path):
            running_procs = [Popen(['rm', '-rf', path], stdout=PIPE, \
                stderr=PIPE)]
            while running_procs:
                for proc in running_procs:
                    retcode = proc.poll()
                    if retcode is not None:
                        running_procs.remove(proc)
                        break
                    else:
                        time.sleep(.2)
                        continue
                if retcode != 0:
                    for line in proc.stderr.readlines():
                        self.logger.error("Error in proc.stderr.readlines()")
                    return False
        else:
            self.logger.debug("Path %s does not exist" % path)
        return True

    def _clean_account_data(self, deleted_account, fs_list):
        status = True
        acc_dir = self._account_ring.get_directory_by_hash(\
            self._get_account_name(deleted_account))
        for fs in fs_list[::-1]:
            account_path = os.path.join(self.root, fs, acc_dir, \
                self._get_account_name(deleted_account))
            try:
                if not self._delete_dir(account_path):
                    status = False
            except Exception, ex:
                self.logger.error("while deleting dir: %s : %s" \
                    % (account_path, ex)) 
                status = False
        return status

    def _sweep_account(self):
        """Sweep account data"""
        deleted_accounts_List = {}
        deleted_accounts_List = self._get_del_files()
        fs_list = get_fs_list(self.logger)
        #fs_list = ['OSP_01','OSP_02','OSP_03','OSP_04','OSP_05','OSP_06',
        #'OSP_07','OSP_08','OSP_09','OSP_10']
        for comp, deleted_accounts in deleted_accounts_List.items():
            for deleted_account in deleted_accounts:
                if self._clean_account_data(deleted_account, fs_list):
                    os.remove("%s/%s/%s/%s" %(self._file_location, comp, \
                                            self.del_path, deleted_account))

    def __accountSweep_check_complete_all_event(self):
        """ Event check of Service Stop """
        if self._complete_all_event.is_set():
            self.logger.info("Received complete all request event in " \
                "accountSweeper")
            self.msg.put_into_Queue()
            return True

    def __accountSweep_get_new_ownership(self):
        """ Event check of getnewcompMap """
        if self._transfer_cmp_event.is_set() and not self.put_queue_flag:
            self.logger.info("Received transfer/accept request event in" \
                "accountSweeper")
            self._ownership_list = self.msg.get_ownershipList()
            self.msg.put_into_Queue()
            self.put_queue_flag = True
        elif not self._transfer_cmp_event.is_set():
            self.put_queue_flag = False

    def run(self):
        while True:
#            time.sleep(5) #TODO: will be removed
            self._ownership_list = self.msg.get_ownershipList()
            if self.__accountSweep_check_complete_all_event():
                break
            
            self.__accountSweep_get_new_ownership()
            try:
                self._sweep_account()
            except Exception as ex:
                self.logger.error("Exception occured :%s" %ex)
                os._exit(130)
            self.logger.debug("accountSweeper exited")
            time.sleep(30)

