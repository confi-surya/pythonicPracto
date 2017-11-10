import traceback
import cPickle as pickle
import os
import shutil
import time
from os.path import join
from eventlet import Timeout
from osd import gettext_ as __
from osd.common.ring.ring import ObjectRing, ContainerRing, get_all_service_id, hash_path
from osd.common.utils import mkdirs, create_recovery_file, \
    remove_recovery_file
from osd.common.helper_functions import split_path
from osd.common.utils import normalize_timestamp
from osd.objectService.diskfile import EXPORT_PATH, DATADIR, METADIR, TMPDIR, \
    DATAFILE_SYSTEM_META
from osd.objectService.objectutils import http_connection
from osd.common.swob import HeaderKeyDict
from osd.common.http import is_success
from libHealthMonitoringLib import healthMonitoring


OSD_DIR = '/export/.osd_meta_config'
#pylint: disable=C0103

class RecoveryHandler:
    '''
     Do both startup recovery and triggered recovery.

     During startup recovery, the tmp directory is looked for files in it.
     If found, then files are being processed, send request to container
     service for update and then finally send request to container
     service to release lock.

     During triggered recovery, file is searched at actual location. If
     found, then request is send to container service for update and
     then finally send request to container service to release lock. 
    '''
    def __init__(self, object_service_id, logger, object_ring=None):
        self.__object_service_id = object_service_id
        self.health = None
        self.__ring = object_ring or ObjectRing(OSD_DIR, logger)
        self.__directory_iterator_obj = DirectoryIterator(logger)
        self.__triggered_recovery_handler_obj = TriggeredRecoveryHandler( \
                                                logger, self.__ring)
        self._logger = logger

    def startup_recovery(self, __get_node_ip=None, __port=None, __ll_port=None, __serv_id=None, recovery_flag=False):
        '''
        Start the start up recovery procedure.
        '''
        self._logger.info("Starting start up recovery process for " 
                          "object server: %s" % self.__object_service_id)
        try:
            create_recovery_file('object-server')
        except Exception as err:
            self._logger.error('Failed to create recovery file')
            return False

        #jai:-To recover data directory wise 
        #we would need to comment out this section
        #try:
        #    self._move_data()
        #except (OSError, Exception):
        #    return False
        tmp_dir_list_to_recover = self._get_tmp_dir_list()
        try:
            for tmp_dir in tmp_dir_list_to_recover:
                try:
                    self.__directory_iterator_obj.recover(tmp_dir)
                except (OSError, Exception) as err:
                    self._logger.error(__(
                        'ERROR Could not complete startup recovery process '
                        'for tmp directory: %(tmp_dir)s'
                        ' close failure: %(exc)s : %(stack)s'),
                        {'exc': err, 'stack': ''.join(traceback.format_stack()),
                         'tmp_dir': tmp_dir})
                    return False
        except Exception as err:
            self._logger.error(__(
                'ERROR Could not complete startup recovery process for '
                'object server: %(object_service_id)s'
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack()),
                 'object_service_id': self.__object_service_id})
            return False
        finally:
            try:
                # Start sending healthMonitoring to local leader
                if not recovery_flag:
                    self.health = healthMonitoring(__get_node_ip, __port, __ll_port, __serv_id)
                    self._logger.debug("Health monitoring instance executed")
                status = None
                remove_recovery_file('object-server')
            except Exception as err:
                self._logger.error('Failed to remove recovery file %s'% err)
                status = True
        
        if status:
            return False
        return True

    def bulk_delete_request(self, data_file_path, meta_file_path, path, host):
        '''
        Update container in case of bulk delete request
        '''
        try:
            self._logger.info('Starting container update in case of bulk_delete')
            self.__triggered_recovery_handler_obj.recover_delete_request(data_file_path, meta_file_path, path, host)
            self._logger.info('Container Updated for in case of bulk delete')
        except Exception as err:
            self._logger.error((
                'ERROR Could not complete triggered recovery '
                'process for file: %(file)s',
                'close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack()),
                 'file': path})

    def triggered_recovery(self, path, method, host):
        '''
        Start the triggered recovery procedure.
        :param path: object path to be recovered
        :returns: True, when file is recoverd successfully
                  False, otherwise.
        '''
        try:
            self._logger.info('Starting triggered recovery for file: %s'
                               % path) 
            self.__triggered_recovery_handler_obj.recover(path, method, host)
        except Exception as err:
            self._logger.error(__(
                'ERROR Could not complete triggered recovery '
                'process for file: %(file)s'
                'close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack()),
                 'file': path})
        self._logger.info('Successfully completed triggered recovery '
                          'for file: %s'   % path) 

    def _move_data(self):
        '''
        Move data from old node to current node id
        '''

        tmp_dir_list = self._get_tmp_dir_list_to_move()
        try:
            for tmp_dir in tmp_dir_list:
                old_node_id = tmp_dir.split('/')[-1].split('_')[0]
                current_node_id = self.__object_service_id.split('_')[0]
                new_tmp_dir = tmp_dir.replace(old_node_id, current_node_id)
                if os.path.exists(new_tmp_dir):
                    if os.listdir(tmp_dir):
                        for root, _, files in os.walk(tmp_dir):
                            for afile in files:
                                shutil.move(os.path.join(root, afile), new_tmp_dir)
                    shutil.rmtree(tmp_dir)
                else:
                    os.rename(tmp_dir, new_tmp_dir)
        except (OSError, Exception) as err:
            self._logger.error(__(
                'ERROR While moving data from %(old)s to %(new)s '
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack()),
                'old': old_node_id, 'new': current_node_id})
            raise
       

    def _get_file_system_list(self):
        '''
        Return file system list
        :returns: file system list
        '''
        return self.__ring.get_fs_list()

    def _get_tmp_dir_list(self):
        '''
         Returns all tmp directories list for a object service
        :returns: list of tmp directories
        '''
        tmp_dir_list = []
        fs_list = self._get_file_system_list()
        for file_system in fs_list:
            tmp_dir = os.path.join(EXPORT_PATH, file_system, TMPDIR,
                                   self.__object_service_id)
            tmp_dir_list.append(tmp_dir)
        return tmp_dir_list
        

    def _get_tmp_dir_list_to_move(self):
        '''
         Returns all tmp directories list for a object service to move
        :returns: list of tmp directories
        '''
        tmp_dir_list = []
        fs_list = self._get_file_system_list()
        current_node_id = self.__object_service_id.split('_')[0]
        for file_system in fs_list:
            tmp_dir = os.path.join(EXPORT_PATH, file_system, TMPDIR)
            if os.path.exists(tmp_dir):
                for directory in os.listdir(tmp_dir):
                    if 'object' in directory:
                        node_id = directory.split('_')[0]
                        if node_id != current_node_id:
                            tmp_dir_list.append(os.path.join(tmp_dir, directory))
                        else:
                            pass
        return tmp_dir_list


class DirectoryIterator:
    '''
    Iterates over each directory to recover files.
    '''
    def __init__(self, logger):
        self.__recover_file_obj = FileRecovery(logger)
        self._logger = logger


    def __process_dir(self, tmp_dir):
        '''
        Process each tmp dir for each files
        :param tmp_dir: location of tmp dir to be processed
        '''
        data_file = []
        meta_file = []
        try:
            files = os.listdir(tmp_dir)
            for afile in files:
                if afile.endswith('.data'):
                    data_file.append(afile)
                elif afile.endswith('.meta'):
                    meta_file.append(afile)
                else:
                    pass 
        except OSError:
            self._logger.error(__(
                'ERROR No such directory %(tmp_dir)s'),
                {'tmp_dir': tmp_dir})
        except Exception as err:
            self._logger.error(__(
                'ERROR While recovering directory %(tmp_dir)s'
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack()),
                 'tmp_dir': tmp_dir})
            raise
        return data_file, meta_file

    def recover(self, tmp_dir):
        '''
        Calls recovery for each tmp file.

        :param tmp_dir: tmp directory
        '''
        status = None
        self._logger.info("Starting recovery for tmp dir: %s" % tmp_dir)
        try:
            data_file_list, meta_file_list = self.__process_dir(tmp_dir)
        except Exception:
            raise
        if not data_file_list and not meta_file_list:
            self._logger.info(__(
                'No tmp files found in %(tmp_dir)s'),
                {'tmp_dir': tmp_dir})

            status = True
        else:
            try:
                status = self._recover_each_tmp_dir(tmp_dir, data_file_list, \
                                                    meta_file_list)
            except Exception as err:
                self._logger.exception((
                    'ERROR failure in startup recovery: %(tmp_dir)s'
                    ' close failure: %(exc)s : %(stack)s'),
                    {'exc': err, 'stack': ''.join(traceback.format_stack()),
                    'tmp_dir' : tmp_dir})
                raise
        if status:
            self._logger.info("Recovery completed for tmp_dir: %s" % tmp_dir)
        else:
            self._logger.info("Could not complete recovery "
                              "for tmp_dir: %s" % tmp_dir)

    def _recover_each_tmp_dir(self, tmp_dir, data_file_list, meta_file_list):
        '''
        Look for files and starts recovery according to following rules:
 
        *case (1) When both files exists with data in it:
            In this case, both files will be moved to actual position,
            request will be sent to container service to update info, and
            finally to release lock.
    
        *case (2) When both files exists but meta file is empty:
            In this case, the files will be deleted and finally send request
            to container service for releasing lock.

        *case (3) When only data file exists:
            In this case, the files will again be deleted.

        *case (4)  When only data file exists:
            In this case, actual location of data file will be searched.
            If found, then meta file will be moved to actual location, send
            request to container service for updating info file, and finally
            to  release lock.

        :param tmp_dir: tmp directory location
        :param data_file_list: list containing data files
        :param meta_file_list: list containing meta files
        :returns: True, if recovery successful
                  False, otherwise
        '''
        status = False
        try:
            for afile in data_file_list:
                self._logger.info("Starting recovery for data file: %s" \
                                  % afile)
                file_name = afile.split('.data')[0]
                meta_file = file_name + '.meta'
                if not meta_file in meta_file_list:
                    self.__recover_file_obj.recover_incomplete_file(file_name)
                    os.remove(os.path.join(tmp_dir, afile))
                else:
                    if not os.path.getsize(os.path.join(tmp_dir, meta_file)):
                        self.__recover_file_obj.recover_incomplete_file( \
                            file_name)
                        os.remove(os.path.join(tmp_dir, afile))
                        os.remove(os.path.join(tmp_dir, meta_file))
                        meta_file_list.remove(meta_file)
                    else:
                        self.__recover_file_obj. \
                            recover_complete_file(tmp_dir, file_name)
                        meta_file_list.remove(meta_file)
                self._logger.info("Completed recovery for data file: %s" \
                                  % afile)
            if len(meta_file_list):
                for afile in meta_file_list:
                    self._logger.info("Starting recovery for meta file: %s" \
                                      % afile)
                    if os.path.getsize(os.path.join(tmp_dir, afile)):
                        self.__recover_file_obj.recover_complete_file(tmp_dir,
                            afile.split('.meta')[0], True)
                    else:
                        os.remove(os.path.join(tmp_dir, afile))
                    self._logger.info("Completed recovery for meta file: %s" \
                                      % afile)
        except (OSError, IOError, Exception) as err:
            raise err
        else:
            status = True
        return status

class FileRecovery:
    '''
    Write file to actual location.
    '''
    def __init__(self, logger, object_ring=None):
        self._logger = logger
        self.__communicator_obj = CommunicateContainerService(self._logger)
        self.__object_ring = object_ring or \
                             ObjectRing(OSD_DIR, self._logger)

    def recover_complete_file(self, tmp_dir, file_name, only_meta=False):
        '''
        Writes file to actual location and call container update request method
        and then finally release lock request.
 
        :param tmp_dir: tmp directory
        :param file_name: file name to be recovered
        :param only_meta: flag to identify only meta file case
        '''
        update_status, release_status = None, None
        metadata = None
        duplicate = False
        try:                
            meta_file = file_name + '.meta'
            with open(os.path.join(tmp_dir, meta_file), 'rb') as meta_fd:
                orig_metadata = pickle.loads(meta_fd.read())
            metadata = dict(
                [(key, val) for key, val in orig_metadata.iteritems()
                 if key.lower() in DATAFILE_SYSTEM_META])
            metadata.update({'name': orig_metadata['name']})
            metadata.update({'X-Timestamp': orig_metadata['X-Timestamp']})
        except (Exception, IOError) as err:
            self._logger.error(__(
                'ERROR  While reading %(meta)s file'
                ' close failure: %(exc)s : %(stack)s'),
                {'exc': err, 'stack': ''.join(traceback.format_stack()),
                 'meta' : os.path.join(tmp_dir, file_name + '.meta')})
            raise
        obj_hash = file_name.replace('_', '/')
        data_target_path, meta_target_path = self.__get_target_path(metadata,
                                                 file_name)
        data_file = os.path.join(tmp_dir, file_name) + '.data'
        meta_file = os.path.join(tmp_dir, file_name) + '.meta'
        if os.path.exists(data_target_path) and os.path.exists(meta_target_path):
            duplicate = True
        try:
            if not only_meta:
                mkdirs(os.path.dirname(data_target_path))
                os.rename(data_file, data_target_path)  
            mkdirs(os.path.dirname(meta_target_path))
            os.rename(meta_file, meta_target_path)
        except OSError:
            self._logger.error("Failure during file renaming")
            raise
        self._logger.info("Sending request to container service "
                          "for file: %s" % data_file)

        update_status = self.__communicator_obj. \
                            container_update(metadata, duplicate)
        if not update_status:
            self._logger.error("Could not update container")
        else:
            self._logger.info(__("Container update successful for "
                               "file: %s" % data_file))
            self._logger.info(__("Sending request for releasing "
                               "lock: %s" % data_file))
            release_status = self.__communicator_obj. \
                                  release_lock(obj_hash, metadata)
            if release_status:
                self._logger.info(__("Release lock successful for "
                                   "file: %s" % metadata['name']))
            else:
                self._logger.error("Could not release lock for "
                                   "file: %s" % metadata['name'])

    def recover_incomplete_file(self, file_name):
        '''
        Send request to container service to release lock when the files
        are incomplete.

        :param tmp_dir: tmp directory path
        :file_name: file name
        '''
        obj_hash = file_name.replace('_', '/')
        status = self.__communicator_obj.release_lock(obj_hash)
        if status:
            self._logger.info("Release lock successful for "
                               "file: %s" % file_name)
        else:
            self._logger.error("Could not release lock for "
                               "file: %s" % file_name)


    def __get_target_path(self, metadata, file_name):
        '''
        Get targets path of object file and meta file storage.
        :param tmp_dir: tmp directory
        :param file_name: file name
        :returns: data file target path and meta file target path
        '''
        data_file = file_name.split('_')[-1] + '.data'
        meta_file = file_name.split('_')[-1] + '.meta'
        path = metadata['name']
        account, container, obj = split_path(path, 1, 3, True)
        #_, file_system, directory, global_map_version, comp_no = \
        file_system, directory = \
            self.__object_ring.get_node(account, container, obj, only_fs_dir_flag=True)

        acc_dir, cont_dir, obj_dir = directory.split('/')
        data_target_path = join(EXPORT_PATH, file_system, acc_dir, hash_path(account), \
                                cont_dir, hash_path(account, container), \
                                obj_dir, DATADIR, data_file)
        meta_target_path = join(EXPORT_PATH, file_system, acc_dir, hash_path(account), \
                                cont_dir, hash_path(account, container), \
                                obj_dir, METADIR, meta_file)
        return data_target_path, meta_target_path


class CommunicateContainerService:
    '''
    Communicate with container service to update file and release lock request
    '''
    def __init__(self, logger=None, container_ring=None):
        self._logger = logger
        self.__container_ring = container_ring or \
                      ContainerRing(OSD_DIR, self._logger)

    def __get_service_details(self, metadata, host=''):
        '''
        Get container service details, filesystem, directory
        of container update request.

        :param metadata: object metadata
        :returns: host:       string containing IP and port of container service
                  directory:  directory name
                  file_system:filesystem name
        '''
        account, container, _ = split_path(metadata['name'], 1, 3, True)
        node, filesystem, directory, global_map_version, comp_no = \
            self.__container_ring.get_node(account, container)
        #TODO : Gl interface
        #GET_GLOBAL_MAP
        #component_name = get_path_name()
        #import libraryutils
        #ip,port = get_ip(),get_port()
        #host = '%s:%s' % (ip,port)
        if not host:
            host = '%s:%s' % (node[0]['ip'], node[0]['port'])
        return host, directory, filesystem


    def container_update(self, metadata, duplicate=False,
                         method='PUT', host=None, unknown=False):
        '''
        Send request to container service for info file update

        :param metadata: object metadata
        :param duplicate: If set to true, then container update request
                          will contain 'x-duplicate-update: True' header
                          else it not contain any additional header.
        :param method: method name
        :param host: host IP and port
        :returns: True, If update successful
                  False, otherwise.
        '''
        if not metadata:
            header = HeaderKeyDict({'x-timestamp': \
                normalize_timestamp(time.time())})
            file_system, directory = '', ''
        else:
            host, directory, file_system = self.__get_service_details(metadata, host=host)
            try:
                header = HeaderKeyDict({
                    'x-global-map-version': '-1',
                    'x-size': metadata['Content-Length'],
                    'x-content-type': metadata['Content-Type'],
                    'x-timestamp': metadata['X-Timestamp'],
                    'x-etag': metadata['ETag']})
            except KeyError:
                return False
            header.update({'x-object-path': metadata['name']})
        if duplicate:
            header.update({'x-duplicate-update': True})
        if unknown:
            header.update({'x-duplicate-unknown': True})
        try:
            resp = http_connection(host, file_system, directory, \
                                   header, method)
        except (Exception, Timeout):
            self._logger.exception(__(
                'ERROR container update failed '
                '%(host)s/%(fs)s '),
                {'host': host,
                'fs': file_system})
            return False
        resp.read()
        if not is_success(resp.status):
            return False
        return True

    def release_lock(self, obj_hash, metadata=None, host=''):
        '''
        Send request to container service for releasing transaction lock.

        :param obj_hash: object hash
        :returns: True, if lock is released
                  False, otherwise.
        '''
        # TODO: In case of recovery of incomplete files, we do not have
        #       account, container, obj name. So evaluating container service
        #       ip, port to which request will be submitted to release lock is
        #       not possible at this time. So this is a just a work around to
        #       get container service details.
        #       In future, some other solution can be implemented to better cope
        #       with such situation.
        directory, filesystem = '', ''
        if not metadata:
            try:
                try:
                    self._logger.info("obj hash : %s " % obj_hash)
                    #hash(/acc/cont)
                    cont_hash = obj_hash.split('/')[1]
                    #hash(/acc)
                    acc_hash = obj_hash.split('/')[0]
                except Exception as err:
                    self._logger.error("Exception raised")

                service_details, filesystem, directory = \
                    self.__container_ring.get_service_list(acc_hash, cont_hash)
                self._logger.info("ip : port in release lock : %s" % service_details)
                #service_list = ([{'ip': u'169.254.1.52', 'port': 61007}], filesystem, directory)
                if not host:
                    host = '%s:%s' % (service_details[0]['ip'], service_details[0]['port'])
                self._logger.debug(" Details -> filesystem: %s directory: %s" % \
                                   (filesystem, directory))
            except Exception as err:
                self._logger.error(__("Error raised in release lock %s" \
                    % err))

        else:
            host, filesystem, directory = self.__get_service_details(metadata, host=host)
        try:
            header = HeaderKeyDict({'x-object-path': obj_hash, 'x-global-map-version': '-1'})
            resp = http_connection(host, filesystem, directory, \
                                   header, 'FREE_LOCKS')
        except (Exception, Timeout):
            self._logger.exception(__(
                'ERROR Release lock failed '
                '%(ip)s for %(obj_name)s'),
                {'ip': host,
                 'obj_name': obj_hash})
            return False
        resp.read()
        if not is_success(resp.status):
            return False
        return True

class TriggeredRecoveryHandler:
    """
    Class to handle any request for triggered recovery
    """
    def __init__(self, logger, ring):
        """
        :param logger: logger
        :param ring: ObjectRing instance
        """
        self._logger = logger
        self._ring = ring
        self.__communicator_obj = CommunicateContainerService(self._logger)


    def recover_delete_request(self, data_file_path, meta_file_path, path, host):
        """
        Does triggered recovery for object PUT operations
        :param data_file_path: data file path
        :param meta_file_path: metadata file path
        :param path: object path
        :param host: string containing container service details
        """
        if os.path.exists(data_file_path) and os.path.exists( \
                                              meta_file_path):
            release_status = self.__communicator_obj. \
                                 release_lock(path, host=host)
            if release_status:
                self._logger.info(__("Release lock successful for "
                                   "file: %s" % path))
            else:
                self._logger.error("Could not release lock for "
                                   "file: %s" % path)
        else:
            if os.path.exists(meta_file_path):
                with open(meta_file_path, 'rb') as meta_fd:
                    orig_metadata = pickle.loads(meta_fd.read())
                    metadata = HeaderKeyDict({'name': orig_metadata['name'],
                                              'x-size': orig_metadata['Content-Length'],
                                              'X-Timestamp': orig_metadata['X-Timestamp']})
                # TODO: 
                # Now there is no way to identify whether the update
                # request is overwrite case or not. Hence sending request
                # for new entry.
                status = self.__communicator_obj. \
                             container_update(metadata, \
                             method='DELETE', host=host, unknown=True)
                if not status:
                    self._logger.error(__("Could not update container "
                                        "for file: %s" % path))
                else:
                    self._logger.info(__("Container update successful for "
                                       "file: %s" % path))
                    os.remove(meta_file_path)
                    release_status = self.__communicator_obj. \
                                         release_lock(path, host=host)
                    if release_status:
                        self._logger.info(__("Release lock successful for "
                                           "file: %s" % path))
                    else:
                        self._logger.error("Could not release lock for "
                                           "file: %s" % path)
            else:
                release_status = self.__communicator_obj. \
                                          release_lock(path, host=host)
                if release_status:
                    self._logger.info(__("Release lock successful for "
                                       "file: %s" % path))
                else:
                    self._logger.error("Could not release lock for "
                                       "file: %s" % path)

    def recover_put_request(self, data_file_path, meta_file_path, path, host):
        """
        Does triggered recovery for object PUT operations
        :param data_file_path: data file path
        :param meta_file_path: metadata file path
        :param path: object path
        :param host: string containing container service details
        """
        if os.path.exists(data_file_path) and os.path.exists( \
                                              meta_file_path):
            with open(meta_file_path, 'rb') as meta_fd:
                orig_metadata = pickle.loads(meta_fd.read())
                metadata = dict(
                    [(key, val) for key, val in orig_metadata.iteritems()
                    if key.lower() in DATAFILE_SYSTEM_META])
                metadata.update({'name': orig_metadata['name']})
                metadata.update({'X-Timestamp': orig_metadata['X-Timestamp']})
                # TODO: 
                # Now there is no way to identify whether the update
                # request is overwrite case or not. Hence sending request
                # for new entry.
                status = self.__communicator_obj. \
                             container_update(metadata, host=host, unknown=True)
                if not status:
                    self._logger.error(__("Could not update container "
                                        "for file: %s" % path))
                else:
                    self._logger.info(__("Container update successful for "
                                       "file: %s" % path))
                    release_status = self.__communicator_obj. \
                                      release_lock(path, host=host)
                    if release_status:
                        self._logger.info(__("Release lock successful for "
                                           "file: %s" % metadata['name']))
                    else:
                        self._logger.error("Could not release lock for "
                                           "file: %s" % metadata['name'])
        else:
            release_status = self.__communicator_obj. \
                                 release_lock(path, host=host)
            if release_status:
                self._logger.info(__("Release lock successful for "
                                   "file: %s" % path))
            else:
                self._logger.error("Could not release lock for "
                                   "file: %s" % path)


    def recover(self, path, method, host):
        """
        Does recovery for an object path.
        Looks for file at actual location. If both meta and data file
        is found, then it sends request to container service for update.
        Else, if only data file is present, it deletes the file and in case
        only meta file is present, it deletes the file and send request to
        container service for update and then send request for releasing lock.

        :param path: object path
        :param method: method name
        :param host: string containing container service IP and port
        """
        try:
            acc_hash, cont_hash, obj_hash = path.split('/')
            file_system, directory = self._ring.get_directories_by_hash(acc_hash, cont_hash, obj_hash)
            acc_dir, cont_dir, obj_dir = directory.split('/')
            data_file_path = os.path.join(EXPORT_PATH, file_system, acc_dir,
                acc_hash, cont_dir, cont_hash,
                obj_dir, DATADIR, obj_hash + '.data')
            meta_file_path = os.path.join(EXPORT_PATH, file_system, acc_dir,
                acc_hash, cont_dir, cont_hash,
                obj_dir, METADIR, obj_hash + '.meta')
            if method == 'DELETE':
                self.recover_delete_request(data_file_path, meta_file_path, path, host)
            else:
                self.recover_put_request(data_file_path, meta_file_path, path, host)
        except Exception as err:
            raise err
