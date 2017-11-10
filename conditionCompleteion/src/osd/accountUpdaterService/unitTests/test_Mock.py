class HttpReponse:
     def __init__(self):
         self.status = 200
         pass

class HTTPConnection:
     def __init__(self, ip, port):
         pass
     def request(self, ar1, ar2, ar3, ar4):
         pass 
     def getresponse(self):
         return HttpReponse()

def exit(status):
    print "In exit status"
    return 0
