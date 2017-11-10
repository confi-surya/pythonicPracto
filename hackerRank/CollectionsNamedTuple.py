n=int(raw_input())
a,b,c,d=map(str, raw_input().split(" "))
if a=='MARKS':
	x=0
if b=='MARKS':
	x=1
if c=='MARKS':
	x=2
if b=='MARKS':
	x=3

sum1=0

for j in range(n):
	a=map(str, raw_input().split(" "))
	sum1+=int(a[x])
	del(a)

print sum1/n
