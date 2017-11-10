t=int(raw_input())
while t>=1:
	n,k = raw_input().split(" ")
	n=int(n)
	k=int(k)
	if n==1:
		print k
	else:
		result=k
                m=k
		for j in range(n-1):
			result = (result*(k-1))%(1000000007)
		print result
	t-=1
