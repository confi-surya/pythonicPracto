string = raw_input()
 
found = False
# Write the code to find the required palindrome and then assign the variable 'found' a value of True or False

charDict = {'a':0,'b':0,'c':0,'d':0,'e':0,'f':0,'g':0,'h':0,'i':0,'j':0,'k':0,\
		'l':0,'m':0,'n':0,'o':0,'p':0,'q':0,'r':0,'s':0,'t':0,'u':0,'v':0,\
		'w':0,'x':0,'y':0,'z':0,\
		}

for j in charDict.keys():
	charDict[j] = string.count(j)

count = 0
for j in charDict.values():
	if j%2!=0:
		count += 1

if count > 1:
    print("NO")
else:
    print("YES")

