""" 
Class to create journal file and insert entries in journal file.
Also to read journal file in case of account updater request(GET_LIST) 
and container service recovery.
"""
import os
import glob
import cPickle as pickle

# First journal file
JOURNAL_FILE = 'cont_serv_journal.log.1'
# Max file size of journal(50 MB)
MAX_FILE_SIZE = 50 * 1024 * 1024
# Default checkpoint value (5 MB)
CHECKPOINT_MULTIPLIER = 5 * 1024 * 1024
# Max checkpoint length (638)
CHECKPOINT_LEN = 638
# Record entry fixed size
FIXED_RECORD_SIZE = 16
# Record entry maker
START_MARKER = '####'
# Record end marker
END_MARKER = '&&&&'
# Single entry record value
RECORD_SINGLE = '1902'
# Snap shot record value
RECORD_SNAPSHOT = '1903'
# Checkpoint record value
RECORD_CHECKPOINT = '1904'
#Read chunk size
CHUNK = 65535

def latest_file(path, logger):
    """
    Create a journal file from path
    Returns the latest journal file
    """
    jour_file = ''
    try:
        if not os.path.exists(path):
            os.makedirs(path)
            jour_file = os.path.join(path, JOURNAL_FILE)
            logger.info(('Container service journal file created :'
                '%(file)s'),
                {'file' : jour_file})
        else:
            updated_path = '%s/%s' % (path, 'cont_serv_journal*')
            files = glob.glob(updated_path)
            if not files:
                jour_file = os.path.join(path, JOURNAL_FILE)
                logger.info(('Container service journal file created :'
                '%(file)s'),
                {'file' : jour_file})
                return jour_file
            jour_file = max(glob.glob(updated_path))
    except Exception as err:
        logger.error(('ERROR occurred when creating '
            'container journal file for path: %(path)s'),
            {'path' : path})
        logger.exception(err)
    return jour_file

class FileHandler(object):
    """
    Helper class for file read and write
    """
    def __init__(self, path, mode, logger):
        """
        Initialize function
        """
        self.__path = path
        self.__logger = logger
        self.__fd = None
        self.open(mode)

    def open(self, mode):
        """
        Function to open a file
        """
        try:
            self.__fd = open(self.__path, mode)
        except Exception as err:
            self.__logger.error('Cannot open journal file!')
            self.__logger.exception(err)
            raise err

    def write(self, data):
        """
        Function to write in a file
        """
        try:
            self.__fd.write(data)
        except Exception as err:
            self.__logger.error('Cannot write to journal file!')
            self.__logger.exception(err)
            raise err

    def read(self, chunk):
        """
        Function to read from a file
        """
        return self.__fd.read(chunk)

    def tell(self):
        """
        Function to tell the current position in a file
        """
        return self.__fd.tell() 

    def seek(self, offset):
        """
        Function to seek to a position in a file
        """
        self.__fd.seek(offset) 

    def sync(self):
        """
        Function to flush data to memory
        """
        self.__fd.flush() 

    def close(self):
        """
        Function to close a file
        """
        self.__fd.close()

    def get_name(self):
        """
        Function to get name of a file
        """
        path = self.__fd.name
        return path.rsplit('/')[-1]

    def get_end_offset(self):
        """
        Function to get eof of a file
        """
        self.__fd.seek(0, 2)
        return self.tell()

class JournalManager(object):
    """
    Class that manages journal read/write and recovery
    """
    def __init__(self, path, logger):
        """
        Initialize function
        """
        self.__logger = logger
        self.__journal = JournalImpl(path, logger)

    def start_recovery(self):
        """
        Start recovery of container service
        Returns True/False
        """
        return self.__journal.start_recovery()

    def add_entry(self, single_dict):
        """
        Function to add an entry to journal file
        param: single_dict: Dictionary to be dumped to journal

        The dictionary 'single_dict' contains account name as key
        and container name as value
        Returns True/False
        """
        ret = self.__journal.write_journal(RECORD_SINGLE, single_dict)
        if not ret:
            self.__logger.error('Single entry addition in journal failed')
        return ret

    def add_snapshot(self, memory_snapshot):
        """
        Function to add memory snapshot to journal file
        This function is called only in case of account updater request.
        param:memory_snapshot: Dictionary send to account updater
        Returns True/False
        """
        ret = self.__journal.write_journal(RECORD_SNAPSHOT, memory_snapshot)
        if not ret:
            self.__logger.error('Memory snapshot addition to journal failed')
        return ret

    def get_list(self, req):
        """
        Handle GET_LIST request
        Returns dictionary of modified container list or None
        """
        return self.__journal.get_list(req)

class ProcessJournal(object):
    """
    Class for perform recovery of journal file
    Also a helper class for getting account updater memory
    """
    def __init__(self, path, logger):
        """
        Initialize function
        """
        self.__path = path
        self.__logger = logger
        self.__fd = None
        self.__final_dict = {}
        self.__prev_file_data = ''

    def __common_checks(self, offset_list):
        """
        Function to perform common checks on checkpoint data
        """
        try:
            if not offset_list:
                return True
            return (len(offset_list.keys()) == 4)
        except AttributeError:
            self.__logger.error(('Dict type object is not '
                'returned from checkpoint! Type is:- %(type)s'),
                {'type' : type(offset_list)})
            return False
        except Exception as err:
            self.__logger.exception(err)
            return False

    def recover(self):
        """
        Start recovery of container service
        This function is invoked just to set data members 
        so that correct journal write can be done.
        """
        # set last snapshot offset, second last snapshot offset
        # and next checkpoint
        try:
            offset_list = self.__process_checkpoint()
            if not self.__common_checks(offset_list):
                return False, offset_list
            return True, offset_list
        except Exception as err:
            self.__logger.error('Recovery of container service journal failed')
            self.__logger.exception(err)
            raise err

    def __process_piece(self, data):
        """
        Helper function to create final dictionary 
        to be send to account updater
        """
        value = ''
        for key in data.keys():
            try:
                if key in self.__final_dict:
                    value = self.__final_dict[key]
                else:
                    value = self.__final_dict.setdefault(key, set())
                value = value.union(data[key])
                self.__final_dict.update({key : value})
            except Exception as err:
                self.__logger.exception(err)
                raise err

    def __read_till_eof(self, data):
        """
        Function to read data from a snapshot offset till eof of a file
        """
        counter = False
        try:
            data = self.__prev_file_data + data
            for record in data.split(START_MARKER):
                if record:
                    value = (record[:4]).replace('\0', '')
                    length = int(value)
                    record_type = record[4:8]
                    data = record[8:8+length]
                    end = record[8+length:len(record)]
                    end = end.replace('\0', '')
                    if end != END_MARKER:
                        self.__prev_file_data = record
                        continue
                    if record_type == RECORD_CHECKPOINT:
                        continue
                    if record_type == RECORD_SNAPSHOT:
                        if counter == True:
                            continue
                        self.__final_dict = pickle.loads(data)
                        counter = True
                        continue
                    self.__process_piece(pickle.loads(data))
        except Exception as err:
            self.__logger.exception(err)
            raise err

    def __calculate_new_name(self, filename):
        """
        Function to get new filename by adding +1 to given file
        """
        _, tail = os.path.split(filename)
        value = tail.rsplit('.')
        journal_file = '%s.%s.%s' % (value[0], value[1], int(value[2]) + 1)
        return os.path.join(self.__path, journal_file)

    def __get_final_dict(self, filename, offset):
        """
        Helper function to create final dictionry to be send
        """
        try:
            filename = os.path.join(self.__path, filename)
            while (os.path.exists(filename)):
                self.__fd = FileHandler(filename, 'r')
                self.__fd.seek(offset)
                while 1:
                    data = self.__fd.read(CHUNK)
                    if not data:
                        break
                    self.__read_till_eof(data)
                self.__fd.close()
                filename = self.__calculate_new_name(filename)
        except Exception as err:
            raise err

    def create_list(self, req):
        """
        Processes snapshot to get create account updater memory
        """
        self.__final_dict = {}
        self.__prev_file_data = ''
        try:
            offset_list = self.__process_checkpoint()
            if not self.__common_checks(offset_list):
                return False, self.__final_dict
            if offset_list:
                if 'x-account-recovery' in req.headers:
                    self.__get_final_dict(offset_list['prev_filename'], \
                        offset_list['sec_last_off'])
                else:
                    self.__get_final_dict(offset_list['curr_filename'], \
                        offset_list['last_off'])
            else:
                self.__logger.error('No container list is available'
                    ' in container journal file!')
        except Exception as err:
            self.__logger.error('Cannot process snapshot in journal file!')
            self.__logger.exception(err)
            return False, self.__final_dict
        return True, self.__final_dict

    def __process_all_checkpoints(self, last_offset):
        """
        Function to get all checkpoints in a journal file
        """
        valid_checkpoint = 0
        while last_offset >= CHECKPOINT_MULTIPLIER:
            last_checkpoint = (last_offset/CHECKPOINT_MULTIPLIER) \
                    * CHECKPOINT_MULTIPLIER
            if self.__is_valid_checkpoint(last_checkpoint):
                valid_checkpoint = last_checkpoint
                break
            last_offset = last_checkpoint - 1
        return valid_checkpoint

    def __read_checkpoint(self, record=False):
        """
        Function to read a checkpoint in a file
        """
        try:
            data = self.__fd.read(CHECKPOINT_LEN)
            if record:
                return data[8:12]
            data = data.split(END_MARKER)[0]
            pickled_data = data[12:]
            return pickle.loads(pickled_data)
        except Exception as err:
            self.__logger.error('Cannot read journal file!')
            self.__logger.exception(err)
            raise err

    def __is_valid_checkpoint(self, last_offset):
        """
        Function to process a offset to check if it is a valid checkpoint
        """
        try:
            self.__fd.seek(last_offset)
            record_type = self.__read_checkpoint(True)
            if record_type == RECORD_CHECKPOINT:
                return True
        except Exception as err:
            self.__logger.error('Cannot process checkpoint in journal file!')
            self.__logger.exception(err)
        return False

    def __get_previous_file(self, filename):
        """
        Function to create new journal file name by substracting -1
        to latest journal file
        """
        _, tail = os.path.split(filename)
        value = tail.rsplit('.')
        filename = '%s.%s.%s' % (value[0], value[1], int(value[2]) - 1)
        filename = os.path.join(self.__path, filename)
        return filename

    def __process_checkpoint(self):
        """
        Function to process a checkpoint offset for getting checkpoint data
        Returns checkpoint data
        """
        valid_checkpoint = 0
        offset_list = {}
        filename = latest_file(self.__path, self.__logger)
        try:
            while (os.path.exists(filename)):
                self.__fd = FileHandler(filename, 'r')
                last_offset = self.__fd.get_end_offset()
                valid_checkpoint = self.__process_all_checkpoints(last_offset) 
                if valid_checkpoint == 0:
                    # Checkpoint not found in current file
                    # Open previous file
                    filename = self.__get_previous_file(self.__fd.get_name())
                    self.__fd.close()
                else:
                    self.__fd.seek(valid_checkpoint)
                    offset_list = self.__read_checkpoint()
                    break
            return offset_list
        except Exception as err:
            self.__logger.error('Cannot process checkpoint in journal file!')
            self.__logger.exception(err)
            raise err
        finally:
            self.__fd.close()

class JournalImpl(object):
    """
    Class that perform operations on journal file
    """
    def __init__(self, path, logger):
        """
        Initialize function
        """
        self.__path = path
        self.__logger = logger
        self.__fd = FileHandler(latest_file(path, logger), 'a+')
        self.__curr_file_name = self.__fd.get_name()
        self.__prev_file_name = ''
        self.__last_offset = 0
        self.__sec_last_offset = 0
        self.__next_checkpoint = CHECKPOINT_MULTIPLIER
        self.__recovery = ProcessJournal(path, logger)

    def __calculate_new_name(self):
        """
        Function to create new journal file name by adding +1
        to previous journal file
        """
        _, tail = os.path.split(self.__curr_file_name)
        value = tail.rsplit('.')
        journal_file = '%s.%s.%s' % (value[0], value[1], int(value[2]) + 1)
        self.__curr_file_name = journal_file
        journal_file = os.path.join(self.__path, self.__curr_file_name)
        self.__logger.info(('Container service journal file created :'
            '%(file)s'),
            {'file' : journal_file})
        return journal_file

    def __add_padding(self, pad_len):
        """
        Function to add zero's string to a journal file
        """
        try:
            self.__fd.write('\0' * pad_len)
        except Exception as err:
            raise err

    def __is_checkpoint(self, offset, length):
        """
        Function to check if given offset is of checkpoint type or not
        """
        return (offset + length >= self.__next_checkpoint)

    def __rotate_file(self):
        """
        Rotate the journal file when maximum size is reached
        """
        try:
            self.__fd.sync()
            self.__fd.close()
            self.__prev_file_name = self.__curr_file_name
            self.__fd = FileHandler(self.__calculate_new_name(), 'a+')
            self.__curr_file_name = self.__fd.get_name()
        except Exception as err:
            raise err

    def write_journal(self, record_type, data):
        """
        Function to write to a journal file
        """
        try:
            data = pickle.dumps(data)
            length = len(data) + FIXED_RECORD_SIZE
            curr_offset = self.__fd.tell()
            if self.__is_file_end(curr_offset, length):
                pad_len = MAX_FILE_SIZE - curr_offset
                self.__add_padding(pad_len)
                self.__rotate_file()
                self.__next_checkpoint = CHECKPOINT_MULTIPLIER
                self.__logger.info(('Container service journal'
                    'file created : %(file)s'),
                    {'file' : self.__curr_file_name})
                curr_offset = self.__fd.tell()
            if self.__is_checkpoint(curr_offset, length):
                self.__add_checkpoint(curr_offset)
                self.__next_checkpoint += CHECKPOINT_MULTIPLIER
                curr_offset = self.__fd.tell()
            # update snapshot offsets, if record type is snapshot
            if record_type == RECORD_SNAPSHOT:
                self.__sec_last_offset = self.__last_offset
                self.__last_offset = curr_offset
            # add start record marker
            self.__fd.write(START_MARKER)
            # add len of data
            data_length = len(data)
            pad_len = 4 - len(str(data_length))
            self.__add_padding(pad_len)
            self.__fd.write(str(data_length))
            # write type of record
            self.__fd.write(record_type)
            # write serialized data
            self.__fd.write(data)
            # write end record marker
            self.__fd.write(END_MARKER)
            self.__fd.sync()
            return True
        except Exception as err:
            self.__logger.error('Journal write failed!')
            self.__logger.exception(err)
            return False

    def __is_file_end(self, offset, length):
        """
        Function to check if the eof file is reached or not
        """
        return (offset + length >= MAX_FILE_SIZE)

    def __add_checkpoint(self, offset):
        """
        Function to add checkpoint to journal file
        """
        try:
            pad_len = self.__next_checkpoint - offset
            self.__add_padding(pad_len)
            offset_list = {'last_off' : self.__last_offset, \
                'curr_filename' : self.__curr_file_name, \
                'sec_last_off' : self.__sec_last_offset, \
                'prev_filename' : self.__prev_file_name}
            serialized_data = pickle.dumps(offset_list)
            # add start record marker
            self.__fd.write(START_MARKER)
            # add len of data
            data_length = len(serialized_data)
            pad_len = 4 - len(str(data_length))
            self.__add_padding(pad_len)
            self.__fd.write(str(data_length))
            # write type of record
            self.__fd.write(RECORD_CHECKPOINT)
            # write serialized data
            self.__fd.write(serialized_data)
            # write end record marker
            self.__fd.write(END_MARKER)
            curr_offset = self.__fd.tell()
            pad_len = (self.__next_checkpoint + CHECKPOINT_LEN) - curr_offset
            self.__add_padding(pad_len)
            self.__fd.sync()
        except Exception as err:
            self.__logger.error('checkpoint addition failed!')
            raise err

    def get_list(self, req):
        """
        Creates account updater memory
        Returns dictionary or None
        """
        return self.__recovery.create_list(req)

    def start_recovery(self):
        """
        Initiates container service recovery
        Returns True/False
        """
        try:
            status, offset_list = self.__recovery.recover()
            if not status:
                return False
            if status and offset_list:
                self.__curr_file_name = offset_list['curr_filename']
                self.__prev_file_name = offset_list['prev_filename']
                self.__last_offset = offset_list['last_off']
                self.__sec_last_offset = offset_list['sec_last_off']
                last_offset = self.__fd.get_end_offset()
                last_checkpoint = (last_offset/CHECKPOINT_MULTIPLIER) * \
                    CHECKPOINT_MULTIPLIER
                self.__next_checkpoint =  \
                    last_checkpoint + CHECKPOINT_MULTIPLIER
            return True
        except Exception as err:
            self.__logger.error('Recovery of container service journal failed')
            self.__logger.exception(err)
            return False
