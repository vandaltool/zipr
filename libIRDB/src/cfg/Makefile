
LIB=../../lib/libIRDB-cfg.a

OBJS=BasicBlock.o CFG.o

all: $(OBJS)

$(OBJS): ../../include/core/*.hpp ../../include/cfg/*.hpp ../../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -g -c -I. -I../../include -I../../../beaengine/include $<
	ar rc $(LIB) $@
