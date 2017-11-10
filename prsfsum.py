t=int(raw_input())
while t>=1:
	n=int(raw_input())
	arr = map(int, raw_input().split(" "))
	totSum = sum(arr)
	j=0
	minindex=1
	temp=0
	minSum = totSum + arr[0]
	while j<n:
		if (totSum+arr[j])<minSum:
			minSum = (totSum+arr[j])
			minindex = j+1
		j+=1
	print minindex
	t-=1
