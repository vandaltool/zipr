#!/bin/sh
ORIG=$SECURITY_TRANSFORMS_HOME/tests/simple/dumbledore_cmd_O3.exe
XFORM=$1

report_failure()
{
    rm -f out xout err xerr status xstatus
    echo "TEST WRAPPER FAILED"
    exit 1
}

report_success()
{
    echo "TEST WRAPPER SUCCESS"
    exit 0
}


test_xform()
{
    $ORIG $* >out 2>err
    status=$?
    $XFORM $* >xout 2>xerr
    xstatus=$?

    if [ ! $status -eq $xstatus ]; then
	echo $status
	echo $xstatus
	echo "Error status failure"
	report_failure
    fi

    diff out xout
    if [ ! $? -eq 0 ]; then
	echo "Standard output failure"
	report_failure
    fi

    diff err xerr
    if [ ! $? -eq 0 ]; then
	echo "Error output failure"
	report_failure
    fi

    rm -f out xout err xerr status xstatus
}

test_xform 
test_xform Ben
test_xform "Hello World"
test_xform "Wizard in Training"
report_success
