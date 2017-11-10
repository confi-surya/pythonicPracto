def fun1(a,b,c,d):
	print a,b,c,d

l=[5,6,7,8]
print "=================================================================================="
print " !!!!! Understanding what '*' does from inside a function call !!!!! "
fun1(*l)

d={'c':5, 'd':7}
print "=================================================================================="
print " !!!!! Understanding what '**' does from inside a function call !!!!! "
fun1(1,2,**d)



def test_var_args(f_arg, *argv):
    print "first normal arg:", f_arg
    for arg in argv:
        print "another arg through *argv :", arg

print "=================================================================================="
print " !!!!!!!  understanding what '*args' mean inside a function definition !!!!!!! "

test_var_args('yasoob','python','eggs','test')


def greet_me(**kwargs):
    if kwargs is not None:
        for key, value in kwargs.iteritems():
            print "%s == %s" %(key,value)

print "=================================================================================="
print " !!!!!!! understanding what '*kwargs' mean inside a function definition !!!!!!! "

greet_me(name="yasoob")



