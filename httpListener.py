from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SocketServer import ThreadingMixIn

class ThreadedAccountUpdaterServer(ThreadingMixIn, HTTPServer):
    pass

class A:
	def __init__(self):
		self.a = 4
		self.b = 5

class HttpListener(BaseHTTPRequestHandler):

	def do_fun1(self):
		print "fun1"		
		self.msg = A()
		print self.msg.a

	def do_fun2(self):
		print "fun2"
		print self.msg

servER = ThreadedAccountUpdaterServer(("10.0.4.76",8080),HttpListener)
servER.serve_forever()
