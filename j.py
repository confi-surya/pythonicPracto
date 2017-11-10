def fun():
    j=0
    while j<2:
       j+=1
       fun()
    print "hello:%s"% j

fun()
