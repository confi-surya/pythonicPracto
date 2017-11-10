x=int(raw_input())
while x>=1:
	string = raw_input()
	charDict = {'a':1,'b':2,'c':3,'d':4,'e':5,'f':6,'g':7,'h':8,'i':9,'j':10,'k':11,\
                'l':12,'m':13,'n':14,'o':15,'p':16,'q':17,'r':18,'s':19,'t':20,'u':21,'v':22,\
                'w':23,'x':24,'y':25,'z':26\
                }
	reverseString = string[::-1]
	flag = False
	for j in range(1,len(string)-1):
		sDiff = charDict[string[j]] - charDict[string[j-1]]
		rDiff = charDict[reverseString[j]] - charDict[reverseString[j-1]]
		if abs(sDiff) == abs(rDiff):
			flag = True
			pass
		else:
			print "Not Funny"
			flag = False
			break 
	if flag:
		print "Funny"
	x -= 1
