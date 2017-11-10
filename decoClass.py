class NullDecl (object):
   def __init__ (self, func):
      self.func = func
      for name in set(dir(func)) - set(dir(self)):
        setattr(self, name, getattr(func, name))

   def __call__ (self, *args):
      return self.func (*args)

@NullDecl
def myFunc (x,y,z):
   return (x+y)/z

