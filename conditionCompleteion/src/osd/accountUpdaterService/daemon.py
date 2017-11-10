import os
import sys
import signal
import time
from re import sub
import traceback
from osd.accountUpdaterService.utils import SimpleLogger, drop_privileges, readconf, DirtyLogger


class Daemon:
    def __init__(self, conf, logger):
        self.__conf = conf
        self.__logger = logger

    def run_once(self, *args, **kargs):
        raise NotImplementedError("run_once not implemented in the child class")

    def run_forever(self, *args, **kargs):
        raise NotImplementedError("run_forever not implemented in the child class")

    def run(self, once=False, **kwargs):
        drop_privileges(self.__conf.get('user', 'root'))

        fd = os.open("/dev/null", os.O_RDWR)
        stdio = [sys.stdin, sys.stdout, sys.stderr]
        for f in stdio:
            try:
                f.flush()
            except IOError:
                self.__logger.warning("Got IO Error while performing flush on fd: %s" %f.fileno())
                pass
            try:
                os.dup2(fd, f.fileno())
            except OSError:
                self.__logger.warning("Got OS Error while performing dup2 on fd: %s" %f.fileno())
                pass

        sys.stdout = DirtyLogger(self.__logger)
        sys.stderr = DirtyLogger(self.__logger)

        sys.excepthook = lambda * exc_info: \
            self.__logger.critical("UNCAUGHT EXCEPTION: %s%s%s" %(exc_info[1], "".join(traceback.format_tb(exc_info[2])), exc_info[1]))

        def kill_children(signum, stack):
            self.__logger.info("recieved signal : %s" %signum)
            signal.signal(signal.SIGTERM, signal.SIG_DFL)
            os.killpg(0, signal.SIGTERM)
            sys.exit(130)

        signal.signal(signal.SIGTERM, kill_children)
        if once:
            self.run_once(**kwargs)
        else:
            self.run_forever(**kwargs)

def run_daemon(server, conf, section_name="", **kwargs):
    if section_name is '':
        section_name = sub(r'([a-z])([A-Z])', r'\1-\2',
            server.__name__).lower()
    conf = readconf(conf, section_name,
        log_name=kwargs.get('log_name'))

    logger = SimpleLogger(conf).get_logger_object()
    os.environ['TZ'] = time.strftime("%z", time.gmtime())

    try:
        server(conf, logger).run()
    except KeyboardInterrupt:
        logger.info('User quit')
    logger.info('Exited')
