import unittest
from osd.common.middleware.bwcontroller import \
    BWInfoCache, BWController, BWControlInputProxy, _sleep
from osd.objectService.unitTests import FakeLogger, debug_logger
from osd.common.swob import Request
import mock

def _get_account_info(*args, **kwargs):
    return {}

def _get_account_info1(*args, **kwargs):
    return {'meta':{}}

def _get_account_info2(*args, **kwargs):
    return {'meta':{'max-read-bandwidth': '0', 'max-write-bandwidth': '0'}}

def _get_account_info3(*args, **kwargs):
    return {'meta':{'max-read-bandwidth':'100', 'max-write-bandwidth':'100'}}

class TestBWCache(unittest.TestCase):
    def setUp(self):
        self.cache = BWInfoCache(2, 123, 60, FakeLogger())
        self.cache.add_new_key('a', 10, 10)
    def tearDown(self):
        self.cache.clear_cache()
    
    def test_key_deletee_after_reset(self):
        self.cache.reset_bytes(124)
        self.assertEqual(self.cache.get('a'), None)
    def test_rw_bytes_after_zero(self):
        self.cache.inc_req_count('a')
        self.cache.inc_read_bytes('a', delta=10)
        self.cache.inc_write_bytes('a', delta=10)
        self.cache.reset_bytes(124)
        self.assertEqual(self.cache.get_read_bytes('a'), 0)
        self.assertEqual(self.cache.get_write_bytes('a'), 0)
    def test_prune_zero_request_count(self):
        self.cache.add_new_key('b', 20, 20)
        self.cache._prune()
        self.assertTrue(not self.cache.get('a'))
        self.assertTrue(not self.cache.get('b'))
        self.assertTrue(self.cache.prune_flag)
    def test_prune_non_zero_count(self):
        self.cache.add_new_key('b', 20, 20)
        self.cache.inc_req_count('a')
        self.cache.inc_req_count('b')
        self.cache._prune()
        self.assertTrue(not self.cache.get('a'))
        self.assertTrue(self.cache.get('b'))
        self.assertTrue(not self.cache.prune_flag)

class TestBWController(unittest.TestCase):
    def setUp(self):
        class FakeApp:
            def __init__(self):
                pass
        conf = {'bandwidth_controlled_unit_time_period': 60, 'bandwidth_control': 'Enable',
                'bandwidth_controlled_account_limit': 1024, 'default_max_bandwidth_read_limit': 100,
                'default_max_bandwidth_write_limit': 100,
               }
        app = FakeApp()
        self.controller = BWController(app, conf, FakeLogger(''))
      
    def tearDown(self):
        self.controller.cache.clear_cache()

    def test_need_to_apply_bw_control_method_head(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'HEAD'})
        self.assertFalse(self.controller._need_to_apply_bw_control(req, 'a1', 'c1', 'o1'))
    
    def test_need_to_apply_bw_control_method_delete(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'DELETE'})
        self.assertFalse(self.controller._need_to_apply_bw_control(req, 'a1', 'c1', 'o1'))

    def test_need_to_apply_bw_control_method_post(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'POST'})
        self.assertFalse(self.controller._need_to_apply_bw_control(req, 'a1', 'c1', 'o1'))

    def test_need_to_apply_bw_control_object_none(self):
        req = Request.blank('/a1/c1/', environ={'REQUEST_METHOD': 'POST'})
        self.assertFalse(self.controller._need_to_apply_bw_control(req, 'a1', 'c1', None))

    def test_need_to_apply_bw_control_obj_cont_none(self):
        req = Request.blank('/a1', environ={'REQUEST_METHOD': 'POST'})
        self.assertFalse(self.controller._need_to_apply_bw_control(req, 'a1', None, None))

    def test_need_to_apply_bw_control_obj_get(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'GET'})
        self.assertTrue(self.controller._need_to_apply_bw_control(req, 'a1', 'c1', 'o1'))

    def test_need_to_apply_bw_control_obj_put(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'PUT'})
        self.assertTrue(self.controller._need_to_apply_bw_control(req, 'a1', 'c1', 'o1'))
        
        
    def test_handle_bw_control(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'HEAD'})
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))

    @mock.patch("osd.common.middleware.bwcontroller.get_account_info", _get_account_info)
    def test_handle_bw_control_empty_meta(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'PUT'})
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_read_limit = 0
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_write_limit = 0
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))

    @mock.patch("osd.common.middleware.bwcontroller.get_account_info", _get_account_info1)
    def test_handle_bw_control_empty_limit(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'PUT'})
        self.assertTrue(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_write_limit = 0
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_read_limit = 0
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))

    @mock.patch("osd.common.middleware.bwcontroller.get_account_info", _get_account_info2)
    def test_handle_bw_control_limit_zero(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'PUT'})
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_read_limit = 0
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_write_limit = 0
        self.assertFalse(self.controller.handle_bw_control(req, 'a', 'c', 'o'))

    @mock.patch("osd.common.middleware.bwcontroller.get_account_info", _get_account_info3)
    def test_handle_bw_control_limit_non_zero(self):
        req = Request.blank('/a1/c1/o1', environ={'REQUEST_METHOD': 'PUT'})
        self.assertTrue(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_read_limit = 0
        self.assertTrue(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.controller.def_max_bandwidth_write_limit = 0
        self.assertTrue(self.controller.handle_bw_control(req, 'a', 'c', 'o'))
        self.assertTrue(bool(self.controller.cache.get_read_limit('a')))


if __name__ == '__main__':
    unittest.main()
 
