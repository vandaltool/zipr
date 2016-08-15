#!/bin/bash

export LD_LIBRARY_PATH=/techx_share/techx_umbrella/peasoup/zipr_trace_plugin/libtrace/:$LD_LIBRARY_PATH

mkdir -p $_CB_SERVER_TRACE_OUTPUT_DIR

/usr/share/cgc-pin/pin -t /usr/share/cgc-pin/source/tools/Trace/obj-ia32/Trace.so -o ${_CB_SERVER_TRACE_OUTPUT_DIR}/trace$$.dump -- $@
