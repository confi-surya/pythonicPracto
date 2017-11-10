from collections import Counter
n,m=map(int,raw_input().split())
arr=map(int,raw_input().split())
a=map(int,raw_input().split())
b=map(int,raw_input().split())
intsec1 = set(arr).intersection(a)
intsec2 = set(arr).intersection(b)
print len(intsec1)-len(intsec2)

