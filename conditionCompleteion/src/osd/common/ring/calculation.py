import struct

class Calculation(object):
    """
    Provides methods for calculation used by Ring
    """
    @classmethod
    def evaluate(cls, key, shift_param):
        """
        Evaluates directory or service id based on hash of the path
        :param key: the key of the path
        :param shift_param: the no. to which the integer value of the key must be shifted
                            to generate the desired value.
                            shift_param totally depends on the no. of directories, file system,
                            container and account services.
        :returns: directory no or container or account service id

        For example:
            key: b7400056a32e449d4df25a238da8e5e1 (string: '/account/container/obj')
            shift_param: 10 (If no. of directories is 10)

            Then, evaluate(key, shift_param) returns 8.
        """
        return int(key[:8], 16) % shift_param + 1

    @classmethod
    def distribute(cls, service_id_list, component_list):
        """
        Distributes components amongst services

        :param service_id_list: Service ID list for which components need to be distributed
        :param component_list: Components to be distributed
        :returns: dictionary containing distributed components amongst services
        """
        i = 0
        counter = 0
        ring_data = {}

        while(True):
            jumps = len(component_list)/len(service_id_list)

            try:
                ring_data[service_id_list[i]].append(component_list[counter])
                counter += 1
                i = (i, i+1)[counter % jumps == 0]
            except KeyError:
                ring_data[service_id_list[i]] = []
            except IndexError:
                i -= 1
                ring_data[service_id_list[i]].extend(component_list[counter:])
                break
        return ring_data
