x=int(input())
n=int(input())

def powersum(x, power, num):
   value = x-pow(num, power)
   if value < 0:
       return 0
   elif value ==0:
       return 1
   else:
       return powersum(value, power, num+1) + powersum(x, power, num+1)

print(powersum(x,n,1))
