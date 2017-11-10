from collections import Counter
l=raw_input()
c=Counter(l)
k=c.most_common()
count=0
if k[0][1] == k[1][1] and k[2][1]==k[1][1]:
	count+=2
if count==2:
	p=sorted([str(k[0][0]),str(k[1][0]),str(k[2][0])])
	print "%s %s"%(p[0],k[0][1])
	print "%s %s"%(p[1],k[0][1])
	print "%s %s"%(p[2],k[0][1])
else:
	print "%s %s"%(k[0][0],k[0][1])
	print "%s %s"%(k[1][0],k[1][1])
	print "%s %s"%(k[2][0],k[2][1])

