from collections import Counter
def validString(str1):
	count = 0
	mapy = []
	mappy = {}
	for j in list(set(str1)):
		mapy.append(str1.count(j))
	count = Counter(mapy)
	if len(count.most_common()) > 1 and count.most_common()[1][1]>1:
		return "NO"
	else:
		return "YES"
string1 = raw_input()
print validString(string1)	


