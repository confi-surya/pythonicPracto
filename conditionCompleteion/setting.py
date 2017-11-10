from threading import Lock, Condition, Thread
import time
lock_object = Lock()
cond_object = Condition()

X = 2

finishedCond_object = Condition()

flag = False

class A:
	def __init__(self):
		self.a=1
		self.b=2

	def seta(self, value):
		lock_object.acquire()
		try:
			self.a = value
			#time.sleep(2)
		finally:
			lock_object.release()
			cond_object.notify()
                flag = True 

	def setb(self, value):
		lock_object.acquire()
		try:
			self.b = value
			#time.sleep(2)
		finally:
			lock_object.release()
			cond_object.notify()
		flag = True

	def geta(self):
		lock_object.acquire()
		try:
			return self.a
		finally:
			lock_object.release()

	def getb(self):
		lock_object.acquire()
		try:
			return self.b
		finally:
			lock_object.release()

a=A()

def thread_func(num):
	cond_object.acquire()
	a.seta(num+1)
	if not flag:
		cond_object.wait(3)
	print "Notified for %s" %num
	cond_object.release()
        print gettingAVariable()

def gettingAVariable():
	return a.geta(),a.getb()

if __name__ == "__main__":
	thread_list_id = []
	for j in range(100):
		thread_id = Thread(target=thread_func,args=(j,))
		thread_list_id.append(thread_id)
		thread_id.start()

	for threadId in thread_list_id:
		threadId.join()


