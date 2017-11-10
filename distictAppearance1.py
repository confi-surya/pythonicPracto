from collections import deque
from collections import Counter
n=int(raw_input())
d=deque()
while n>=1:
        s=raw_input()
        d.append(s)
        n-=1
print len(set(d))
c=Counter(d)

