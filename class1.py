from mock import Mock

class Foo:

   def __init__(self):
       self._x = 12
       self._y = 10

   def getX(self):
       return self._x

   def getY(self):
       return self._y

object_to_beMock = Foo()

class Foo(object_to_beMock):
    # instance properties
    _fooValue = 123
    
    def callFoo(self):
        print "Foo:callFoo_"
    
    def doFoo(self, argValue):
        print "Foo:doFoo:input = ", argValue
 
# creating the mock object
fooObj = Foo()

print fooObj
mockFoo = Mock(return_value = fooObj)

print mockFoo
mockObj = mockFoo()

print mockObj._fooValue

