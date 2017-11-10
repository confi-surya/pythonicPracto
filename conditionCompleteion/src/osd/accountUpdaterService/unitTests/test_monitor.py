import mock
import unittest
from osd.accountUpdaterService.unitTests import conf, logger, mockReq, mockStatus, mockResp, FakeRingFileReadHandler, mockServiceInfo
from osd.accountUpdaterService.monitor import GlobalVariables

acc_map = list()
cont_map = list()
obj_map = list()
au_map = list()
acc_id = list()
acc_ip = list()
acc_port = list()
cont_id = list()
cont_ip = list()
cont_port = list()
obj_id = list()
obj_ip = list()
obj_port = list()
au_id = list()
au_ip = list()
au_port = list()

for i in range(512):
    acc_id.append("HN010%s_61006_account" %i)
    cont_id.append("HN010%s_61007_container" %i)
    obj_id.append("HN010%s_61008_object" %i)
    au_id.append("HN010%s_61009_acc-up" %i)
    acc_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    cont_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    obj_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    au_ip.append("%s.%s.%s.%s" %(i, i, i, i))
    acc_port.append("%s%s" %(i, i))
    cont_port.append("%s%s" %(i, i))
    obj_port.append("%s%s" %(i, i))
    au_port.append("%s%s" %(i, i))

    acc_map.append(mockServiceInfo("HN010%s_61006_account"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))
    cont_map.append(mockServiceInfo("HN010%s_61007_container"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))
    obj_map.append(mockServiceInfo("HN010%s_61008_object"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))
    au_map.append(mockServiceInfo("HN010%s_61009_acc-up"%i, "%s.%s.%s.%s"%(i, i, i, i), "%s%s"%(i, i)))

class TestGlobalVariables(unittest.TestCase):
    @mock.patch("osd.accountUpdaterService.monitor.Req", mockReq)
    @mock.patch("osd.accountUpdaterService.monitor.Resp", mockResp)
    @mock.patch("osd.accountUpdaterService.monitor.ServiceInfo", mockServiceInfo)
    def setUp(self):
        self.global_var = GlobalVariables(logger)
        self.global_var.set_service_id("service_id")
    
    def test_load_gl_map(self):
        #self.assertTrue(self.global_var.load_gl_map())
        with mock.patch('osd.accountUpdaterService.unitTests.mockReq.connector', return_value = None):
            self.assertFalse(self.global_var.load_gl_map())
        with mock.patch('osd.accountUpdaterService.unitTests.mockStatus.get_status_code', return_value = "Resp.FAILURE"):
            self.assertFalse(self.global_var.load_gl_map())

    def test_load_ownership(self):
        with mock.patch('osd.accountUpdaterService.unitTests.mockReq.connector', return_value = None):
            self.global_var.load_ownership()
            self.assertEquals(self.global_var.get_ownershipList(), [])
        with mock.patch('osd.accountUpdaterService.unitTests.mockStatus.get_status_code', return_value = "Resp.FAILURE"):
            self.global_var.load_ownership()
            self.assertEquals(self.global_var.get_ownershipList(), [])
        self.global_var.load_ownership()
        self.assertEquals(self.global_var.get_ownershipList(), range(1,513))

    def test_get_account_map(self):
        self.assertEquals([obj.get_id() for obj in self.global_var.get_account_map()], acc_id)
        self.assertEquals([obj.get_ip() for obj in self.global_var.get_account_map()], acc_ip)
        self.assertEquals([obj.get_port() for obj in self.global_var.get_account_map()], acc_port)

    def test_get_container_map(self):
        self.assertEquals([obj.get_id() for obj in self.global_var.get_container_map()], cont_id)
        self.assertEquals([obj.get_ip() for obj in self.global_var.get_container_map()], cont_ip)
        self.assertEquals([obj.get_port() for obj in self.global_var.get_container_map()], cont_port)

    def test_get_acc_updater_map(self):
        self.assertEquals([obj.get_id() for obj in self.global_var.get_acc_updater_map()], au_id)
        self.assertEquals([obj.get_ip() for obj in self.global_var.get_acc_updater_map()], au_ip)
        self.assertEquals([obj.get_port() for obj in self.global_var.get_acc_updater_map()], au_port)

    def test_get_acc_updater_map_version(self):
        self.assertEquals(self.global_var.get_acc_updater_map_version(), "5.0")

    def test_get_global_map_version(self):
        self.assertEquals(self.global_var.get_global_map_version(), "5.0")

    def test_get_service_id(self):
        self.assertEquals(self.global_var.get_service_id(), "service_id")

if __name__ == '__main__':
    unittest.main()
    

    
