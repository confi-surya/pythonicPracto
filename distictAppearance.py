from collections import OrderedDict
n=int(raw_input())
d=OrderedDict()
l=[]

while n>=1:
	s=raw_input()
	l.append(s)
	d[s]='A'
	n-=1

print len(set(l))
for j in d.keys():
	print l.count(j),

