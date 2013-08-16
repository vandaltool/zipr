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
CFLAGS= -g -O3 -Wall
INCLUDE=-I. -I../../include -I../../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../../libtransform/include/
LIBS=-L../../lib -L../../xform -lxform -lELFIO -L../../libIRDB/lib/ -lIRDB-core -lIRDB-cfg -L../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -lpqxx  -L../../libMEDSannotation/lib -lMEDSannotation -L../../libtransform/lib -ltransform

OBJS=transformutils.o

all_objs=PNTransformDriver.o PNStackLayout.o PNRange.o Range.o OffsetInference.o DirectOffsetInference.o ScaledOffsetInference.o P1Inference.o PNRegularExpressions.o PNMain.o StackLayout.o Rewrite_Utility.o General_Utility.o AnnotationBoundaryGenerator.o PrecedenceBoundaryInference.o PNIrdbManager.o



.SUFFIXES: .o .c .exe .cpp

all: pntransform.exe $(OBJS) $(PROGS)
	echo pntransform build complete

.cpp.o: PNStackLayoutInference.hpp 
	$(CC) $(INCLUDE) $(CFLAGS)  -g -c $< 

.cpp.exe:  
	$(CC) -g $< $(INCLUDE) $(LIBS) $(OBJS) -o $@

clean:
	rm -f *.o core $(PROGS)


pntransform.exe:  $(all_objs) ../../lib/libELFIO.a ../../xform/libxform.a ../../libIRDB/lib/libIRDB-core.a ../../libIRDB/lib/libIRDB-cfg.a ../../beaengine/lib/Linux.gnu.Debug/libBeaEngine_s_d.a ../../libMEDSannotation/lib/libMEDSannotation.a ../../libtransform/lib/libtransform.a
	$(CC) $(CFLAGS) $(all_objs) $(INCLUDE) $(LIBS) -o pntransform.exe
	cp pntransform.exe p1transform.exe
