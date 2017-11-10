"""
Mediator class for communicating with TranscationManager
and ContainerOperationManager
"""
from libTransactionLib import Request
from osd.common.swob import HTTPInternalServerError, \
    HTTPRequestTimeout
from osd.containerService.utils import SingletonType, get_container_id, \
    FAIL_ERROR, FAIL_TIMEOUT, WRITE_MODE, READ_MODE

class CommunicationMediator(object):
    """ 
    Mediator class for communicating transcation handler
    and operation handler components
    """
    __metaclass__ = SingletonType

    def __init__(self, logger):
        self.__logger = logger
        self.__operations_obj = None
        self.__trans_obj = None

    def set_trans_obj(self, trans_obj):
        """
        setter function for TranscationManager
        param:trans_obj: Transcation manager object
        """
        self.__trans_obj = trans_obj

    def set_cont_obj(self, cont_obj):
        """
        setter function for ContainerOperationManager
        param:cont_obj: Container operation object
        """
        self.__operations_obj = cont_obj

    def get_lock(self, account, container, mode, operation):
        """
        Method to call GET_LOCK of transcation handler component
        param:account: Account name
        param:container: Container name
        param:mode: mode type for operation
        param:operation: Type of operation
        """
        service_id = get_container_id()
        full_path = '%s/%s/' % (account, container)
        req_mode = WRITE_MODE
        if mode == 'r':
            req_mode = READ_MODE
        req = Request(full_path, operation, req_mode, service_id)
        trans_id = self.__trans_obj.internal_get_lock(req)
        if trans_id == FAIL_ERROR:
            raise HTTPInternalServerError()
        elif trans_id == FAIL_TIMEOUT:
            raise HTTPRequestTimeout()
        else:
            return trans_id

    def release_lock(self, trans_id):
        """
        Method to call FREE_LOCK of transcation handler component
        param:trans_id: Transaction ID to be released
        """
        try:
            return self.__trans_obj.internal_free_lock(trans_id)
        except Exception as err:
            raise err
