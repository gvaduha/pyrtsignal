#! /usr/bin/env -S python3 -X faulthandler

import pyrtsignal as r
import os, time

def sighandler(s,v):
    #pass
    print("sighandler for",s,v)

r.set_sighandler(0, sighandler)
#r.set_sighandler(31, sighandler)

print("--- local tests ---")
r.test_sighandler(0,10)

print("--- signal tests ---")
pid = os.getpid()
print("pid:",pid)
print("sigfun:",sighandler)
r.send_signal(pid, 0, 11)
r.send_signal(pid, 0, 11)

#time.sleep(120)
try:
    r.suspend_for_signal(0)
except InterruptedError:
    pass
print("test finished")
