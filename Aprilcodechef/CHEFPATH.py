t=int(raw_input())
while t>=1:
	m,n = map(int,raw_input().split(" "))
	if (m%2==0 and n%2!=0) or (m%2!=0 and n%2==0):
		if abs(m-n)%2!=0 and abs(m-n) >=3 and (m==1 or n==1):
			print "No"
		elif abs(m-n)%2!=0 and m!=1 and n!=1:
			print "Yes"
		else:
			print "Yes"
	if m%2==0 and n%2==0:
		print "Yes"
	if m%2!=0 and n%2!=0:
		print "No"
	t-=1

