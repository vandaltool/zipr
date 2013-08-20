#
# Makefile.in - DESCRIPTION.
#
# Copyright (c) 2011 - University of Virginia
#
# This file may be used and modified for non-commercial purposes as long as 
# all copyright, permission, and nonwarranty notices are preserved.  
# Redistribution is prohibited without prior written consent from the University of Virginia
#
# Please contact the authors for restrictions applying to commercial use.
#
# THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#


CC=g++
CFLAGS= -g
INCLUDE=-I. -I../../include -I../../xform -I../../beaengine/include -I../../libIRDB/include/
LIBS=-L../../lib -L../../xform -lxform -lIRDB-core -lBeaEngine_s_d -lpqxx 
OBJS=coverage.o

.SUFFIXES: .o .c .cpp

.cpp.o .c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<


all: cover 
	echo cover build complete

clean:
	rm -f *.o core cover

cover: $(OBJS) cover.cpp ../../lib/*
	$(CC) $(CFLAGS) -o cover $(INCLUDE) cover.cpp $(OBJS) $(LIBS)

