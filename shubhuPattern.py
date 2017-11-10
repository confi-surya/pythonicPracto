print "Enter Value n for desired 2^n :"
n=int(raw_input())
generalList = [2**j for j in range(n+1)]
generalList += generalList[::-1][1:]
k=1
for i in generalList:
	for j in range(i):
		if k > 9:
			if k%9==0:
				print "9",
			else:
				print k%9,
		else:
			print k,
		k+=1
	print 

