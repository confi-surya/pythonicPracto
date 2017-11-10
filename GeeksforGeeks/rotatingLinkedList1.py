class Node:
	def __init__(self, data):
		self.data = data
		self.next = None

class Mylist:
	def __init__(self):
		self.head = None
		self.temp= self.head 

	def push(self,data):
		newNode = Node(data)
		if self.head:
			self.temp.next = newNode
			self.temp = newNode
		else:
			self.head = newNode
			self.head.next = None
			self.temp = self.head

	def displayinfo(self):
		temp=self.head
		while temp:
			print temp.data
			temp = temp.next

	def rotateList(self,kunit):
		temp1 = self.head
		while temp1:
			temp1 = temp1.next
		temp1 = self.head
		temp = self.head
		for k in range(kunit-1):
			temp = temp.next
		self.head = temp.next
		temp.next = None
		temp = self.head
		print "List After Rotation"
		while temp:
			print temp.data
			temp = temp.next

ll = Mylist()
ll.push(2)
ll.push(3)
ll.push(4)
ll.push(5)
ll.push(6)

ll.displayinfo()		
ll.rotateList(2)			
