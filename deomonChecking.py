from threading import Thread
import time

def fun(num):
	time.sleep(15)
	print num

thread_id_list = []
for j in range(15):
	thread_id = Thread(target=fun,args=(j,))
	thread_id_list.append(thread_id)
	thread_id.setDaemon(True)
	thread_id.start()
	print thread_id

for j in thread_id_list:
	j.join()

