def factorialfinding(num):
	if num==1:
		return 1
	else:
		return num * factorialfinding(num-1)

print factorialfinding(999)
