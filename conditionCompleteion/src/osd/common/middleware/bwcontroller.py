import time
import eventlet

from osd.common.utils import get_logger, config_true_value
from osd.proxyService.controllers.base import get_account_info
from osd.common.swob import Request


def _sleep(seconds):
    eventlet.sleep(seconds)

class BWInfoCache:
    def __init__(self, threshold, next_reset_time, unit_time_period, logger):
        self.cache = {}
        self.clear = self.cache.clear
        self.threshold = threshold
        self.next_reset_time = next_reset_time
        self.unit_time_period = unit_time_period
        self.logger = logger        
        self.prune_flag = False
        #defining constants to access cache index wise [read_bytes, write_bytes
        #, read_limit, write_limit, current_time, request_count]    
        self.rbytes_index = 0
        self.wbytes_index = 1
        self.rlimit_index = 2
        self.wlimit_index = 3
        self.ctime_index = 4
        self.req_count_index = 5
    
    def _prune(self):
        if len(self.cache) >= self.threshold:
            for key in self.cache.keys():
                if not self.cache[key][self.req_count_index]:
                    del self.cache[key]
                    self.prune_flag = True
            if not self.prune_flag:
                sorted_cache = sorted(self.cache.iteritems(), key=lambda value: \
                                                        value[1][self.ctime_index])
                del self.cache[sorted_cache[0][0]]

    def get(self, key):
        return self.cache.get(key, None)

    def set(self, key, value):
        self._prune()
        self.cache[key] = value
        
    def reset_bytes(self, current_time):
        if current_time >= self.next_reset_time:
            for key in self.cache.keys():
                if not self.cache[key][self.req_count_index]:
                    del self.cache[key]
                else:
                    self.cache[key][self.rbytes_index] = 0
                    self.cache[key][self.wbytes_index] = 0
                    self.cache[key][self.ctime_index] = time.time()
            self.next_reset_time = (self.unit_time_period - (time.time() % self.unit_time_period)) + time.time()
            self.logger.debug('Cache reset completed: nxt reset time is : %s'%self.next_reset_time)
    
    def clear_cache(self):
        self.clear()

    def delete(self, key) :
        self.cache.pop(key, None)
    
    def inc_read_bytes(self, key, delta=0):
        self.cache[key][self.rbytes_index] = self.cache[key][self.rbytes_index] + delta
        self.cache[key][self.ctime_index] = time.time()
    
    def inc_write_bytes(self, key , delta=0):
        self.cache[key][self.wbytes_index] = self.cache[key][self.wbytes_index] + delta
        self.cache[key][self.ctime_index] = time.time()
        
    def get_read_bytes(self, key):
        return self.cache[key][self.rbytes_index]
        
    def get_write_bytes(self, key):
        return self.cache[key][self.wbytes_index]
    
    def add_new_key(self, key, read_limit, write_limit):
        #Setting key with [read_bytes, write_bytes, read_limit, write_limit, current_time, request_count]
        self.set(key, [0, 0, read_limit, write_limit, time.time(), 0])
        self.logger.debug('New key added , account:%s rlimit:%s, wlimit:%s'
                          %(key, read_limit, write_limit))
    
    def get_write_limit(self, key):
        return self.cache[key][self.wlimit_index]
    
    def get_read_limit(self, key):
        return self.cache[key][self.rlimit_index]
    
    def update_read_write_limit(self, key, rlimit, wlimit):
        self.cache[key][self.rlimit_index] = rlimit
        self.cache[key][self.wlimit_index] = wlimit
        self.cache[key][self.ctime_index] = time.time()
        self.logger.debug('Updated r/w limit: read_limit: %s , write_limit: %s'\
                          ' for account %s '%(rlimit, wlimit, key))

    def inc_req_count(self, key):
        self.cache[key][self.req_count_index] += 1
        self.cache[key][self.ctime_index] = time.time()
        self.logger.debug('req_inc operation : Current running request count for account %s is %s '\
                          %(key, self.cache[key][self.req_count_index]))
        

    def dec_req_count(self, key):
        self.cache[key][self.req_count_index] -= 1
        self.cache[key][self.ctime_index] = time.time()
        self.logger.debug('req_dec operation : Current running request count for account %s is %s '\
                          %(key, self.cache[key][self.req_count_index]))

    def get_req_count(self, key):
        return self.cache[key][self.req_count_index]


class BWController:
    def __init__(self, app, conf, logger=None):
        self.app = app
        self.logger = logger or get_logger(conf)
        self.unit_time_period = int(conf.get('bandwidth_controlled_unit_time_period', 60))
        self.bw_control = str(conf.get('bandwidth_control', 'Disable'))
        self.next_unit_time = time.time() + self.unit_time_period
        self.account_limit = 30000 if not int(conf.get('bandwidth_controlled_account_limit', 0)) \
                             else int(conf.get('bandwidth_controlled_account_limit', 30000))
        self.def_max_bandwidth_read_limit = int(conf.get('default_max_bandwidth_read_limit', 0))
        self.def_max_bandwidth_write_limit = int(conf.get('default_max_bandwidth_write_limit', 0))
        #creating the cache with self.account_limit+1024 maximum entries of accounts
        self.cache = BWInfoCache(self.account_limit+1024, self.next_unit_time, \
                                 self.unit_time_period, self.logger)
    
    
    def _need_to_apply_bw_control(self, req, account, container, obj):
        if not self.bw_control == "Enable":
            return False
        if req.method == 'GET' and (self.def_max_bandwidth_read_limit < 0 or \
            self.def_max_bandwidth_read_limit > 10485760):
            self.logger.debug('Value of default_max_bandwidth_read_limit is ' \
                'not in range of 1-10485760 which is in KB/s: %s ,' \
                'therefore discarding bandwidth feature for this request' \
                %self.def_max_bandwidth_read_limit)
            return False
        if req.method == 'PUT' and (self.def_max_bandwidth_write_limit < 0 or \
            self.def_max_bandwidth_write_limit > 10485760):
            self.logger.debug('Value of default_max_bandwidth_write_limit is ' \
                'not in range of 1-10485760 which is in KB/s: %s ,' \
                'therefore discarding bandwidth feature for this request' \
                %self.def_max_bandwidth_write_limit)
            return False
        if self.unit_time_period <= 0 or self.unit_time_period > 60:
            self.logger.debug('Value of bandwidth_controlled_unit_time_period ' \
                'is not in range of 1-60 seconds: %s ,therefore discarding ' \
                'bandwidth feature for this request' %self.unit_time_period)
            return False
        if not obj:
            return False
        if account and container and obj and req.method not in ('PUT', 'GET'):
            return False
        return True
    
        
    def handle_bw_control(self, req, account, container, obj):
        if not self._need_to_apply_bw_control(req, account, container, obj):
            self.logger.debug('Need not to apply BWC for this request '\
                             'Method:%s, account:%s, container:%s, object:%s '\
                              %(req.method, account, container, obj))
            return False
                   
        account_info = get_account_info(req.environ, self.app)
        if not account_info:
            return False
        try:
            max_read_bw = int(account_info['meta'].\
                              get('max-read-bandwidth', '-1').split(',')[-1])
            max_write_bw = int(account_info['meta'].\
                              get('max-write-bandwidth', '-1').split(',')[-1])
        except ValueError:
            return False

        if max_read_bw == -1:
            max_read_bw = self.def_max_bandwidth_read_limit
        if max_write_bw == -1:
            max_write_bw = self.def_max_bandwidth_write_limit

        if max_read_bw == 0 and req.method == 'GET':
            self.logger.debug('Need not to apply BWC for this request '\
                       'Method:%s, account:%s, container:%s, object:%s '\
                       'max_read_bw:%d, default_max_bandwidth_read_limit:%d '\
                       %(req.method, account, container, obj, max_read_bw, \
                       self.def_max_bandwidth_read_limit))
            return False

        if max_write_bw == 0 and req.method == 'PUT':
            self.logger.debug('Need not to apply BWC for this request '\
                       'Method:%s, account:%s, container:%s, object:%s '\
                       'max_write_bw:%d, default_max_bandwidth_write_limit:%d '\
                       %(req.method, account, container, obj, max_write_bw, \
                       self.def_max_bandwidth_read_limit))
            return False

        ##assuming the unit of bandwidth limit is KB/sec and unit_time_period is in seconds
        key = account
        read_limit = max_read_bw * self.unit_time_period * 1024
        write_limit = max_write_bw * self.unit_time_period * 1024
        self.logger.debug('Start applying BWC for this request '\
                       'account_info:%s, max_read_bw:%d, max_write_bw:%d '\
                       %(account_info, max_read_bw, max_write_bw))
       
        ##if account information is not in cache then insert it
        ## otherwise update it, for the case when bw has been changed in memcache
        if self.cache.get(key) is None:
            self.cache.add_new_key(key, read_limit, write_limit)
        else:
            self.cache.update_read_write_limit(key, read_limit, write_limit)
        self.cache.inc_req_count(key)
        input_proxy =  BWControlInputProxy(req.environ.get('wsgi.input'), \
                                           self.cache, key, self.logger)
        req.environ['wsgi.input'] = input_proxy
        return True
                      
            
    def __call__(self, env, start_response):
        req = Request(env)
        try:
            version, account, container, obj = req.split_path(1, 4, True)
        except ValueError:
            return self.app(env, start_response)
        
        def iter_response(iterable):
            key = account
            iterator = iter(iterable)
            try:
                chunk = iterator.next()
                while not chunk:
                    chunk = iterator.next()
            except StopIteration:
                chunk = ''
            
            try:
                while chunk:
                    self.cache.inc_read_bytes(key, delta=len(chunk))
                    yield chunk
                    current_time = time.time()
                    self.cache.reset_bytes(current_time)
                    if self.cache.get_read_bytes(key) >= self.cache.get_read_limit(key):
                        self.logger.debug('Going to sleep for %s seconds '\
                                          'Key:%s, read_bytes:%s, read_limit:%s '\
                                          %(self.cache.next_reset_time - current_time,\
                                            key, self.cache.get_read_bytes(key),\
                                            self.cache.get_read_limit(key)))
                        _sleep(self.cache.next_reset_time - current_time)
                    chunk = iterator.next()
            except GeneratorExit:  # generator was closed before we finished
                raise
            finally:
                self.cache.dec_req_count(key)
            
        bw_control_resp = self.handle_bw_control(req, account, container, obj)
        if not bw_control_resp:
            return self.app(env, start_response)
        else:
            try:
                iterable = self.app(env, start_response)
                return iter_response(iterable)
            except Exception:
                raise
            return iter_response(iterable) 

class BWControlInputProxy:
    def __init__(self, wsgi_input,  cache, account, logger):
        self.wsgi_input = wsgi_input
        self.cache = cache
        self.key = account
        self.logger = logger
        
    def read(self, size):
        current_time = time.time()
        self.cache.reset_bytes(current_time)
        if self.cache.get_write_bytes(self.key) + size >= self.cache.get_write_limit(self.key):
            self.logger.debug('Going to sleep for %s seconds '\
                              'Key:%s, write_bytes:%s, write_limit:%s '\
                              %(self.cache.next_reset_time - current_time,\
                              self.key, self.cache.get_write_bytes(self.key),\
                              self.cache.get_write_limit(self.key)))
            _sleep(self.cache.next_reset_time - current_time)
            self.cache.reset_bytes(time.time())
            
        try:
            chunk = self.wsgi_input.read(size)
        except Exception:
            raise
        self.cache.inc_write_bytes(self.key, delta=len(chunk))
        return chunk 
    
    def readline(self, *args, **kwargs):
        current_time = time.time()
        self.cache.reset_bytes(current_time)
        if self.cache.get_write_bytes(self.key) >= self.cache.get_write_limit(self.key):
            self.logger.debug('Going to sleep for %s seconds '\
                              'Key:%s, write_bytes:%s, write_limit:%s '\
                              %(self.cache.next_reset_time - current_time,\
                              self.key, self.cache.get_write_bytes(self.key),\
                              self.cache.get_write_limit(self.key)))

            _sleep(self.cache.next_reset_time - current_time)
        try:
            line = self.wsgi_input.readline(*args, **kwargs)
        except Exception:
            raise
        self.cache.inc_write_bytes(self.key, delta=len(line))
        return line


def filter_factory(global_conf, **local_conf):
    """
    paste.deploy app factory for creating WSGI proxy apps.
    """
    conf = global_conf.copy()
    conf.update(local_conf)

    def bw_controller(app):
        return BWController(app, conf)
    return bw_controller

