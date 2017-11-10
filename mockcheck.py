from mocky import D

#from test_mocky1 import Sam

class A:

    def __init__(self):
        self.a = 1
        self.obj = D()
	self.b = self.obj.bar()

    def foo(self, x):
        self.b = self.obj.bar()
        try:
            self.a = self.b/x
	except Exception:
            raise
	return self.a
        
    def bar(self):
#        self.b = self.obj.bar()
        return self.b

