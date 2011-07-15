
LIB=../lib/libIRDB.a


OBJS=baseobj.o  variantid.o pqxxdb.o dbinterface.o function.o variantir.o file.o instruction.o address.o generate_spri.o

all: $(OBJS)

$(OBJS): ../include/*.hpp *.hpp

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -g -c -I../include -I../../beaengine/include $<
	ar rc $(LIB) $@
