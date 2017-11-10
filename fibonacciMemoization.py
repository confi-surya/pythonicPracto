from functools import lru_cache

fibbo_Cache = {}

def fibbo(n):
    if n in fibbo_Cache:
        return fibbo_Cache[n] 
    if n<0:
        raise ValueError("fibbo function takes positive integer")
    if type(n) != int:
        raise TypeError("fibbo takes positive integer argument")
    if n==0:
        x = 0
    if n==1:
        x = 1
    elif n==2:
        x = 2
    elif n>2:
        x = fibbo(n-1) + fibbo(n-2)
    fibbo_Cache[n] = x
    return x


@lru_cache(maxsize=1000)
def fibonacci(n):
    if n==1:
        return 1
    if n==2:
        return 1
    if n>2:
        return fibonacci(n-1) + fibonacci(n-2)

#for j in range(1,100):
#    print j,": ", fibbo(j)


for j in range(1000):
    print (j,": ",fibonacci(j))
