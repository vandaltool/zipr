
LIB=../lib/libIRDB.a


OBJS=baseobj.o  variantid.o pqxxdb.o dbinterface.o function.o variantir.o file.o instruction.o address.o

all: $(OBJS)

$(OBJS): ../include/*.hpp *.hpp

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -g -c -I../include $<
	ar rc $(LIB) $@
