CC=g++
CFLAGS= -g
INCLUDE=-I. -I../../include -I../../xform -I../../beaengine/include -I../../libMEDSannotation/include -I../../libIRDB/include
LIBS=-L../../lib -lxform -lBeaEngine_s_d -lpq -lpqxx -lIRDB-core -lMEDSannotation

.SUFFIXES: .o .c .cpp

.cpp.o .c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<


all: meds2pdb  
	echo meds2pdb build complete

clean:
	rm -f *.o core meds2pdb

meds2pdb: $(OBJS) meds2pdb.cpp ../../lib/*.a
	$(CC) $(CFLAGS) -o meds2pdb $(INCLUDE) meds2pdb.cpp $(OBJS) $(LIBS)

