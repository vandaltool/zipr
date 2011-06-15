
LIB=../lib/libIRDB.a


OBJS=baseobj.o  programid.o pqxxdb.o dbinterface.o

all: $(OBJS)

clean:
	rm $(OBJS)

.cpp.o:
	g++ -g -c -I../include $<
	ar rc $(LIB) $@
