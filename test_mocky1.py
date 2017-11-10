import unittest
import mock
import test_mocky1
from mock import patch, Mock
from mockcheck import A
from mocky import D

class Myclass:

     def __init__(self):
         self.p = 12
         self.obj  = D()

     def foo(self):
         self.p = 14

     def bar(self):
         return self.p

class Sam:
     def __init__(self):
         pass

     def bar(self):
         return 12

class testmocki(unittest.TestCase):

     def setup(self):
         pass

     #@mock.patch('mocky.D', autospec = True)
     #def test_getter(self):
     #    with patch('mocky.D') as mock:
     #        instance = mock.return_value
     #        instance.bar.return_value = 6
     #        aobj = A()
     #        assert aobj.obj == 6

     #@mock.patch('mockcheck.D', autospec = True)

     @mock.patch('mockcheck.D', autospec = True)
     def test_classA(self, mock1):
         ctmocks = [mock.Mock(), mock.Mock()]
         print ctmocks[0]
         ctmocks[0].bar.return_value = 2
         #ctmocks[1].bar.return_value = 2
         mock1.side_effect = ctmocks
         aobj = A()

         #myobj = Myclass()
         #print aObj1.obj
         #aObj2 = A()
         #aObj.obj.bar()
         #return self.assertEquals(aObj1.obj.bar(), 3)
         #if myobj.bar() == 2:
         #    print "Myclass"

         if aobj.obj.bar() == 2:
             print "class A"
         return self.assertEquals(aobj.obj.bar(), 2)

     '''
     @mock.patch('mockcheck.D', Sam)
     def test_myclass(self):
         #mocky.D = Sam()
         aobj = A()
         print aobj.obj.bar()
         return self.assertEquals(aobj.obj.bar(), 12)
     '''

         #print aobj.obj
         #aobj.obj = Myclass()
         #aobj.obj = mock.MagicMock()

         #aobj.obj.bar = mock.MagicMock(return_value = 6)
         #self.assertEquals(aobj.foo(6), 1)

         #aobj.obj.bar = mock.MagicMock(return_value = 6)
         #self.assertEquals(aobj.foo(3), 2)

         #aobj.obj.bar = mock.MagicMock(return_value = -1)
         #self.assertEquals(aobj.foo(-1), 1)

         #aobj.obj.bar = mock.MagicMock(return_value = 0)
         #self.assertEquals(aobj.foo(-1), 0)

         #aobj.obj.bar = mock.MagicMock(return_value = 1)
         #self.assertRaises(Exception, aobj.foo, 0)

         #aobj.obj.bar = mock.MagicMock(side_effect = ValueError)
         #self.assertRaises(ValueError, aobj.foo, 1)

if __name__ == '__main__':
    unittest.main()

