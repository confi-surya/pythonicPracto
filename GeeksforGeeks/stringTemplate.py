# A Simple Python templaye example
from string import Template
import time
# Create a template that has placeholder for value of x
t = Template('x is $x')
# Substitute value of x in above template
time.sleep(120)
print (t.substitute({'x' : 1}))

