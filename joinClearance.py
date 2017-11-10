from threading import Thread
import time

def fun(num):
	time.sleep(1)
	print num

thread_list=list()

for j in range(15):
	th_id = Thread(target=fun,args=(j,))
#	th_id.get_ident()
	th_id.start()
	thread_list.append(th_id)
#	th_id.get_ident()
#	th_id.join()

#print thread_list

for j in thread_list:
	print "j %s"%j
	j.join()
