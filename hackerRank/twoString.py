t=int(raw_input())
while t>=1:
	str1 = raw_input()
	str2 = raw_input()
	flag=False
	for j in str1:
		if j in str2:
			flag = True
	t -= 1
	if flag:
		print "YES"
	else:
		print "NO"

