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


PROGS=p1transform.exe nulltransform.exe integertransformdriver.exe

CC=g++
CFLAGS= -g -Wall
INCLUDE=-I. -I../../include -I../../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../../libtransform/include/
LIBS=-L../../lib -L../../xform -lxform -lELFIO -L../../libIRDB/lib/ -lIRDB-core -lIRDB-cfg -L../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -lpqxx  -L../../libMEDSannotation/lib -lMEDSannotation -L../../libtransform/lib -ltransform
OBJS=transformutils.o

.SUFFIXES: .o .c .exe .cpp

all: pntransform $(OBJS) $(PROGS)
	echo build complete

.o: $<
	$(CC) -g -c $@

.cpp.exe:  $<
	$(CC) -g $< $(INCLUDE) $(LIBS) $(OBJS) -o $@

clean:
	rm -f *.o core $(PROGS)


pntransform: PNTransformDriver.cpp PNStackLayoutInference.hpp PNStackLayout.cpp PNRange.cpp Range.cpp OffsetInference.cpp DirectOffsetInference.cpp ScaledOffsetInference.cpp P1Inference.cpp PNRegularExpressions.cpp PNMain.cpp StackLayout.cpp Rewrite_Utility.cpp General_Utility.cpp
	$(CC) $(CFLAGS) PNMain.cpp PNTransformDriver.cpp PNStackLayout.cpp PNRange.cpp Range.cpp OffsetInference.cpp DirectOffsetInference.cpp ScaledOffsetInference.cpp P1Inference.cpp PNRegularExpressions.cpp StackLayout.cpp Rewrite_Utility.cpp General_Utility.cpp $(INCLUDE) $(LIBS) -o pntransform.exe
	cp pntransform.exe p1transform.exe
