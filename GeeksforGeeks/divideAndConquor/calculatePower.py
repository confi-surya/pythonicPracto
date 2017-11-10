def powern_in_orderof_n(x,y):
	if y==0:
		return 1
	elif (y%2==0):
		return powern_in_orderof_n(x,y/2)*powern_in_orderof_n(x,y/2)
	else:
		return x*powern_in_orderof_n(x,y/2)*powern_in_orderof_n(x,y/2)

def power_in_orderof_logn(x,y):
	if y==0:
		return 1
	temp=power_in_orderof_logn(x,y/2)
	if (y%2==0):
		return temp*temp
	else:
		return x*temp*temp

print power_in_orderof_logn(2,200)
