from optparse import OptionParser
import sys

def parse_args(argvish):
    """
    Build OptionParser and evaluate command line arguments.
    """
    parser = OptionParser()
    parser.add_option(
            '-f', '--fs-no',
            help="Pass no. of file system", dest='fs_no')
    parser.add_option(
            '-a', '--acc-dir-no',
            help="Pass no. of account level directories", dest='acc_dir_no')
    parser.add_option(
            '-c', '--cont-dir-no',
            help="Pass no. of container level directories", dest='cont_dir_no')
    parser.add_option(
            '-o', '--obj-dir-no',
            help="Pass no. of object level directories", dest='obj_dir_no')
    parser.add_option(
            '-x', '--comp-no',
            help="Pass no. of components", dest='comp_no')
    parser.add_option(
            '-i', '--id',
            help="Pass service id", dest='service_id')
    parser.add_option(
            '-p', '--ip',
            help="Pass service ip", dest='ip')
    parser.add_option(
            '-t', '--port',
            help="Pass service port", dest='port')
    parser.add_option(
            '-s', '--service-id',
            help="Pass current running service id", dest='current')
    parser.add_option(
            '-n', '--name',
            help="Pass name of ring attribute to be changed", dest='name')
    parser.add_option(
            '-v', '--value',
            help="Pass value of ring attribute to be changed", dest='value')
    return parser.parse_args(argvish)

def _build_add_arguments_from_opts(opts, method=None):
    """ 
    Convert optparse stype options into a arguments dictionary.
    """
    add_object_option_list = (['fs_no', '-f', '--fs-no'],
                              ['acc_dir_no', '-a', '--acc-dir-no'],
                              ['cont_dir_no', '-c', '--cont-dir-no'],
                              ['obj_dir_no', '-c', '--obj-dir-no'],
                              ['service_id', '-i', '--id'],
                              ['ip', '-p', '--ip'],
                              ['port', '-t', '--port'])
    add_account_container_option_list = (['fs_no', '-f', '--fs-no'],
                                         ['acc_dir_no', '-a', '--acc-dir-no'],
                                         ['cont_dir_no', '-c', '--cont_dir-no'],
                                         ['obj_dir_no', '-c', '--obj-dir-no'],
                                         ['comp_no', '-x', '--comp-no'],
                                         ['service_id', '-i', '--id'],
                                         ['current', '-s', '--service-id'],
                                         ['ip', '-p', '--ip'],
                                         ['port', '-t', '--port'])
    if method == 'add_object':
        option_list = add_object_option_list
    else:
        option_list = add_account_container_option_list
    for attribute, shortopt, longopt in option_list:

        if not getattr(opts, attribute, None):
            raise ValueError('Required argument %s/%s not specified.' %
                             (shortopt, longopt))
    service_ids = opts.service_id.split(',')
    ips = opts.ip.split(',')
    ports = opts.port.split(',')
    service_details = []
    if not len(service_ids) == len(ips) == len(ports):
        print 'ERROR please enter complete details for service'
        sys.exit(1)
    for i in range(len(service_ids)):
        service_details.append(':'.join([service_ids[i], ips[i], ports[i]]))
      
    return {'fs_no': opts.fs_no, 'acc_dir_no': opts.acc_dir_no, \
            'cont_dir_no': opts.cont_dir_no, \
            'obj_dir_no': opts.obj_dir_no, \
            'service_details': service_details, \
            'comp_no': opts.comp_no, 'current': opts.current}


def _build_modify_arguments_from_opts(opts):
    """ 
    Convert optparse stype options into a arguments dictionary.
    """
    for attribute, shortopt, longopt in (['name', '-n', '--name'],
                                         ['value', '-v', '--value']):

        if not getattr(opts, attribute, None):
            raise ValueError('Required argument %s/%s not specified.' %
                             (shortopt, longopt))


    return {'name': opts.name, 'value': opts.value}

