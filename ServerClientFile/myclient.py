import socket

myport = 50000
mysocket = socket.socket()

myhostname = socket.gethostname()

mysocket.connect((myhostname,myport))
mysocket.send('Hello server -----')
outputfile = 'outputfile.txt'

with open('outputfile.txt','wb') as f:
    while True:
        data = mysocket.recv(1024)
        if not data:
            break
        print "data received " , data
        f.write(data)
    f.close()
    print "Successfully got the file"
mysocket.close()
print "connection closed"
