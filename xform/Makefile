#
# Makefile.in - DESCRIPTION.
#
# Copyright (c) 2011 - University of Virginia
#
# This file may be used and modified for non-commercial purposes as long as 
# all copyright, permission, and nonwarranty notices are preserved.  
#
# Please contact the authors for restrictions applying to commercial use.
#
# THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#

CC=$(CXX)
CFLAGS= -DUBUNTU -g
DIR=..
INCLUDEDIR=${DIR}/include
INCLUDE=-I${INCLUDEDIR} -I. -I${DIR}/beaengine/include
LIB=libxform.a

OBJS= instruction_descriptor.o function_descriptor.o elfreader.o null_transform.o rewriter.o spri_alloc.o gen_hash.o constant_hash.o funclist_hash.o instrmap_hash.o framerestore_hash.o stackref_hash.o 

.SUFFIXES: .o .c .cpp

.cpp.o .c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<


all: ../lib/$(LIB)
	echo $(LIB) build complete

$(LIB): $(OBJS)
	ar -r $(LIB) $(OBJS) 

../lib/$(LIB): $(LIB)
	cp $(LIB) ../lib/

clean:
	rm -f *.o core *.a

stackref_hash.o gen_hash.o: *.h stackref_hash.c gen_hash.c
	$(CC) $(CFLAGS) $(INCLUDE) -fomit-frame-pointer -c $*.c

$(OBJS): *.h  
