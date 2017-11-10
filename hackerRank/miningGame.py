string = raw_input()
substrings = lambda x: [x[i:j] for i in range(len(x)) for j in range(i+1, len(x)+1)]
allsubstring = list(set(substrings(string)))
#['A', 'B', 'BA', 'NANA', 'NA', 'NAN', 'AN', 'ANANA', 'ANAN', 'N', 'BANA', 'BAN', 'BANAN', 'BANANA', 'ANA']
stuartsubString = ['B','C','D','F','G','H','I','K','L','M','N','P','Q','R','S','T','V','W','X',\
			'Y','Z']

kevinsubStrings = ['A','E','I','O','U']

kevinScore = 0
stuartScore = 0

for j in kevinsubStrings:
	for k in allsubstring:
		if k.startswith(j):			
			kevinScore = kevinScore + string.count(k)

for j in stuartsubString:
        for k in allsubstring:
                if k.startswith(j):
                        stuartScore = stuartScore + string.count(k)

if kevinScore > stuartScore:
	print "Kevin", kevinScore+1
elif kevinScore == stuartScore:
	print "Draw"
else:
	print "Stuart", stuartScore
