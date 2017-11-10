import sys
#import pdb
#pdb.set_trace()
n = int(raw_input().strip())
s = raw_input().strip()
k = int(raw_input().strip())
newStr = ''
for j in range(n):
    if s[j].islower() or s[j].isupper():
        if ord(s[j]) + k%26 > ord('z') or ord(s[j]) + k%26 > ord('Z'):
            newStr += chr(ord(s[j]) + k%26 - 26)
        else:
            newStr += chr(ord(s[j]) + k%26)
    else:
        newStr += s[j]
print newStr
