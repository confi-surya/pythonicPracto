# PythonDecorators/entry_exit_function.py
def entry_exit(f):
    def new_f():
        print("Entering", f.__name__)
        f()
        print("Exited", f.__name__)
        print
    return new_f

@entry_exit
def func1():
    print("inside func1()")

@entry_exit
def func2():
    print("inside func2()")

@entry_exit
def func3():
    print("inside func3()")

func1()
func2()
func3()

print(func1.__name__)
print(func2.__name__)
print(func3.__name__)

