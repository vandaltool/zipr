
LIB=../../lib/libIRDB-util.a

OBJS=insn_preds.o

all: $(OBJS)

$(OBJS): ../../include/core/*.hpp ../../include/util/*.hpp ../../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -fPIC -g -c -I. -I../../include -I../../../beaengine/include $<
	ar rc $(LIB) $@
