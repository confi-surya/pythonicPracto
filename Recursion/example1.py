def findingSum(li):
	if not li:
		return 0
	else:
		return li[0] + findingSum(li[1:]) # Leaving first element

print findingSum([1,2,3,4,5])

