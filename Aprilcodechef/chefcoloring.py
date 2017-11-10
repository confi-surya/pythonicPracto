#from Collections import Counter
from collections import Counter
t=int(raw_input())
while t>=1:
	n=int(raw_input())
	s=raw_input()
	totR = s.count("R")
	totG = s.count("G")
	totB = s.count("B")
	maxVar = max([totR,totG,totB])
	print n - maxVar
	t -= 1
	
	
