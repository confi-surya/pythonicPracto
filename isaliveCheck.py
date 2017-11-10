from threading import Thread
import threading
import time

class IsAliveCheck(Thread):
	def __init__(self):
		Thread.__init__(self)

	def run(self):
		print "Running and sleeping for 15 secs"
		time.sleep(15)

aliveObj = IsAliveCheck()
print aliveObj.isAlive()
aliveObj.start()
print aliveObj.isAlive()
aliveObj.join()

