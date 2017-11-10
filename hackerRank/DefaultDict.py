from collections import defaultdict
n,m = raw_input().split(" ")
n=int(n)
m=int(m)
d=defaultdict(list)
count = 0
while n>=1:
	c=raw_input()
	count += 1
	d[c].append(count)
	n-=1

while m>=1:
	c=raw_input()
	if c in d.keys():
		print " ".join(map(str,d[c]))
	else:
		print "-1"
	m-=1

