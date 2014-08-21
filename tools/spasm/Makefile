#
# Makefile.in - DESCRIPTION.
#
# Copyright (c) 2011 - University of Virginia
#
# This file may be used and modified for non-commercial purposes as long as 
# all copyright, permission, and nonwarranty notices are preserved.  
#
# Redistribution is prohibited without prior written consent from the University of Virginia
#
# Please contact the authors for restrictions applying to commercial use.
#
# THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#

CFLAGS=  -g 
# -DUBUNTU -Wall -O3

INCLUDE=-I. -I../../include -I../../xform 
LIBS=-L../../xform -lxform 

.cpp.o .c.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c $<

all: spasm
	echo spasm build complete

clean:
	rm -f *.o core spasm *.map *.bspri *.asm *.bin

spasm: $(OBJS) spasm.cpp Makefile spasm.h spasm_main.cpp ben_lib.cpp ben_lib.h ../../xform/libxform.a 
	$(CXX) -o spasm $(INCLUDE) $(CFLAGS) spasm_main.cpp  spasm.cpp ben_lib.cpp $(OBJS) $(LIBS)
