t=int(raw_input())
while t>=1:
	n=int(raw_input())
	electVill = map(int,raw_input())
	coordVill = map(int,raw_input())
	totCost = 0
	k=len(electVill)
	for j in range(k):
		if electVill[j] == 0:
			
