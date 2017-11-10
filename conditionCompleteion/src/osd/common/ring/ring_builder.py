import os
import ConfigParser
import cPickle as pickle
from sys import argv as sys_argv, exit
from osd.common.ring.utils import parse_args, _build_add_arguments_from_opts, \
    _build_modify_arguments_from_opts 

RING_FILE_LOCATION = '/export/osd_meta_config'
OSD_DIR = '/opt/HYDRAstor/objectStorage/configFiles'
EXIT_SUCCESS = 0
EXIT_WARNING = 1
EXIT_ERROR = 2
fs_prefix = None
acc_dir_prefix = None
cont_dir_prefix = None
obj_dir_prefix = None
component_prefix = None

#pylint: disable=E0211, E0213

def read_prefixes(prefix_conf):
    """
    Read prefixes from config file
    :param prefix_conf: path of prefix configuration file
    """
    global fs_prefix, acc_dir_prefix, cont_dir_prefix, obj_dir_prefix, component_prefix
    config = ConfigParser.ConfigParser()
    config.read(prefix_conf)
    fs_prefix = config.get('prefixes', 'filesystem')
    acc_dir_prefix = config.get('prefixes', 'account_level_dir')
    cont_dir_prefix = config.get('prefixes', 'container_level_dir')
    obj_dir_prefix = config.get('prefixes', 'object_level_dir')
    component_prefix = config.get('prefixes', 'component')

def _parse_add_object_values(argvish):
    """ 
    Parse add object arguments specified on the command line.
    Will exit on error.
    """

    opts, args = parse_args(argvish)

    opts_used = opts.fs_no or opts.acc_dir_no or opts.cont_dir_no or \
        opts.obj_dir_no or opts.ip or opts.port or opts.service_id

    if len(args) > 0 and opts_used:
        print Commands.add_object.__doc__.strip()
        exit(EXIT_ERROR)
    elif not opts_used:
        if len(args) < 5 or len(args[4].split(':')) < 3:
            print Commands.add_object.__doc__.strip()
            exit(EXIT_ERROR)
        else:
            parsed_args = {}
            try:
                fs_no, acc_dir_no, cont_dir_no, obj_dir_no, service_details = args
                service_details = service_details.split(',')
            except ValueError as err:
                print 'ERROR while parsing options %s' % err
                exit(EXIT_ERROR)
            parsed_args.update({'fs_no': fs_no, 'acc_dir_no': acc_dir_no, \
                                'cont_dir_no': cont_dir_no, \
                                'obj_dir_no': obj_dir_no, \
                                'service_details': service_details})
    else:
        try:
            parsed_args = _build_add_arguments_from_opts(opts, 'add_object')
        except ValueError as err:
            print 'ERROR while parsing options %s' % err
            exit(EXIT_ERROR)
    return parsed_args


def _parse_add_container_or_account_values(argvish):
    """ 
    Parse add  account/container arguments specified on the command line.
    Will exit on error.
    """

    opts, args = parse_args(argvish)

    opts_used = opts.fs_no or opts.acc_dir_no or opts.cont_dir_no or \
        opts.obj_dir_no or opts.ip or opts.port or \
        opts.service_id or opts.comp_no

    if len(args) > 0 and opts_used:
        print Commands.add_container_or_account.__doc__.strip()
        exit(EXIT_ERROR)
    elif not opts_used:
        if len(args) < 6 or len(args[4].split(':')) < 3:
            print Commands.add_container_or_account.__doc__.strip()
            exit(EXIT_ERROR)
        else:
            parsed_args = {}
            fs_no, acc_dir_no, cont_dir_no, obj_dir_no, service_details, comp_no, current = args
            service_details = service_details.split(',')
            parsed_args.update({'fs_no': fs_no, 'acc_dir_no': acc_dir_no, \
                                'cont_dir_no': cont_dir_no, \
                                'comp_no': comp_no, 'current': current, \
                                'obj_dir_no': obj_dir_no, \
                                'service_details': service_details})
    else:
        try:
            parsed_args = _build_add_arguments_from_opts(opts)
        except (ValueError) as err:
            print 'ERROR while parsing options %s' % err
            exit(EXIT_ERROR)
    return parsed_args

def _parse_modify_values(argvish):
    """ 
    Parse modify arguments specified on the command line.
    Will exit on error.
    """

    opts, args = parse_args(argvish)

    opts_used = opts.name or opts.value

    if len(args) > 0 and opts_used:
        print Commands.modify.__doc__.strip()
        exit(EXIT_ERROR)
    elif not opts_used:
        if len(args) < 2:
            print Commands.modify.__doc__.strip()
            exit(EXIT_ERROR)
        else:
            parsed_args = {}
            try:
                name, value = args
                if name in ['fs', 'acc_dir', 'cont_dir', 'obj_dir', 'comp', 'detail']:
                    value = value.split(',')
            except ValueError as err:
                print 'ERROR while parsing options %s' % err
                exit(EXIT_ERROR)
            parsed_args.update({'name': name, 'value': value})
    else:
        try:
            parsed_args = _build_modify_arguments_from_opts(opts)
            if parsed_args['name'] in ['fs', 'acc_dir', 'cont_dir', 'obj_dir', 'comp', 'detail']:
                parsed_args['value'] = parsed_args['value'].split(',')
        except ValueError as err:
            print 'ERROR while parsing options %s' % err
            exit(EXIT_ERROR)
    
    return parsed_args


class RingBuilder:
    def __init__(self):
        pass

    def fs_id_creation(self, fs_no):
        """
        Does file system id creation
        :param fs_no: no of filesystem
        :returns: list of filesystem ids
        """
        fs_list = []
        for  i in range(1, fs_no + 1):
            fs_list.append(fs_prefix.upper() + '%02d' % i)
        return fs_list
    
    def account_level_directory_id_creation(self, dir_no):
        """
        Does account level directory id creation
        :param dir_no: no of directories
        :returns: list of directory ids
        """
        dir_list = []
        for i in range(1, dir_no + 1):
            dir_list.append(acc_dir_prefix + '%02d' % i)
        return dir_list
            
    def container_level_directory_id_creation(self, dir_no):
        """
        Does container level directory id creation
        :param dir_no: no of directories
        :returns: list of directory ids
        """
        dir_list = []
        for i in range(1, dir_no + 1):
            dir_list.append(cont_dir_prefix + '%02d' % i)
        return dir_list

    def object_level_directory_id_creation(self, dir_no):
        """
        Does object level directory id creation
        :param dir_no: no of directories
        :returns: list of directory ids
        """
        dir_list = []
        for i in range(1, dir_no + 1):
            dir_list.append(obj_dir_prefix + '%02d' % i)
        return dir_list

    def service_details_list(self, service_details):
        """
        Does service id creation
        :param service_details: string containing service details
        :returns: list containing service details
        """
        service_list = []
        for service in service_details:
            service_id, ip, port = service.split(':')
            service_list.append({service_id: {'ip': ip, 'port': port}})
        return service_list

    def component_id_creation(self, comp_no):
        """
        Does component id creation
        :param comp_no: no of components
        :returns: list of component ids
        """
        comp_list = []
        for i in range(1, comp_no + 1):
            comp_list.append(component_prefix + `i`)
        return comp_list

    def create_distribution_list(self, component_list, service_id):
        """
        Creates distribution list
        :param component_list: list of components
        :param service_id: service id
        :returns: list containing distribution
        """
        # TODO
        # Currently only one component is available.
        # So distribution is made in a simple way.
        # In future, when there will me multiple components
        # some algorithm needs to be implemented to distribute
        # components among services.
        return {service_id: component_list}

    def create_ring_file(self, ring_name):
        """
        Creates ring file
        :param ring_name: name of the ring
        """
        try:
            ring_fd = open(os.path.join(RING_FILE_LOCATION,
                           ring_name + '.ring'), 'a')
        except (IOError, OSError, Exception) as err:
            raise err

    def create_object_ring_data(self, fs_no, acc_dir_no, cont_dir_no, \
                                obj_dir_no, service_details):
        """
        Creates object ring data
        :param fs_no: no of filesystem
        :param acc_dir_no: no of account level directories
        :param cont_dir_no: no of container level directories
        :param obj_dir_no: no of object level directories
        :param service_details: string containing service details
        """
        if not isinstance(fs_no, int) or not isinstance(acc_dir_no, int) \
            or not isinstance(cont_dir_no, int) or \
            not isinstance(obj_dir_no, int):
            raise TypeError('Filesystem no or directory no must be an integer')
        fs_list = self.fs_id_creation(fs_no)
        acc_dir_list = self.account_level_directory_id_creation(acc_dir_no)
        cont_dir_list = self.container_level_directory_id_creation(cont_dir_no)
        obj_dir_list = self.object_level_directory_id_creation(obj_dir_no)
        service_details_list = self.service_details_list(service_details)
        object_ring_data = {'fs_list': fs_list,
                            'account_level_dir_list': acc_dir_list, 
                            'container_level_dir_list': cont_dir_list,
                            'object_level_dir_list': obj_dir_list,
                            'object_service_list': service_details_list}

        # write data into object.ring file
        try:
            with open(os.path.join(RING_FILE_LOCATION, \
                      'object.ring'), 'rb+') as fd:
                data = fd.read()
                if data:
                    data = pickle.loads(data)
                    old_data = {}
                    new_data = {}
                    for detail in data['object_service_list']:
                        old_data.update(detail)
                    for detail in service_details_list:
                        new_data.update(detail)
                    old_data.update(new_data)
                    object_ring_data = data
                    data['object_service_list'] = [{key:value} for key, value \
                                                   in old_data.iteritems()]
                    object_ring_data = data
                fd.seek(0)
                fd.truncate(0)
                fd.write(pickle.dumps(object_ring_data))

        except (IOError, OSError, Exception) as err:
            raise err

    def create_container_or_account_ring_data(self, fs_no, acc_dir_no, \
        cont_dir_no, obj_dir_no, service_details, comp_no, current, ring_name):
        """
        Creates account or container ring data
        :param fs_no: no of filesystem
        :param acc_dir_no: no of account level directories
        :param cont_dir_no: no of container level directories
        :param obj_dir_no: no of object level directories
        :param service_details: string containing service details
        :param comp_no: no of components
        :param current: currently running service id
        """
        if not isinstance(fs_no, int) or not isinstance(acc_dir_no, int) or \
            not isinstance(cont_dir_no, int) or not \
            isinstance(obj_dir_no, int) or not isinstance(comp_no, int):
            raise TypeError('Filesystem no or directory no or component no'
                            ' must be an integer')
        fs_list = self.fs_id_creation(fs_no)
        acc_dir_list = self.account_level_directory_id_creation(acc_dir_no)
        cont_dir_list = self.container_level_directory_id_creation(cont_dir_no)
        obj_dir_list = self.object_level_directory_id_creation(obj_dir_no)
        service_details_list = self.service_details_list(service_details)
        comp_list = self.component_id_creation(comp_no)
        comp_dist = self.create_distribution_list(\
                              comp_list, current)
        ring_data = None
        if ring_name == 'container':
            ring_data = {'fs_list': fs_list,
                         'account_level_dir_list': acc_dir_list, 
                         'container_level_dir_list': cont_dir_list,
                         'object_level_dir_list': obj_dir_list,
                         'container_service_list' : service_details_list,
                         'comp_list' : comp_list,
                         'comp_container_dist': comp_dist}
        else:
            ring_data = {'fs_list': fs_list,
                         'account_level_dir_list': acc_dir_list, 
                         'container_level_dir_list': cont_dir_list,
                         'object_level_dir_list': obj_dir_list,
                         'account_service_list' : service_details_list,
                         'comp_list' : comp_list,
                         'comp_account_dist': comp_dist}

        # write data into container.ring/account.ring file
        try:
            with open(os.path.join(RING_FILE_LOCATION, \
                      ring_name + '.ring'), 'rb+') as fd:
                data = fd.read()
                if data:
                    data = pickle.loads(data)
                    old_data = {}
                    new_data = {}
                    for detail in data['%s_service_list' % ring_name]:
                        old_data.update(detail)
                    for detail in service_details_list:
                        new_data.update(detail)
                    old_data.update(new_data)
                    if ring_name == 'container':
                        data['container_service_list'] = [{key:value} for \
                            key, value in old_data.iteritems()]
                        data['comp_container_dist'] = comp_dist
                        ring_data = data
                    else:
                        data['account_service_list'] = [{key:value} for \
                            key, value in old_data.iteritems()]
                        data['comp_account_dist'] = comp_dist
                        ring_data = data
                fd.seek(0)
                fd.truncate(0)
                fd.write(pickle.dumps(ring_data))
        except (IOError, OSError, Exception) as err:
            raise err

    def modify_ring(self, name, value, ring_name):
        """
        Modifies ring data
        :param name: attribute name whose value is to be changed
        :param value: value of the attribute
        :param ring_name: name of the ring
        """
        def get_dist_data(value):
            value = value.split(':')
            return {value[0]: value[1].split(',')}

        try:
            check_attribute = {'object': lambda name: name in ['fs', \
                                   'acc_dir', 'cont_dir', 'obj_dir', 'detail'], \
                               'account': lambda name : name in ['fs', \
                                   'acc_dir', 'cont_dir', 'obj_dir', \
                                   'detail', 'comp', 'dist'], \
                               'container': lambda name : name in ['fs', \
                                   'acc_dir', 'cont_dir', 'obj_dir', \
                                   'detail', 'comp', 'dist']}
            if not check_attribute[ring_name](name):
                print 'ERROR enter valid attribute name'
                exit(EXIT_ERROR)
            name = self.check_data(name, value, ring_name)
            if 'dist' in name:
                value = get_dist_data(value)
            if 'service' in name:
                for i in value:
                    if not len(i.split(':')) == 3:
                        print 'Attribute "detail" must be in format <id:ip:port>' 
                        exit(EXIT_ERROR)
                value = self.service_details_list(value)
        except (KeyError, ValueError, TypeError, Exception) as err:
            raise err
        try:
            with open(os.path.join(RING_FILE_LOCATION, \
                         ring_name + '.ring'), 'rb+') as fd:
                data = pickle.loads(fd.read())
                data[name] = value
                fd.seek(0)
                fd.truncate(0)
                fd.write(pickle.dumps(data))
        except OSError as err:
            raise err
        except KeyError as err:
            raise err 
        except Exception as err:
            raise err
    
    def check_data(self, name, value, ring_name):
        """
        Verifies ring data attributes
        :param name: attribute name whose value is to be changed
        :param value: value of the attribute
        :param ring_name: name of the ring
        """
        check_data = {'fs': (lambda attribute: isinstance(attribute, list), \
                             TypeError('Attribute "fs" must be a list'), 'fs_list'), \
                      'acc_dir': (lambda attribute: isinstance(attribute, list), \
                             TypeError('Attribute "acc_dir" must be a list'), 'account_level_dir_list'), \
                      'cont_dir': (lambda attribute: isinstance(attribute, list), \
                             TypeError('Attribute "cont_dir" must be a list'), 'container_level_dir_list'), \
                      'obj_dir': (lambda attribute: isinstance(attribute, list), \
                             TypeError('Attribute "obj_dir" must be a list'), 'object_level_dir_list'), \
                      'comp': (lambda attribute: isinstance(attribute, list), \
                             TypeError('Attribute "comp" must be a list'), 'comp_list'), \
                      'detail': (lambda attribute: isinstance(attribute, list), \
                                 ValueError('Attribute "detail" must be in '
                                            ' format <id:ip:port>'), '%s_service_list' % ring_name), \
                      'dist': (lambda attribute: len(attribute.split(':')) == 2, \
                              ValueError('Attribute "dist" must be in '
                                         ' format <id:comp_list>'), 'comp_%s_dist' % ring_name)}
        try:
            if not check_data[name][0](value):
                raise check_data[name][1]
            else:
                return check_data[name][2]
        except KeyError:
            raise ValueError('Account/Container/Object Ring attribute must be one'
                             ' of ["fs", "acc_dir", "cont_dir", "obj_dir", "detail",'
                             ' "comp", "dist"]')  
        

class Commands(object):
    def unknown():
        print 'Unknown command: %s' % argv[2]
        exit(EXIT_ERROR)
  

    def create():
        """
osd-ring-builder create <ring_name>
    Creates ring file for specified ring name.
        """
        try:
            RingBuilder().create_ring_file(argv[1])
        except (OSError, Exception) as err:
            print 'ERROR while creating ring file: %s' % err 
            exit(EXIT_ERROR)
        exit(EXIT_SUCCESS)

    def add():
        if argv[1] == 'object':
            Commands.add_object.im_func()
        elif argv[1] == 'container':
            Commands.add_container_or_account.im_func('container')
        else:
            Commands.add_container_or_account.im_func('account')


    def add_object():
        """
osd-ring-builder <ring_name> add <no_of_filesystem> <no_of_account_level_directories> <no_of_container_level_directories> <no_of_object_level_directories> <service_id:ip:port>

or

osd-ring-builder <ring_name> add
    --fs-no <no_of_filesystem>
    --acc-dir-no <no_of_account_level_directories>
    --cont-dir-no <no_of_container_level_directories>
    --obj-dir-no <no_of_object_level_directories>
    --id <service_id> --ip <ip> --port <port>
    Adds data to the ring with the given information.
        """
        arguments = _parse_add_object_values(argv[3:])
        try:
            RingBuilder().create_object_ring_data( \
                          int(arguments['fs_no']),
                          int(arguments['acc_dir_no']),
                          int(arguments['cont_dir_no']),
                          int(arguments['obj_dir_no']),
                          arguments['service_details'])
        except (TypeError, OSError, Exception) as err:
            print 'ERROR During object ring creation %s' % err
            exit(EXIT_ERROR)
        exit(EXIT_SUCCESS)


    def add_container_or_account(ring_name):
        """
osd-ring-builder <ring_name> add <no_of_filesystem> <no_of_account_level_directories> <no_of_container_level_directories> <no_of_object_level_directories> <service_id:ip:port> <no_of_components>

or

osd-ring-builder <ring_name> add
    --fs-no <no_of_filesystem> 
    --acc-dir-no <no_of_account_level_directories>
    --cont-dir-no <no_of_container_level_directories>
    --obj-dir-no <no_of_object_level_directories>
    --id <service_id> --ip <ip> --port <port> --comp-no <no_of_components>
    Adds data to the ring with the given information.
        """
        arguments = _parse_add_container_or_account_values(argv[3:])
        try:
            ring_data = RingBuilder().create_container_or_account_ring_data( \
                                   int(arguments['fs_no']), \
                                   int(arguments['acc_dir_no']), \
                                   int(arguments['cont_dir_no']), \
                                   int(arguments['obj_dir_no']), \
                                   arguments['service_details'], \
                                   int(arguments['comp_no']), \
                                   arguments['current'], ring_name)
        except (TypeError, OSError, Exception) as err:
            print 'ERROR During %s ring creation %s' % (ring_name, err)
            exit(EXIT_ERROR)
        exit(EXIT_SUCCESS)

    def modify():
        """
osd-ring-builder <ring_name> modify <attribute_name> <new_value>

or

osd-ring-builder <ring_name> modify
    --name <attribute_name> --value <new_value>

Object Ring Attributes:
    fs:		To change filesystem list.
    acc_dir:	To change account level directory list.
    cont_dir:	To change container level directory list.
    obj_dir:	To change object level directory list.
    detail:	To change service details.

Eg:
    osd-ring-builder object modify fs fs1
    osd-ring-builder object modify acc_dir a1,a2
    osd-ring-builder object modify cont_dir c1
    osd-ring-builder object modify obj_dir o2
    osd-ring-builder object modify detail id:ip:port

Account/Container Ring Attributes:
    fs:		To change filesystem list.
    acc_dir:	To change account level directory list.
    cont_dir:	To change container level directory list.
    obj_dir:	To change object level directory list.
    detail:	To change service details.
    comp:	To change component list.
    dist:	To change distribution list.

Eg:
    osd-ring-builder account modify fs fs1,fs2
    osd-ring-builder account modify acc_dir a1
    osd-ring-builder account modify cont_dir c1
    osd-ring-builder account modify obj_dir o1
    osd-ring-builder account modify detail id:ip:port
    osd-ring-builder account modify comp x0
    osd-ring-builder account modify dist id:component

Note: All the values must be a complete list.
      The existing value will be replaced with new value given.
        """
        arguments = _parse_modify_values(argv[3:])
        
        try:
            if argv[1] == 'object':
                RingBuilder().modify_ring(arguments['name'], arguments['value'], 'object')
            elif argv[1] == 'container':
                RingBuilder().modify_ring(arguments['name'], arguments['value'], 'container')
            else:
                RingBuilder().modify_ring(arguments['name'], arguments['value'], 'account')
        except (ValueError, TypeError, OSError, Exception) as err:
            print 'ERROR while modifying ring file: %s' % err
            exit(EXIT_ERROR)
        exit(EXIT_SUCCESS)


def main(arguments=None):
    global argv
    if arguments:
        argv = arguments
    else:
        argv = sys_argv

    if len(argv) < 3:
        print "osd-ring-builder\n" % \
              globals()
        cmds = [c for c, f in Commands.__dict__.iteritems()
                if f.__doc__ and c[0] != '_']
        cmds.sort()
        for cmd in cmds:
            print Commands.__dict__[cmd].__doc__.strip()
            print
        print('Exit codes: 0 = operation successful\n'
              '            1 = operation completed with warnings\n'
              '            2 = error')
        exit(EXIT_SUCCESS)
    
    if argv[1] not in('account', 'container', 'object'):
        print 'Invalid ring name: %s' % argv[1]
        exit(EXIT_ERROR)

    command = argv[2]
    read_prefixes(os.path.join(OSD_DIR, 'prefix.conf'))
    Commands.__dict__.get(command, Commands.unknown.im_func)()

if __name__ == '__main__':
    main() 
