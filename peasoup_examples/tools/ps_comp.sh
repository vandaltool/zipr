#!/bin/sh


#
# this odd loop is a way to stop/error check that no one can pass a gcc flag
# to gcc and get a reasonable compile-step.
#
baseflags="-fno-stack-protector -c -w"

#
# don't pass me flags or i'll smack you.
# exceptions: -I is OK
#
for i in $*
do
        echo $i|egrep "^-";     # check for starting with a -
        if [ 0 -eq  $? ] ; then
                echo $i|egrep "^-I" > /dev/null;    # check for starting with a -o
                dashI=$?

                if [ 0 -eq  $dashI ] ; then
                        echo -n ; # blank on purpose
                else
                        echo SMACK\! No flags to this script, only files to link
                        exit 1
                fi
        elif [ ! -f $i ]; then
                echo File $i not found
                exit 2
        fi
done

gcc $baseflags $*
retval=$?
# return gcc's exit code
exit $retval

