LIB=../lib/libtransform.a

OBJS=transform.o integertransform.o leapattern.o

all: $(OBJS)

$(OBJS): ../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -g -c -I. -I../include -I../../libIRDB/include -I../../libMEDSannotation/include -I../../beaengine/include $<
	ar rc $(LIB) $@
	cp $(LIB) ../../lib/
