#!/bin/bash
FAXSPIN=$SECURITY_TRANSFORMS_HOME/tests/mplayer/faxspin.avi
FAXSPIN_OUT=$SECURITY_TRANSFORMS_HOME/tests/mplayer/faxspin.out
FAXSPIN_ERR=$SECURITY_TRANSFORMS_HOME/tests/mplayer/faxspin.err

HELP_OUT=$SECURITY_TRANSFORMS_HOME/tests/mplayer/help.out
HELP_ERR=$SECURITY_TRANSFORMS_HOME/tests/mplayer/help.err

report_failure()
{
    echo "TEST WRAPPER FAILED"
    exit 1
}

report_success()
{
    echo "TEST WRAPPER SUCCESS"
    exit 0
}

rm -f out err

timeout 20 $1 -h >out 2>err

if [ ! $? -eq 1 ]; then
    echo "Help Test Exit Status Failure"
    report_failure
fi

rm -f tmp tmp2

head out >tmp
head $HELP_OUT >tmp2
diff tmp tmp2
if [ ! $? -eq 0 ]; then
    echo "Help Test Std Out Failure"
    report_failure
fi

#head err >tmp
#head $HELP_ERR >tmp2
#diff tmp tmp2
diff $HELP_ERR err
if [ ! $? -eq 0 ]; then
    echo "Help Test Std Err Failure"
    report_failure
fi 

rm -f out err

timeout 20 $1 $FAXSPIN  >out 2>err
if [ ! $? -eq 0 ]; then
    echo "Faxspin Test Exit Status Failure"
    report_failure
fi 

head out >tmp
head $FAXSPIN_OUT >tmp2
diff tmp tmp2
if [ ! $? -eq 0 ]; then
    echo "Faxspin Test Std Out Failure"
    report_failure
fi

#head out >tmp
#head $FAXSPIN_ERR >tmp2
#diff tmp tmp2

diff $FAXSPIN_ERR err
if [ ! $? -eq 0 ]; then
    echo "Faxspin Test Std Err Failure"
    report_failure
fi 

report_success

