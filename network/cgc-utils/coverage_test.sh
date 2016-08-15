#!/bin/bash

# Add libtrace to the library path so that trace-* utilities
# work as expected.
export LD_LIBRARY_PATH=/techx_share/techx_umbrella/peasoup/zipr_trace_plugin/libtrace/:$LD_LIBRARY_PATH

if [ $# -ne 2 ]; then
	echo "usage: <dir> <cb name>"
	exit 1
fi

dir=$1
shift

#cb=`ls bin | grep -v _patched | grep -v elf`
cb=$1
shift

cd /techx_share/techx_umbrella/peasoup/ 1>&2
source set_env_vars 1>&2
cd - 1>&2

cd $dir > /dev/null 2>&1;
if [ $? -ne 0 ]; then
	echo "Invalid test directory"
	exit 1
fi

make DO_TRACE=1 DO_COVERAGE=1 1>&2
# Add libtrace utils to the path so it's easier to read.
export PATH=/techx_share/techx_umbrella/peasoup/zipr_trace_plugin/libtrace/utils:$PATH

trace-merge build/master.coverage.trace build/for-coverage-traces/*
trace-merge build/master.poller.trace build/for-release-traces/* build/for-testing/traces/*
trace-print build/master.poller.trace > build/master.poller.trace.print
trace-print build/master.coverage.trace > build/master.coverage.trace.print
trace-compare.py --annotate bin/$cb --objdump /usr/i386-linux-cgc/bin/objdump build/master.coverage.trace.print build/master.poller.trace.print > build/master.trace_comparison
exit 0
