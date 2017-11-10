""" Container service transcation manager component"""
import time
from osd.common.swob import HTTPServiceUnavailable, \
    HTTPConflict, HTTPNotAcceptable
from osd.containerService.utils import SingletonType, AsyncLibraryWithPipe, \
    FAIL_ERROR, WRITE_MODE, READ_MODE, MAX_EXP_TIME, FAIL_TIMEOUT
from libTransactionLib import Request, TransactionLibrary, ActiveRecord
from osd.common.ring.ring import hash_path
import traceback
import socket

class TransactionManager(object):
    """ Acquire and release lock interface class"""

    __metaclass__ = SingletonType

    def __init__(self, journal_path_trans, node_id, recovery_port, logger):
        """ 
        Initialize function
        """
        self._logger = logger
        #create library instance
        self.__trans_lib = TransactionLibrary(journal_path_trans, node_id, recovery_port)
        self.asyn_helper = AsyncLibraryWithPipe(self.__trans_lib)

    def acquire_bulk_delete_lock(self, filesystem, acc_dir, cont_dir, account, container, object_names):
        """
        """
        try:
            self._logger.debug("Received obj list: %s" % object_names)
            req_mode = WRITE_MODE
            obj_service_id = socket.gethostname() + "_object-server"
            active_records = []
            for obj in object_names:
                obj_hash = "%s/%s/%s" % (hash_path(account), \
                                     hash_path(account, container), \
                                     hash_path(account, container, obj))
                self._logger.debug("Locking obj: %s" % obj_hash)
                active_records.append(ActiveRecord(int(req_mode), obj_hash, 'BULK_DELETE', obj_service_id, int(0)))
            status_obj = self.asyn_helper.call("add_bulk_delete_transaction", active_records)
            ret_val = status_obj.get_value()
            self._logger.debug("Transaction manager returning acquire_bulk_delete_lock with: %s" % ret_val)
            return status_obj
        except Exception as err:
            self._logger.error(
                'ERROR Transaction method add_bulk_delete_transaction failed')
            self._logger.exception(err)
            raise err


    def internal_get_lock(self, req):
        """
        Method to acquire lock from transcation library
        param: libTransactionLib Request class object
        return: transcation ID or failure
        """
        try:
            self._logger.debug('Called add transaction interface of library')
            status_obj = self.asyn_helper.call("add_transaction", req)
            trans_id = status_obj.get_value()
            return trans_id
        except Exception as err:
            self._logger.error(
                'ERROR container method GET_LOCK failed')
            self._logger.exception(err)
            raise err


    def conflict_dict(self, path_obj, comp_no):
        """
        Conflict id's retrieval 
        """
        try:
            self._logger.debug('Called transaction interface of library')
            #TODO:jaivish lib interface name
            trans_id_dict = self.__trans_lib.recover_active_record(path_obj, int(comp_no))
            return trans_id_dict
        except Exception as err:
            self._logger.error(
                'ERROR container method GET_LOCK failed')
            self._logger.exception(err)
            raise err


    def get_lock(self, request):
        """
        Method to acquire lock from transcation library
        param:HTTP request
        return: Return transcation ID
        """
        try:
            service_id, full_path, operation = '', '', ''
            req_mode = WRITE_MODE
            try:
                service_id = request.headers['x-server-id']
                full_path = request.headers['x-object-path']
                operation = request.headers['x-operation']
                if request.headers['x-mode'] == 'r':
                    req_mode = READ_MODE
            except:
                self._logger.debug('GET_LOCK Request doesnot contain'
                ' the correct headers!')
                raise HTTPConflict(request=request)
            req_trans_id = request.headers.get('x-trans-id', '-')
            self._logger.debug(('get_lock called with transaction'
                'ID:- %(id)s'), {'id' : req_trans_id})
            req = Request(full_path, operation, req_mode, service_id)
            trans_id = self.internal_get_lock(req)
            if trans_id == FAIL_ERROR:
                # This is raised when lock is already acquired
                raise HTTPNotAcceptable(request=request)
                #raise HTTPConflict(request=request) 
            elif trans_id == FAIL_TIMEOUT:
                 # This is raised when lock queue is full
                raise HTTPServiceUnavailable(request=request)
            else:
                trans_id_dict = {'x-request-trans-id': str(trans_id)}
                self._logger.debug("Active transaction size ====%d" %len(self.__trans_lib.get_transaction_map())) 
            return trans_id_dict
        except Exception as err:
            self._logger.exception(err)
            raise err

    def internal_free_lock(self, trans_id):
        """
        Method to free lock from transcation library
        param: transcation ID to be freed
        return: success or failure
        """
        try:
            self._logger.debug('Called release lock interface of library')
            status_obj = self.asyn_helper.call("release_lock", trans_id)
            ret_val = status_obj.get_value()
            if not ret_val:
                self._logger.error(
                'ERROR internal free lock failed!')
            return ret_val
        except Exception as err:
            self._logger.error(
            'ERROR container method FREE_LOCK failed')
            self._logger.exception(err)
            raise err

    def free_lock(self, request):
        """
        Method to release lock from transcation library on the
        basis of transcation ID
        param: HTTP request
        """
        try:
            trans_id = 'x-request-trans-id'
            if not trans_id in request.headers:
                self._logger.debug('Request headers doesnot contain '
                'the correct headers')
                raise HTTPConflict(request=request)
            trans_id = request.headers.get('x-request-trans-id')
            req_trans_id = request.headers.get('x-trans-id', '-')
            self._logger.debug(('free_lock called with transaction'
                'ID:- %(id)s'), {'id' : req_trans_id})
            return self.internal_free_lock(int(trans_id))
        except Exception as err:
            self._logger.exception(err)
            raise err

    def free_locks(self, request):
        """
        Method to release lock from transcation library on the 
        basis of object name
        This method is called in case of object startup recovery
        param: HTTP request
        """
        try:
            obj_hash = 'x-object-path'
            if not obj_hash in request.headers:
                self._logger.debug('Request headers doesnot contain '
                'the correct headers')
                raise HTTPConflict(request=request)
            obj_hash = request.headers.get('x-object-path')
            req_trans_id = request.headers.get('x-trans-id', '-')
            self._logger.debug(('free_locks called with transaction'
                'ID:- %(id)s'), {'id' : req_trans_id})
            status_obj = self.asyn_helper.call("release_lock", obj_hash)
            ret_val = status_obj.get_value()
            if not ret_val:
                self._logger.error(
                'ERROR internal free locks failed!')
            return ret_val
        except Exception as err:
            self._logger.error((
            'ERROR container method FREE_LOCKS failed for object: '
            '%(obj_name)s'), {'obj_name' : obj_hash})
            self._logger.exception(err)
            raise err

    def get_list(self):
        """
        Get expired list from library
        """
        try:
            return self.__trans_lib.get_transaction_map()
        except Exception as err:
            self._logger.exception(err)
            raise err

    def get_expired_trans_id(self):
        """
        Get expired transaction IDs of object service
        """
        transaction_map = {}
        keys_list = []
        try:
            transaction_map = self.get_list()
            for key, active_record in transaction_map.iteritems():
                record_time = active_record.get_time()
                time_now = time.time()
                if (time_now - record_time) < MAX_EXP_TIME:
                    keys_list.append(key)
        except Exception as err:
            self._logger.error('Get expired transcation list failed!')
            self._logger.exception(err)
        for item in keys_list:
            del transaction_map[item]
        return transaction_map

    def start_recovery_trans_lib(self,my_components):
        """
        Start recovery of transaction library
        """
        try:
            self._logger.debug('Called start recovery interface of '
                'Transaction Library')
            return self.__trans_lib.start_recovery(my_components)
        except Exception as err:
            self._logger.exception(err)
            return False

    def __accept_transaction_data(self, active_records):
        try:
            self._logger.debug('accept_transaction_data interface called')
            return self.asyn_helper.call \
                ("accept_transaction_data", active_records)
        except Exception as err:
            self._logger.error('accept_transaction_data failed '.join(traceback.format_stack()))
            raise err
            
    def accept_components_trans_data(self, active_records):
        try:
            status_obj = self.__accept_transaction_data(active_records)
            status = status_obj.get_error_code() 
            self._logger.info("Response code : %s" % status)
            return status
        except Exception as err:
            self._logger.error(('accept_transaction_data failed'
                'close failure: %(exc)s : %(stack)s'),
                {'obj' : status_obj,
                'exc': err, 'stack': ''.join(traceback.format_stack())})
            raise err

    def commit_recovered_data(self,components, base_version_changed):
        #Call the commit_recovered_data function of container library
        try:
            self._logger.debug('commit_recovered_data interface called')
            status_obj = self.__trans_lib.commit_recovered_data(components, base_version_changed)
            status = status_obj.get_return_status()
            return status
        except Exception as err:
            self._logger.error('commit_recovered_data failed'.join(traceback.format_stack()))
            raise err

    def extract_transaction_data(self, components):
        try:
            return self.__trans_lib.extract_transaction_data(components)
        except Exception as err:
            self._logger.error('extract_transaction_data failed'.join(traceback.format_stack()))
            raise err

    def revert_back_trans_data(self, transaction_records, memory_flag):
        try:
            self.__trans_lib.revert_back_trans_data(transaction_records, memory_flag)
        except Exception as err:
            self._logger.error('revert_back_trans_data failed'.join(traceback.format_stack()))
            raise err

