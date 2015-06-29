#!/bin/sh

hosts="U64-01 U64-02 U64-03 U64-04 U64-05 U64-06 U64-07 U64-08 U64-09 U64-10 U64-11 U64-12 U64-13 U64-14 U64-15 U64-16 U64-17 U64-18 U64-19 U64-20"

remoteUser="ps1"

for host in $hosts; do
  scp $remoteUser@$host:/tmp/$host-results.txt .
done;

echo "Hostname, Copy Time, Wait Time, Execute Time, Copy Answer Time" >>IdaPro-timings.txt
cat *-results.txt >>IdaPro-timings.txt
rm *-results.txt
