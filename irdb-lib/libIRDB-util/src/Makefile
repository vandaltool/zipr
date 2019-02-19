
LIB=../../lib/libIRDB-util.a

OBJS=insn_preds.o

all: $(OBJS)

$(OBJS): ../../include/core/*.hpp ../../include/util/*.hpp ../../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	$(CXX) -fPIC -g -c -I. -I../../include -I../../../beaengine/include $<
	ar rc $(LIB) $@
