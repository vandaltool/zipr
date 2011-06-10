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

CC=g++
CFLAGS= -DUBUNTU -g
INCLUDE=-I. -I../../xform
LIBS=-L../../lib -L../../xform -lxform

.SUFFIXES: .o .c .cpp

.cpp.o .c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<


all: spasm
	echo build complete

clean:
	rm -f *.o core spasm

spasm: $(OBJS) spasm.c
	$(CC) -o spasm $(INCLUDE) spasm.c $(OBJS) $(LIBS)
