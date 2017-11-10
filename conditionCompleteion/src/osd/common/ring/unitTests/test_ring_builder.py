import mock
import os, sys
import tempfile
import unittest
import cPickle as pickle
from shutil import rmtree
from osd.common.ring import ring_builder

#pylint: disable=C0301
class TestRingBuilder(unittest.TestCase):
    def setUp(self):
        self.ring_builder = ring_builder.RingBuilder()
        ring_builder.fs_prefix = 'test'
        ring_builder.acc_dir_prefix = 'a'
        ring_builder.cont_dir_prefix = 'd'
        ring_builder.obj_dir_prefix = 'o'
        ring_builder.component_prefix = 'x'
        self.test_dir = tempfile.mkdtemp()
        rmtree(self.test_dir, ignore_errors=1)
        os.mkdir(self.test_dir)
        ring_builder.RING_FILE_LOCATION = self.test_dir
    
    def tearDown(self):
        rmtree(self.test_dir, ignore_errors=1)

    def test_fs_id_creation(self):
        self.assertEqual(self.ring_builder.fs_id_creation(2), ['TEST01', 'TEST02'])
        self.assertEqual(self.ring_builder.fs_id_creation(0), [])
      
    def test_account_level_directory_id_creation(self):
        self.assertEqual(self.ring_builder.account_level_directory_id_creation(3), ['a01', 'a02', 'a03'])
        self.assertEqual(self.ring_builder.account_level_directory_id_creation(0), [])

    def test_container_level_directory_id_creation(self):
        self.assertEqual(self.ring_builder.container_level_directory_id_creation(3), ['d01', 'd02', 'd03'])
        self.assertEqual(self.ring_builder.container_level_directory_id_creation(0), [])

    def test_object_level_directory_id_creation(self):
        self.assertEqual(self.ring_builder.object_level_directory_id_creation(3), ['o01', 'o02', 'o03'])
        self.assertEqual(self.ring_builder.object_level_directory_id_creation(0), [])

    def test_service_details_list(self):
        self.assertEqual(self.ring_builder.service_details_list(['id:ip:port']), [{'id': {'ip': 'ip', 'port': 'port'}}])
        self.assertEqual(self.ring_builder.service_details_list(['id:ip:port', 'os:1.2.3:000']), [{'id': {'ip': 'ip', 'port': 'port'}}, {'os': {'ip': '1.2.3', 'port': '000'}}])
        self.assertEqual(self.ring_builder.service_details_list(['os:1.2.3:000']), [{'os': {'ip': '1.2.3', 'port': '000'}}])

    def test_component_id_creation(self):
        self.assertEqual(self.ring_builder.component_id_creation(1), ['x1'])
        self.assertEqual(self.ring_builder.component_id_creation(0), [])
    
    def test_create_distribution_list(self):
        self.assertEqual(self.ring_builder.create_distribution_list(['x0'], 'os'), {'os': ['x0']})
        self.assertEqual(self.ring_builder.create_distribution_list(['x0', 'x3'], 'os'), {'os': ['x0', 'x3']})

    def test_create_ring_file(self):
        self.ring_builder.create_ring_file('object')
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'object.ring')))
        ring_builder.RING_FILE_LOCATION = 'wrong'
        self.assertRaises(IOError, self.ring_builder.create_ring_file, ('wrong'))

    def test_create_object_ring_data(self):
        self.ring_builder.create_ring_file('object')
        self.ring_builder.create_object_ring_data(1, 2, 2, 2, ['os:1.2.3:000'])
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'object.ring')))
        with open(os.path.join(self.test_dir, \
                      'object.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'object_service_list': [{'os': {'ip': '1.2.3', 'port': '000'}}], 'object_level_dir_list': ['o01', 'o02']})

        os.remove(os.path.join(self.test_dir, 'object.ring'))

        self.ring_builder.create_ring_file('object')
        self.ring_builder.create_object_ring_data(1, 2, 2, 2, ['os:1.2.3:000', 'os2:1.2.3:999'])
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'object.ring')))
        with open(os.path.join(self.test_dir, \
                      'object.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'object_service_list': [{'os': {'ip': '1.2.3', 'port': '000'}}, {'os2': {'ip': '1.2.3', 'port': '999'}}], 'object_level_dir_list': ['o01', 'o02']})
        ring_builder.RING_FILE_LOCATION = 'wrong'
        self.assertRaises(IOError, self.ring_builder.create_object_ring_data, 1, 2, 2, 2, ['os:1.2.3:000'])
        
        self.assertRaises(TypeError, self.ring_builder.create_object_ring_data, '1', 2, 2, 2, ['os:1.2.3:000'])
        self.assertRaises(TypeError, self.ring_builder.create_object_ring_data, 1, '2', 2, 2, ['os:1.2.3:000'])
        self.assertRaises(TypeError, self.ring_builder.create_object_ring_data, '1', '2', '2', 2, ['os:1.2.3:000'])
        
         
    def test_create_container_or_account_ring_data(self):
        self.ring_builder.create_ring_file('container')
        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['cs:1.2.3:000'], 1, 'cs', 'container')
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'container.ring')))
        with open(os.path.join(self.test_dir, \
                      'container.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'container_service_list': [{'cs': {'ip': '1.2.3', 'port': '000'}}], 'comp_list' : ['x1'], 'comp_container_dist': {'cs': ['x1']}, 'object_level_dir_list': ['o01', 'o02']}) 

        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['cs:1.2.3:000'], 1, 'cs', 'container')
        with open(os.path.join(self.test_dir, \
                      'container.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'container_service_list': [{'cs': {'ip': '1.2.3', 'port': '000'}}], 'comp_list' : ['x1'], 'comp_container_dist': {'cs': ['x1']}, 'object_level_dir_list': ['o01', 'o02']})

        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['cs:1.2.3:111'], 1, 'cs', 'container')
        with open(os.path.join(self.test_dir, \
                      'container.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'container_service_list': [{'cs': {'ip': '1.2.3', 'port': '111'}}], 'comp_list' : ['x1'], 'comp_container_dist': {'cs': ['x1']}, 'object_level_dir_list': ['o01', 'o02']})
        
        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['cs_01:1.2.3:111'], 1, 'cs', 'container')
        with open(os.path.join(self.test_dir, \
                      'container.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'container_service_list': [{'cs': {'ip': '1.2.3', 'port': '111'}}, {'cs_01': {'ip': '1.2.3', 'port': '111'}}], 'comp_list' : ['x1'], 'comp_container_dist': {'cs': ['x1']}, 'object_level_dir_list': ['o01', 'o02']})

        ring_builder.RING_FILE_LOCATION = 'wrong'
        self.assertRaises(IOError, self.ring_builder.create_container_or_account_ring_data, 1, 2, 2, 2, ['cs:1.2.3:000'], 1, 'cs', 'container')
        
        self.assertRaises(TypeError, self.ring_builder.create_container_or_account_ring_data, '1', 2, 2, 2, ['os:1.2.3:000'], 1, 'os', 'container')
        self.assertRaises(TypeError, self.ring_builder.create_container_or_account_ring_data, 1, '2', 2, 2, ['os:1.2.3:000'], 1, 'os', 'container')
        self.assertRaises(TypeError, self.ring_builder.create_container_or_account_ring_data, '1', '2', 2, 2, ['os:1.2.3:000'], '1', 'os', 'container')
        self.assertRaises(TypeError, self.ring_builder.create_container_or_account_ring_data, 1, 2, '2', 2, ['os:1.2.3:000'], '1', 'os', 'container')
        
        
        ring_builder.RING_FILE_LOCATION = self.test_dir
        self.ring_builder.create_ring_file('account')
        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['as:1.2.3:000', 'as2:0.0.0:000'],  1, 'as2', 'account')

        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'account.ring')))
        with open(os.path.join(self.test_dir, \
                      'account.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'account_service_list': [{'as': {'ip': '1.2.3', 'port': '000'}}, {'as2': {'ip': '0.0.0', 'port': '000'}}], 'comp_list' : ['x1'], 'comp_account_dist': {'as2': ['x1']}, 'object_level_dir_list': ['o01', 'o02']})
        
        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['cs:1.2.3:000'], 1, 'cs', 'account')
        with open(os.path.join(self.test_dir, \
                      'account.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'account_service_list': [{'cs': {'ip': '1.2.3', 'port': '000'}}, {'as2': {'ip': '0.0.0', 'port': '000'}}, {'as': {'ip': '1.2.3', 'port': '000'}}], 'comp_list' : ['x1'], 'comp_account_dist': {'cs': ['x1']}, 'object_level_dir_list': ['o01', 'o02']})

        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['cs:1.2.3:111'], 1, 'cs', 'account')
        with open(os.path.join(self.test_dir, \
                      'account.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data, {'fs_list': ['TEST01'], 'account_level_dir_list': ['a01', 'a02'], 'container_level_dir_list': ['d01', 'd02'], 'account_service_list': [{'cs': {'ip': '1.2.3', 'port': '111'}}, {'as2': {'ip': '0.0.0', 'port': '000'}}, {'as': {'ip': '1.2.3', 'port': '000'}}], 'comp_list' : ['x1'], 'comp_account_dist': {'cs': ['x1']}, 'object_level_dir_list': ['o01', 'o02']})
        


    def test_modify_ring(self):
        self.ring_builder.create_ring_file('account')
        self.ring_builder.create_ring_file('container')
        self.ring_builder.create_ring_file('object')
        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['cs:1.2.3:000'], 1, 'cs', 'container')
        self.ring_builder.create_container_or_account_ring_data(1, 2, 2, 2, ['as:1.2.3:000'], 1, 'as', 'account')
        self.assertRaises(SystemExit, self.ring_builder.modify_ring, 'wrong', 'wrong', 'container')

        self.ring_builder.create_object_ring_data(1, 2, 2, 2, ['cs:1.2.3:000'])
        self.assertRaises(SystemExit, self.ring_builder.modify_ring, 'wrong', 'wrong', 'object')

        self.assertRaises(TypeError, self.ring_builder.modify_ring, 'fs', 1,2, 'object')
        self.assertRaises(TypeError, self.ring_builder.modify_ring, 'acc_dir', 1,2, 'object')
        self.assertRaises(TypeError, self.ring_builder.modify_ring, 'cont_dir', 1,2, 'object')
        self.assertRaises(TypeError, self.ring_builder.modify_ring, 'obj_dir', 1,2, 'object')
        self.assertRaises(TypeError, self.ring_builder.modify_ring, 'comp', 1,2, 'container')
        self.assertRaises(ValueError, self.ring_builder.modify_ring, 'detail', '1:2', 'container')
        self.assertRaises(ValueError, self.ring_builder.modify_ring, 'dist', '1', 'container')

        self.ring_builder.modify_ring('fs', ['test_fs'], 'object')
        with open(os.path.join(self.test_dir, \
                      'object.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['fs_list'], ['test_fs'])

        self.ring_builder.modify_ring('dist', 'cs:test_comp', 'container')
        with open(os.path.join(self.test_dir, \
                      'container.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['comp_container_dist'], {'cs': ['test_comp']})
        self.ring_builder.modify_ring('detail', ['as:0.0.0:123'], 'account')
        with open(os.path.join(self.test_dir, \
                      'account.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['account_service_list'], [{'as': {'ip': '0.0.0', 'port': '123'}}])
        
        self.ring_builder.modify_ring('detail', ['as:0.0.0:123', 'as1:1.2.3:000'], 'account')
        with open(os.path.join(self.test_dir, \
                      'account.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['account_service_list'], [{'as': {'ip': '0.0.0', 'port': '123'}}, {'as1': {'ip': '1.2.3', 'port': '000'}}])

    def test_modify_ring_file_not_exist(self):
        self.assertRaises(IOError, self.ring_builder.modify_ring, 'fs', ['1'], 'container')

    def test_check_data(self):
        self.assertEqual(self.ring_builder.check_data('fs', [1], 'object'), 'fs_list')
        self.assertRaises(TypeError, self.ring_builder.check_data, 'fs', 1, 'object')
        self.assertRaises(ValueError, self.ring_builder.check_data, 'wrong', 1, 'object')


class TestCommands(unittest.TestCase):
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        rmtree(self.test_dir, ignore_errors=1)
        os.mkdir(self.test_dir)
        ring_builder.RING_FILE_LOCATION = self.test_dir
        ring_builder.fs_prefix = 'fs'
        ring_builder.acc_dir_prefix = 'a'
        ring_builder.cont_dir_prefix = 'd'
        ring_builder.obj_dir_prefix = 'c'
        ring_builder.component_prefix = 'x'

    def create_test_ring_file(self):
        object_ring_data = {'fs_list': ['fs1'],
                            'account_level_dir_list': ['a01', 'a02'], 'object_level_dir_list': ['c01', 'c02'], \
                            'container_level_dir_list': ['d01', 'd02'],
                            'object_service_list': [{'os_1': \
                            {'ip': '1.2.3', 'port': '0000'}}]}
        with open(os.path.join(self.test_dir, 'object.ring'), 'wb') as fd:
            fd.write(pickle.dumps(object_ring_data))

    def tearDown(self):
        pass

    def test_unknown(self):
        argv = ["", "object", "a"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)

    def test_errors(self):
        argv = ["", "object"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)

        argv = ["", "wrong", ""]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)

    def test_create(self):
        argv = ["", "object", "create"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'object.ring')))
        self.assertEqual(os.path.getsize(os.path.join(self.test_dir, 'object.ring')), 0)
        ring_builder.RING_FILE_LOCATION = 'wrong'
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)

    def test_add_object(self):
        ring_builder.RING_FILE_LOCATION = self.test_dir
        argv = ["", "object", "create"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertEqual(os.path.getsize(os.path.join(self.test_dir, 'object.ring')), 0)
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'object.ring')))
        argv = ["", "object", "add"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertEqual(os.path.getsize(os.path.join(self.test_dir, 'object.ring')), 0)
        argv = ["", "object", "add", '1', '2', '3', "os:1"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertEqual(os.path.getsize(os.path.join(self.test_dir, 'object.ring')), 0)
        argv = ["", "object", "add", '1', '2', '2', '3', "os:1:222"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'object.ring')))
        self.assertNotEquals(os.path.getsize(os.path.join(self.test_dir, 'object.ring')), 0)
        
        os.remove(os.path.join(self.test_dir, 'object.ring'))
        argv = ["", "object", "create"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        argv = ["", "object", "add", '-f', '1', '-a', '2', '-c', '2', '-o', '2', '-i', 'os', '-p', '1.2.3', '-t', '0000']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'object.ring')))
    
        os.remove(os.path.join(self.test_dir, 'object.ring'))
        argv = ["", "object", "add", '1', '-d', '2', '-i', 'os', '-p', '1.2.3', '-t', '0000']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertFalse(os.path.exists(os.path.join(self.test_dir, 'object.ring')))

    def test_add_container_or_account(self):
        ring_builder.RING_FILE_LOCATION = self.test_dir
        argv = ["", "container", "create"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        argv = ["", "container", "add"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertEquals(os.path.getsize(os.path.join(self.test_dir, 'container.ring')), 0)
        argv = ["", "container", "add", '1', '2', '2', "os:1", "os"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertEquals(os.path.getsize(os.path.join(self.test_dir, 'container.ring')), 0)
        argv = ["", "container", "add", '1', '2', '2', '2', "os:1:222", '1', "os"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'container.ring')))
        
        argv = ["", "container", "add", '1', '2', '2', "os:1:222, os2:1.2.3:000", '1', "os"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'container.ring')))

        os.remove(os.path.join(self.test_dir, 'container.ring'))
        argv = ["", "container", "create"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        argv = ["", "container", "add", '-f', '1', '-a', '2', '-c', '2', '-o', '2', '-i', 'os', '-p', '1.2.3', '-t', '0000', '-x', '1', '-s', 'os']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertNotEquals(os.path.getsize(os.path.join(self.test_dir, 'container.ring')), 0)
        
        argv = ["", "container", "add", '-f', '1', '-a', '2', '-c', '2', '-o', '2', '-i', 'os,os2', '-p', '1.2.3,1.2.3', '-t', '0000,1234', '-x', '1', '-s', 'os']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertTrue(os.path.exists(os.path.join(self.test_dir, 'container.ring')))
    
        os.remove(os.path.join(self.test_dir, 'container.ring'))
        argv = ["", "container", "create"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        argv = ["", "container", "add", "-f", '1', '-a', '2', '-c', '2', '-i', 'os', '-p', '1.2.3', '-t', '0000', '-s', 'os']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertEquals(os.path.getsize(os.path.join(self.test_dir, 'container.ring')), 0)

        argv = ["", "container", "add", '-f', '1', '-a', '2', '-c', '2', '-i', 'os,os2', '-p', '1.2.3', '-t', '0000', '-s', 'os', '-x', '1']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.assertEquals(os.path.getsize(os.path.join(self.test_dir, 'container.ring')), 0)

    def test_modify(self):
        argv = ["", "container", "modify"]
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        argv = ["", "container", "modify", 'wrong']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        argv = ["", "container", "modify", 'wrong_name', 'wrong_value']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        self.create_test_ring_file()    
        argv = ["", "object", "modify", 'fs', 'test9,test10']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        with open(os.path.join(self.test_dir, \
                      'object.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['fs_list'], ['test9', 'test10'])
        os.remove(os.path.join(self.test_dir, 'object.ring'))
        self.create_test_ring_file()    
        argv = ["", "object", "modify", '-n', 'fs', '-v', 'test9,test10']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        with open(os.path.join(self.test_dir, \
                      'object.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['fs_list'], ['test9', 'test10'])
    
        os.remove(os.path.join(self.test_dir, 'object.ring'))
        self.create_test_ring_file()    
        argv = ["", "object", "modify", '-n', 'fs', 'test9,test10']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)

        argv = ["", "object", "modify", '-n', 'detail', '-v', 'os:1.2.3:000,os']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        with open(os.path.join(self.test_dir, \
                      'object.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['object_service_list'], [{'os_1': {'ip': '1.2.3', 'port': '0000'}}])

        argv = ["", "object", "modify", '-n', 'detail', '-v', 'os:1.2.3:000,os1:0.0.0:123']
        with mock.patch('osd.common.ring.ring_builder.read_prefixes'):
            self.assertRaises(SystemExit, ring_builder.main, argv)
        with open(os.path.join(self.test_dir, \
                      'object.ring'), 'rb') as fd:
            data = pickle.loads(fd.read())
        self.assertEqual(data['object_service_list'], [{'os': {'ip': '1.2.3', 'port': '000'}}, {'os1': {'ip': '0.0.0', 'port': '123'}}])

if __name__ == '__main__' :
    unittest.main()
