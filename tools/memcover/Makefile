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


PROGS=memcover.exe

CFLAGS= -g 
# -O3 -Wall
INCLUDE=-I. -I../../include -I../../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../../libtransform/include/
LIBS=-L../../lib -L../../xform -lxform -lELFIO -L../../libIRDB/lib/ -lIRDB-core -lIRDB-cfg -L../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -lpqxx  -L../../libMEDSannotation/lib -lMEDSannotation -L../../libtransform/lib -ltransform
OBJS=transformutils.o

.SUFFIXES: .o .c .exe .cpp

all: memcover $(OBJS) $(PROGS)
	echo memcover build complete

.o: $<
	$(CXX) -g -c $@

.cpp.exe:  $<
	$(CXX) -g $< $(INCLUDE) $(LIBS) $(OBJS) -o $@

clean:
	rm -f *.o core $(PROGS)


memcover: 
	$(CXX) $(CFLAGS) memcover.cpp General_Utility.cpp Rewrite_Utility.cpp $(INCLUDE) $(LIBS) -o memcover.exe
