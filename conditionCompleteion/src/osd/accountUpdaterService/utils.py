# Copyright (c) 2010-2012 OpenStack Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import traceback
import pwd
import grp
import gzip, shutil
from logging.handlers import RotatingFileHandler
import logging
import sys
import errno
from datetime import datetime
import datetime
from optparse import OptionParser
from ConfigParser import ConfigParser, RawConfigParser

DEFAULT_LOG_DIR = "/var/log/HYDRAstor/AN/objectStorage"
LOG_FILE_SIZE = 50 * 1024 * 1024
LOG_FILE_NAME = "accountUpdater.log"
LOG_BACKUP_COUNT = 3
LOGGER_NAME = "account-updater"
RECOVERY_FILE_PATH = '/var/run/osd/recovery'


def _gzip(sfn, dfn):
    """
    Compress file in gzip file format
    """ 
    dfo = gzip.open(dfn, 'wb')
    with open(sfn, 'rb') as sfo:
        shutil.copyfileobj(sfo, dfo)
    dfo.close()     
    os.remove(sfn)  

class MyRotatingFileHandler(RotatingFileHandler):
    """
    Just overriding to logging.RotatingFileHandler for gip on rotation of log file.
    """
    def __init__(self, filename, mode='a', maxBytes=0, backupCount=0, encoding=None, delay=0):
        """
        Open the specified file and use it as the stream for logging.
        """
        RotatingFileHandler.__init__(self, filename, mode, maxBytes, backupCount, encoding, delay)
        self.baseFilename = filename
        self.mode = mode
        self.backupCount = backupCount


    def doRollover(self):
        """
        Do a rollover, as described in __init__().
        """

        self.stream.close()
        if self.backupCount > 0:
            for i in range(self.backupCount - 1, 0, -1):
                sfn = "%s.%d.gz" % (self.baseFilename, i)
                dfn = "%s.%d.gz" % (self.baseFilename, i + 1)
                if os.path.exists(sfn):
                    if os.path.exists(dfn):
                        os.remove(dfn)
                    os.rename(sfn, dfn)
            dfn = self.baseFilename + ".1.gz"
            if os.path.exists(dfn):
                os.remove(dfn)
            _gzip(self.baseFilename, dfn)
        self.mode = 'w'
        self.stream = self._open()


def read_conf_dir(parser, conf_dir):
    conf_files = []
    for f in os.listdir(conf_dir):
        if f.endswith('.conf') and not f.startswith('.'):
            conf_files.append(os.path.join(conf_dir, f))
    return parser.read(sorted(conf_files))

def readconf(conf_path, section_name=None, log_name=None, defaults=None,
             raw=False):
    """
    Read config file(s) and return config items as a dict

    :param conf_path: path to config file/directory, or a file-like object
                     (hasattr readline)
    :param section_name: config section to read (will return all sections if
                     not defined)
    :param log_name: name to be used with logging (will use section_name if
                     not defined)
    :param defaults: dict of default values to pre-populate the config with
    :returns: dict of config items
    """
    if defaults is None:
        defaults = {}
    if raw:
        c = RawConfigParser(defaults)
    else:
        c = ConfigParser(defaults)
    if hasattr(conf_path, 'readline'):
        c.readfp(conf_path)
    else:
        if os.path.isdir(conf_path):
            # read all configs in directory
            success = read_conf_dir(c, conf_path)
        else:
            success = c.read(conf_path)
        if not success:
            print _("Unable to read config from %s") % conf_path
            sys.exit(1)
    if section_name:
        if c.has_section(section_name):
            conf = dict(c.items(section_name))
        else:
            print _("Unable to find %s config section in %s") % \
                (section_name, conf_path)
            sys.exit(1)
        if "log_name" not in conf:
            if log_name is not None:
                conf['log_name'] = log_name
            else:
                conf['log_name'] = section_name
    else:
        conf = {}
        for s in c.sections():
            conf.update({s: dict(c.items(s))})
        if 'log_name' not in conf:
            conf['log_name'] = log_name
    conf['__file__'] = conf_path
    return conf


class SingletonType(type):
    """ Class to create singleton classes object"""
    __instance = None

    def __call__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(SingletonType, cls).__call__(*args, **kwargs)
        return cls.__instance


def create_log_file(log_file_path, log_file_name):

    log_file = os.path.join(log_file_path, log_file_name)
    if not os.path.exists(log_file_path):
        os.makedirs(log_file_path)
    if not os.path.exists(log_file):
        os.system("touch %s" %log_file)

def drop_privileges(user):
    """
    Sets the userid/groupid of the current process, get session leader, etc.

    :param user: User name to change privileges to
    """
    if os.geteuid() == 0:
        groups = [g.gr_gid for g in grp.getgrall() if user in g.gr_mem]
        os.setgroups(groups)
    user = pwd.getpwnam(user)
    os.setgid(user[3])
    os.setuid(user[2])
    os.environ['HOME'] = user[5]
    try:
        os.setsid()
    except OSError:
        pass
    os.chdir('/')   # in case you need to rmdir on where you started the daemon
    os.umask(0o22)  # ensure files are created with the correct privileges

class SimpleLogger:
    __metaclass__ = SingletonType

    def __init__(self, conf=None):
        self.__conf = conf
        self.__path = self.__conf.get("log_path", DEFAULT_LOG_DIR)
        self.__prefix = self.__conf.get("log_file", LOG_FILE_NAME)
        self.__size = int(self.__conf.get("log_file_size", LOG_FILE_SIZE))
        self.__backup_count = int(self.__conf.get("log_backup_count", LOG_BACKUP_COUNT))
        self.__prepare()

    def __prepare(self):
        create_log_file(self.__path, self.__prefix)
        self.__log_file = os.path.join(self.__path, self.__prefix)
        self.__logger = logging.getLogger(LOGGER_NAME)
        self.__logger.setLevel(getattr(logging, self.__conf.get('log_level', 'INFO').upper(), logging.debug))
        formatter = logging.Formatter('%(asctime)s : %(levelname)s: %(name)s : %(filename)s: %(lineno)s: %(message)s : %(threadName)s')
        handler = MyRotatingFileHandler(self.__log_file, maxBytes=self.__size, backupCount=self.__backup_count)
        handler.setFormatter(formatter)
        self.__logger.addHandler(handler)

    def get_logger_object(self):
        return self.__logger

    def get_conf(self):
        return self.__conf

class DirtyLogger:
    def __init__(self, logger = None):
        self.__logger = logger or SimpleLogger()

    def write(self, value):
        value = value.strip()
        if value:
            stack = traceback.extract_stack()[-2]
            self.__logger.info("STDOUT: <%s:%s> %s" %(stack[0].split("/")[-1], stack[1], value))

    def writelines(self, values):
        self.__logger.info("STDOUT: %s" %'\n'.join(values))

    def close(self):
        pass

    def flush(self):
        pass

    def __iter__(self):
        return self

    def next(self):
        self.__logger.error("Bad file descriptor")
        raise IOError(errno.EBADF, 'Bad file descriptor')

    def read(self, size=-1):
        self.__logger.error("Bad file descriptor")
        raise IOError(errno.EBADF, 'Bad file descriptor')

    def readline(self, size=-1):
        self.__logger.error("Bad file descriptor")
        raise IOError(errno.EBADF, 'Bad file descriptor')

    def tell(self):
        return 0

    def xreadlines(self):
        return self

def parse_options(parser=None, once=False, test_args=None):
    """
    Parse standard osd server/daemon options with optparse.OptionParser.

    :param parser: OptionParser to use. If not sent one will be created.
    :param once: Boolean indicating the "once" option is available
    :param test_args: Override sys.argv; used in testing

    :returns : Tuple of (config, options); config is an absolute path to the
               config file, options is the parser options as a dictionary.

    :raises SystemExit: First arg (CONFIG) is required, file must exist
    """
    if not parser:
        parser = OptionParser(usage="%prog CONFIG [options]")
    parser.add_option("-v", "--verbose", default=False, action="store_true",
                      help="log to console")
    parser.add_option('-l', '--llport', action="store",
                      help="local leader port", dest="llport")
    parser.add_option('-C', '--clean_journal', action="store_true",
                     help="clean journals in startup", dest="clean_journal") 

    if once:
        parser.add_option("-o", "--once", default=False, action="store_true",
                          help="only run one pass of daemon")

    # if test_args is None, optparse will use sys.argv[:1]
    options, args = parser.parse_args(args=test_args)

    if not args:
        parser.print_usage()
        print _("Error: missing config path argument")
        sys.exit(1)
    config = os.path.abspath(args.pop(0))
    if not os.path.exists(config):
        parser.print_usage()
        print _("Error: unable to locate %s") % config
        sys.exit(1)

    extra_args = []
    # if any named options appear in remaining args, set the option to True
    for arg in args:
        if arg in options.__dict__:
            setattr(options, arg, True)
        else:
            extra_args.append(arg)

    options = vars(options)
    if extra_args:
        options['extra_args'] = extra_args
    return config, options

"""
Below function return timeDifference
"""

def respondsecondsDiff(left_file):
    year = int(left_file.split('.')[0].split('-')[0])
    month = int(left_file.split('.')[0].split('-')[1])
    day = int(left_file.split('.')[0].split('-')[2])
    hour = int(left_file.split('.')[1].split(':')[0])
    minute = int(left_file.split('.')[1].split(':')[1])
    second = int(left_file.split('.')[1].split(':')[2])
    file_datetime_Object = datetime.datetime(year, month, day, hour, minute, second)
    current_datetime_Object = datetime.datetime.now()
    time_difference = current_datetime_Object - file_datetime_Object
    return time_difference.days * 86400 + time_difference.seconds

def mkdirs(path):
    """
    Ensures the path is a directory or makes it if not. Errors if the path
    exists but is a file or on permissions failure.

    :param path: path to create
    """
    if not os.path.isdir(path):
        try:
            os.makedirs(path)
        except OSError as err:
            if err.errno != errno.EEXIST or not os.path.isdir(path):
                raise

def remove_file(path):
    """Quiet wrapper for os.unlink, OSErrors are suppressed

    :param path: first and only argument passed to os.unlink
    """
    try:
        os.unlink(path)
    except OSError:
        pass

def remove_recovery_file(server_name):
    """Removes recovery file
    : param server_name: service name like object-server
    """     
    recovery_file = os.path.join(RECOVERY_FILE_PATH, server_name)
    remove_file(recovery_file)

def create_recovery_file(server_name):
    """Creates recovery file
    : param server_name: service name like object-server
    """
    recovery_file = os.path.join(RECOVERY_FILE_PATH, server_name)
    try:
        if not os.path.exists(RECOVERY_FILE_PATH):
            mkdirs(RECOVERY_FILE_PATH)
        open(recovery_file, 'w').close()
    except (OSError, IOError):
        raise

filename = 1

def mergesorting(tempStructure,firstIndex,lastIndex):
    midIndex = ( firstIndex + lastIndex ) / 2
    if firstIndex < lastIndex:
        mergesorting( tempStructure, firstIndex, midIndex )
        mergesorting( tempStructure, midIndex + 1, lastIndex )
    a, first, last = 0, firstIndex, midIndex + 1
    tmp = [None] * ( lastIndex - firstIndex + 1 )
    while first <= midIndex and last <= lastIndex:
        if tempStructure[first].split("/")[filename] < tempStructure[last].split("/")[filename] :
            tmp[a] = tempStructure[first]
            first += 1
        else:
            tmp[a] = tempStructure[last]
            last += 1
        a += 1
    if first <= midIndex :
        tmp[a:] = tempStructure[first:midIndex + 1]
    if last <= lastIndex:
        tmp[a:] = tempStructure[last:lastIndex + 1]
    a = 0
    while firstIndex <= lastIndex:
        tempStructure[firstIndex] = tmp[a]
        firstIndex += 1
        a += 1
