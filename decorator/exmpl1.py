print "======== 1 ========="
def greet(name):
   return "Hello " + name 

def call_func(func):
    other_name = "John"
    return func(other_name)  

print call_func(greet)

print "======== 2 ========="

def compose_greet_func():
    def get_message():
        return "Hello there!"

    return get_message

greet = compose_greet_func()
print greet()

print "======== 3 CLOSURE Property of decorator ========"

def get_text(name):
   #return "lorem ipsum, {0} dolor sit amet".format(name)
   return " Harry " + name

def p_decorate(func):
   def func_wrapper(name):
       #return "<p>{0}</p>".format(func(name))
       return func(name)
   return func_wrapper

my_get_text = p_decorate(get_text)

print my_get_text("John Karry")
