string = raw_input()
charList = []
for j in "".join(string.split(" ")).upper():
	if j not in charList:
		charList.append(j)

if len(charList) == 26:
	print "pangram"
else:
	print "not pangram"
