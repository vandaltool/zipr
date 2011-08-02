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


PROGS=nulltransform p1transform integerbugtransform.exe
CC=g++
CFLAGS= -g
INCLUDE=-I. -I../../include -I../../libIRDB/include -I../../xform -I../../beaengine/include -I$(SECURITY_TRANSFORMS_HOME)/libIRDB/include
LIB_IRDB=-L$(SECURITY_TRANSFORMS_HOME)/libIRDB/lib -lIRDB -lpqxx
LIBS=-L../../lib -L../../xform -lxform -lELFIO -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d $(LIB_IRDB)

.SUFFIXES: .o .c .exe .cpp

.cpp.exe:  $< 
	$(CC) -g $< $(INCLUDE) $(LIBS) -o $@

all: $(PROGS)
	echo build complete

clean:
	rm -f *.o core $(PROGS)

nulltransform: nulltransform.cpp
	$(CC) -o nulltransform $(INCLUDE) nulltransform.cpp $(OBJS) $(LIBS)

p1transform: p1transform.cpp
	$(CC) -o p1transform $(INCLUDE) p1transform.cpp $(OBJS) $(LIBS)

