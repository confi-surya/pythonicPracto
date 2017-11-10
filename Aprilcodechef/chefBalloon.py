t=int(raw_input())
while t>=1:
	r,g,b = map(int, raw_input().split(" "))
	k = int(raw_input())
	sortedRGB = sorted([r,g,b])
	sumUp = 0
	counter = 3
	if sortedRGB[0]<k:
		sumUp += sortedRGB[0]
		counter-=1
		if sortedRGB[1]<k:
			sumUp += sortedRGB[1]
			counter-=1
			if sortedRGB[2]<k:
				sumUp += sortedRGB[2]
				counter-=1
			else:
				sumUp+=(k-1)*counter+1
		else:
			sumUp+=(k-1)*counter+1
		
	else:
		sumUp+=(k-1)*counter+1
	print sumUp
	t -= 1
	
