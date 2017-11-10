import netaddr
from ConfigParser import ConfigParser
from osd.common import utils, swob

REQUEST = swob
 
class BucketFilter(object):
    def __init__(self, conf):
        self.logger = utils.get_logger(conf, log_route='ceilometer')

    def execute(self, env):
        try:
            path_info = env['PATH_INFO']
            if path_info.split('/')[3] == '$Policies$':
                return True
        except:
            pass
        return False

class RequestFilter(object):
    def __init__(self, conf):
        self.logger = utils.get_logger(conf, log_route='ceilometer')

    def execute(self, env):
        try:
            req = REQUEST.Request(env)
            version, account, container, obj = utils.split_path(req.path, 2, 4, True)
            if (obj is None or (obj and req.method not in ['GET', 'PUT'])):
                return True
        except:
            pass
        return False


class IPfilter(object):
    def __init__(self, conf):
        self.logger = utils.get_logger(conf, log_route='ceilometer')
        parser = ConfigParser()
        parser.read(conf.get('ip_filter_config_path', '/opt/HYDRAstor/objectStorage/configFiles/osd_ip_filter.conf'))
        self.ips_list = [ip.strip() for ip in parser.get('ip_filter', 'internal_ip_list').split('\n')]
        self.subnet_list = [subnet.strip() for subnet in parser.get('ip_filter', 'internal_skip_list').split('\n')]
        self.ip_headers = [header.strip() for header in parser.get('ip_filter', 'ip_headers').split('\n')]

    def get_ip(self, env):
        ip = None
        for ip_header in self.ip_headers:
            try:
                ip = env[ip_header].split(',')[-1].strip()
                try:
                    netaddr.IPAddress(ip)
                except netaddr.AddrFormatError:
                    continue
                else:
                    return ip
            except KeyError:
                pass
        return ip

    def execute(self, env):
        ip = self.get_ip(env)
        if ip:
            if ip in self.ips_list:
                return True

            for internal_subnet in self.subnet_list:
                try:
                    if netaddr.IPAddress(ip) in netaddr.IPNetwork(internal_subnet):
                        return True
                except netaddr.AddrFormatError:
                    pass

        return False

class OSDCeilometerFilterMiddleWare(object):
 
    def __init__(self, app, conf):
        self.app = app
        self.filters = [IPfilter(conf), RequestFilter(conf), BucketFilter(conf)]
        self.logger = utils.get_logger(conf, log_route='ceilometer')
 
    def __call__(self, env, start_response):
        for osd_filter in self.filters:
            if osd_filter.execute(env):
                env['ceilometer-skip'] = True
                break
        return self.app(env, start_response)
 

def filter_factory(global_conf, **local_conf):
    conf = global_conf.copy()
    conf.update(local_conf)
 
    def osd_ceilometer_filter(app):
        return OSDCeilometerFilterMiddleWare(app, conf)
    return osd_ceilometer_filter
