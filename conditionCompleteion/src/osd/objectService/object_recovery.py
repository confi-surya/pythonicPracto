import argparse
import procname
from threading import Thread, Lock, Condition
from osd.glCommunicator.req import Req
from osd.glCommunicator.response import Resp
from osd.common.utils import get_logger

from osd.objectService.recovery_handler import RecoveryHandler
from osd.common.ring.ring import get_fs_list
from osd.common.constraints import check_mount
from osd.common.utils import public, get_logger
import sys
from osd.glCommunicator.communication.communicator import IoType


class RecoveryStatus(object):
    def __init__(self):
        self.status = False
    def get_status(self):
        return self.status
    def set_status(self, status):
        self.status = status


class RecoveryObject:
    '''
    object service recovery 
    :param service_id: service id to recover

    '''
    def __init__(self, config, logger, communicator, recovery_stat, service_id):
        self.__recovery_status = recovery_stat
        self.__serv_id = service_id
        self.__mount_check = True
        self.__root = "/export"
        self.logger = logger or get_logger(config, log_route='object-recovery')
        self.__recovery_handler_obj = \
            RecoveryHandler(self.__serv_id, self.logger)
        self._communicator_obj = communicator
        gl_info_obj = communicator.get_gl_info()
        retry_count = 0 
        while retry_count < 3:
            retry_count += 1
            self._con_obj = communicator.connector(IoType.EVENTIO, gl_info_obj)
            if self._con_obj:
                self.logger.debug("CONN object success")
                break
        self.logger.debug("conn obj:%s, retry_count:%s " % (self._con_obj, retry_count))


        self._network_fail_status_list = [Resp.BROKEN_PIPE, Resp.CONNECTION_RESET_BY_PEER, Resp.INVALID_SOCKET]
        self._success_status_list = [Resp.SUCCESS]

        #flags
        self._final_recovery_status_list = []
        self._strm_ack_flag = False
        self._final_status = True
        self._finish_recovery_flag = False

        # return variables code
        ret = self._communicator_obj.recv_pro_start_monitoring( \
                    self.__serv_id, self._con_obj)

        if ret.status.get_status_code() in self._success_status_list:
            self.logger.info("Strm status successfull")
        else:
            gl_info_obj = self._communicator_obj.get_gl_info()
            retry_count = 0
            while retry_count < 3:
                retry_count += 1
                self._con_obj = self._communicator_obj.connector(IoType.EVENTIO, gl_info_obj)
                if self._con_obj:
                    self.logger.debug("CONN object success")
                    ret = self._communicator_obj.recv_pro_start_monitoring(self.__serv_id, self._con_obj)
                    if ret.status.get_status_code() in self._success_status_list:
                        break
                    else:
                        self.logger.error("Start Monitoring Failure")
                        sys.exit(130)

            self.logger.debug("conn obj:%s, retry_count:%s " %(self._con_obj, retry_count))
            # set flag, main thread will read this flag

        #TODO Start sending health to local leader
        # return variables code

        self.logger.debug("Initialized Object Recovery")



    def __end_monitoring(self):
        ret = self._communicator_obj.comp_transfer_final_stat(self.__serv_id, \
                                         self._final_recovery_status_list, self._final_status, \
                                         self._con_obj)


    def recover_data(self):
        """
        recover data
        """
        try:
            #pass logger
            recovery = self.__recovery_status.get_status()

            if not recovery:
                status = self.__check_filesystem_mount()
                if not status:
                    self.logger.error('Mount check failed before recovery')
                    print "Object Server Recovery failed"
                    sys.exit(130)
                status = self.__recovery_handler_obj.startup_recovery(recovery_flag=True)
                if not status:
                    self.logger.error('Object server recovery failed to remove file')
                    sys.exit(130)
                self.__recovery_status.set_status(True) 
        except Exception as err:
            self.logger.error('Object server recovery failed, error: %s' % err)
            self._final_status = False
            sys.exit(130)

        # set the finish transfer flag
        self.logger.info("Finished cleaning directories")
        self._finish_recovery_flag = True

        self.logger.info("Object server id : %s Recovery exiting" % self.__serv_id)

        #send end monitoring
        self.logger.info("Sending end mon from object recovery process")
        self.__end_monitoring()
        self.logger.info("End mon Completed for object recovery process")
        


    def __check_filesystem_mount(self):
        """
        Check mount of all OSD filesystems
        before recovery
        """
        filesystem_list = get_fs_list(self.logger)
        if self.__mount_check:
            for file_system in filesystem_list:
                if not check_mount(self.__root, file_system):
                    self.logger.info(('mount check failed '
                        'for filesystem %(file)s'), {'file' : file_system})
                    return False
        return True 



if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="Recovery process option parser")
    parser.add_argument("server_id", help="Service id from Global leader.")
    args = parser.parse_args()
    __service_id = args.server_id

    #Change Process name of Script.
    script_prefix = "RECOVERY_"
    proc_name = script_prefix + __service_id
    procname.setprocname(proc_name)


    recovery_status = RecoveryStatus()
    conf = {'log_requests': 'false',
            'log_failure': 'true',
            'log_level': 'debug',
            'log_facility': 'LOG_LOCAL6',
            'log_name': 'object_recovery'
            }
    #create logger obj
    logger_obj = get_logger(conf, log_route='object-recovery')
    logger_obj.info("Service received is : %s of type %s " % \
                    (__service_id, type(__service_id)))

    #create communication object
    communicator_obj = Req(logger_obj)

    recover = RecoveryObject(conf, logger_obj, communicator_obj, recovery_status, __service_id)
    recover.recover_data()
    logger_obj.info("Object Recovery finished")

