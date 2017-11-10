from threading import Thread
import time
def fun1(num):
	time.sleep(3)
	print "Fun1 %s" %num

def fun(num):
	thread_idList = []
	for j in range(5):
		thread_id = Thread(target=fun1,args=(j,))
		thread_idList.append(thread_id)
		thread_id.start()
	for j in thread_idList:
		j.join()
	time.sleep(2)
	print "Fun %s" %num

threadIdList=[]
for j in range(5):
	threadId = Thread(target=fun,args=(j,))
	threadIdList.append(threadId)
	threadId.start()

for j  in threadIdList:
	j.join()
