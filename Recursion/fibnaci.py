def fibo(n):
        i=0
	if n==0:
		return 0
	elif n==1:
		return 1
	else:
		return fibo(n-2) + fibo(n-1)

for j in range(30):
	print fibo(j)
