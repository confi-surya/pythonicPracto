import mock
import unittest

class ClassToPatch():
   def __init__(self, *args):
       pass

   def some_func(self):
       return id(self)

class UUT():
    def __init__(self, *args):
        resource_1 = ClassToPatch()
        resource_2 = ClassToPatch()
        print resource_1.some_func()
        self.test_property = (resource_1.some_func(), resource_2.some_func())

class TestCase1(unittest.TestCase):
    @mock.patch('__main__.ClassToPatch', autospec = True)
    def test_1(self, mock1):
        ctpMocks = [mock.Mock(), mock.Mock()]
        ctpMocks[0].some_func.return_value = "funky"
        ctpMocks[1].some_func.return_value = "monkey"
        mock1.side_effect = ctpMocks
        u = UUT()
        self.assertEqual(u.test_property, ("funky", "monkey"))

if __name__ == '__main__':
    unittest.main()

