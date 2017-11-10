str1='abccdeef'
str2='cdccfee'

k1=len(str1)
k2=len(str2)

mapy = []
county = 0
for j in str1:
	if j in str2 and j not in mapy:
		county = county + min(str1.count(j),str2.count(j))
		mapy.append(j)

print k1+k2-2*county

