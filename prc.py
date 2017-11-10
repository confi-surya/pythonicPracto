import time
import procname
import sys

script_prefix = "RECOVERY_"

proc_name = script_prefix + sys.argv[1]
print "Process name is %s" % proc_name
procname.setprocname(proc_name)
var = 1
while var == 1:
        time.sleep(10)
        print "Sleeping"

