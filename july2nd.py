t=int(raw_input())
while t>=1:
    n=int(raw_input())
    elVill = map(int,raw_input().split(" "))
    xcoVill = map(int,raw_input().split(" "))
    mincost = 0
    l=len(elVill)
    k=0
    ini = elVill[0]
    ind1=0
    flag = 0
    for j in range(k,l):
        if ini == 0 and flag ==0:
            ind1 = elVill.index(1)
            mincost += xcoVill[ind1] - xcoVill[ini]
            k=ind1+1
            flag=1
        elif elVill[j] == 0 and flag == 1:
            if 1 in elVill[j:]:
                ind1 = elVill[j:].index(1) + j
                if (xcoVill[j] - xcoVill[j-1]) > (xcoVill[ind1] - xcoVill[j]):
                    mincost += (xcoVill[ind1] - xcoVill[j-1])
                    elVill[j] = 1
                else:
                    mincost += (xcoVill[j] - xcoVill[j-1])
                    elVill[j] = 1
#                k=ind1+1
            else:
                mincost += (xcoVill[-1]-xcoVill[j-1])
                break
        elif elVill[j] == 0 and flag == 2:
            if 1 in elVill[j:]:
                ind1 = elVill[j:].index(1) + j
                if (xcoVill[j] - xcoVill[j-1]) > (xcoVill[ind1] - xcoVill[j]):
                    mincost += (xcoVill[ind1] - xcoVill[j])
                    elVill[j] = 1
                    k=ind1+1
                else:
                    mincost += (xcoVill[j] - xcoVill[j-1])
                    elVill[j] = 1
                    k+=1
#                k=ind1+1
            else:
                mincost += (xcoVill[-1] - xcoVill[j-1])
                break
        else:
            flag=2
            k=j+1
    print mincost
    t-=1

