import os
import subprocess
import sys
import datetime
import time

failcnt = 0
for i in range(0,int(sys.argv[1])):
    ts = time.time()
    st = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
    print(st.ljust(20) + "Run " + str(i).ljust(5) + " - failures: " + str(failcnt))
    try:
      # https://cmake.org/cmake/help/v3.0/manual/ctest.1.html
      # -I [Start,End,Stride,test#,test#|Test file]
      tests = " -I " + sys.argv[2]
      print("Running supset of tests: " + tests)
    except:
      tests = ""

    trace = subprocess.Popen("ctest -V" + tests, shell=True, stdout=subprocess.PIPE).stdout.read()
    if '100%' not in trace:
        print(trace)
        failcnt = failcnt + 1

if (tests):
    print("Run supset of tests: " + tests)
print("failure count: " + str(failcnt))
