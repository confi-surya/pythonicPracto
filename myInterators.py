class Counter:
    def __init__(self, low, high):
        self.current = low
        self.high = high

    def __iter__(self):
        return self

    def next(self): # Python 3: def __next__(self)
        if self.current > self.high:
            raise StopIteration
        else:
            self.current += 1
            return self.current - 1


class OddNumber:
    def __init__(self,x):
        self.current = 1
        self.maxi=x

    def __iter__(self):
        return self

    def next(self):
       if self.current > self.maxi:
           raise StopIteration
       else:
           self.current +=2
       return self.current - 2

for c in Counter(3, 8):
    print c,

print 
for j in OddNumber(10):
    print j,
