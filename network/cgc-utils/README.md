This is a set of utilities for cgc.

## calculate\_stats.sh
This 'make's each subdirectory in cwd and records
stats (using find\_maxrsss.sh) about the runs.

## find\_maxrsss.sh
This computes the average of all the maxrss (or
minflt, depending on $1) and outputs it in csv.

## cgc-cb.mk
This is a replacement for the standard cgc-cb.mk
that adds a DO\_ZIPR option. When DO\_ZIPR is specified
on the make commandline (make DO\_ZIPR=1), the make
will run the resulting binaries through zipr and
then test the zipr'd version the same way that it would
test the for-release and \_patched.for-release.

## beanstalk/
All the code related to launching testing jobs in beanstalk.
