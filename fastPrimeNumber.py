import math
import time
def primenumbercheck(n):
    if n==1:
        return False
    elif n==2:
        return True
    elif n>2 and n%2==0:
        return False
    else:
        for j in range(3, 1+ math.floor(math.sqrt(n))):
            if n%j == 0:
                return False
        return True

t1=time.time()
for j in range(1,100):
    print(j, primenumbercheck(j))
t2=time.time()
print(t2-t1)
