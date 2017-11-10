import os
from eventlet.green import socket

def get_hostname():
	return socket.gethostname()

def main():
	hostname = get_hostname()
	os.system('osd-ring-builder object create')
	os.system('osd-ring-builder object add 10 10 10 20 %s_object-server:127.0.0.1:61008' % hostname)
	os.system('osd-ring-builder container create')
	os.system('osd-ring-builder container add 10 10 10 20 %s_container-server:127.0.0.1:61007 1 %s_container-server' % (hostname, hostname))
	os.system('osd-ring-builder account create')
	os.system('osd-ring-builder account add 10 10 10 20 %s_account-server:127.0.0.1:61006 1 %s_account-server' % (hostname, hostname))

if __name__ == '__main__':
	main()
	
