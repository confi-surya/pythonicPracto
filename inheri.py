class A:
    def __init__(self):
        print "A 's init"
    def funA(self,x):
        print "class A function", x
    def funBB(self):
        print "class A B function"
 
class B(A):
    def __init__(self):
        A.__init__(self)
        print "B 's init"
    def funAA(self):
        print "B is A function"
    def funB(self):
        print "B's B function"

class c(B):
    def __init__(self):
        B.__init__(self)
        print "c 's init"
    def funA(self):
        print "C's A function"
    def funA(self, x):
        print "C's A function",x
    def funB(self):
        print "c's B function"
    def funB(self):
        print "c's B function"
cobj = c()
cobj.funA(4)
cobj.funB()
