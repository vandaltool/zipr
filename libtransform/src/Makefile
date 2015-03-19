LIB=../lib/libtransform.a

OBJS=Rewrite_Utility.o transform.o integertransform.o leapattern.o integertransform64.o integertransform32.o 

all: $(OBJS)

$(OBJS): ../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	$(CXX) -g -c -I. -I../include -I../../libIRDB/include -I../../libMEDSannotation/include -I../../beaengine/include $<
	ar rc $(LIB) $@
	cp $(LIB) ../../lib/
