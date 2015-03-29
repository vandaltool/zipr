
LIB=../../lib/libIRDB-core.a


OBJS=baseobj.o type.o variantid.o pqxxdb.o dbinterface.o function.o fileir.o file.o instruction.o address.o icfs.o generate_spri.o 

all: $(OBJS)

$(OBJS): ../../include/core/*.hpp *.hpp ../../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -fPIC -g -c -I. -I../../include -I../../../beaengine/include $<
	ar rc $(LIB) $@
