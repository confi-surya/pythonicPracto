t=int(raw_input())
n=int(raw_input())
while(t>=1):
    arr = map(int,raw_input().split(" "))
    prefixSum = 0
    suffixSum = 0
    min_index = 0
    min_sum = arr[0]
    j=0
    while j<len(arr):
        if min_sum >= sum(arr[:j]) + sum(arr[j:]):
            min_sum = sum(arr[:j]) + sum(arr[j:])
            min_index = j
        j+=1
    print min_index
    t-=1
