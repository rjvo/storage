#!/bin/bash

for i in {1..10000}
do
   da=$(date +"%T")
   #./bin/rmc -b 192.168.0.201 -k 10 -t test -m ${da}
   ./bin/rmc -b 138.68.74.70 -s 10101 -k 10 -t test -m ${da}
   sleep 1
done
