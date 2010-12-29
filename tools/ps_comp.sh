#!/bin/sh


#
# this odd loop is a way to stop/error check that no one can pass a gcc flag
# to gcc and get a reasonable compile-step.
#
for i in $*
do
	gcc -fno-stack-protector -c $i
done
