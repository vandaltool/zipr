
LIB=../../lib/libIRDB-core.a


OBJS=baseobj.o  variantid.o pqxxdb.o dbinterface.o function.o variantir.o file.o instruction.o address.o generate_spri.o

all: $(OBJS)

$(OBJS): ../../include/core/*.hpp *.hpp ../../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -g -c -I. -I../../include -I../../../beaengine/include $<
	ar rc $(LIB) $@
