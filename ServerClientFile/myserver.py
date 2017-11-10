import socket

port = 50000
mysocket = socket.socket()
myhostname = socket.gethostname()
mysocket.bind((myhostname, port))
mysocket.listen(5)
while True:
    print "wating for connect"
    clientconnobject, clientaddr  = mysocket.accept()
    if clientconnobject and clientaddr:
        print "Connected from ----- " , clientaddr
        data = clientconnobject.recv(1024)
        print "Recieved from client ", repr(data)
        with open('mytext.txt', 'rb') as f:
            file_content = f.read(1024)
            while file_content:
                clientconnobject.send(file_content)
                print "Sent data ", repr(file_content)
                file_content = f.read(1024)
            f.close()
        clientconnobject.close()
        print "Done Sending ==== "
       
