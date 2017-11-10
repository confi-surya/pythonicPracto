class Stack:
	def __init__(self):
		self.top = -1
		self.stackSize = 5
		self.stack = []

	def push(self,num):
		if len(self.stack) >= self.stackSize:
			print "Stack Over Flow"
		else:
			self.top = self.top + 1
			self.stack.append(num)
	def pop(self):
		if not self.stack:
			print "Stack is empty"
		else:
			item = self.stack[self.top]
			self.stack.remove(item)
			self.top = self.top - 1
			return item

stack = Stack()
stack.push(12)
stack.push(13)
stack.push(14)
stack.push(15)
stack.push(16)
stack.push(17)
print stack.pop()
print stack.pop()
print stack.pop()
print stack.pop()
print stack.pop()
print stack.pop()

