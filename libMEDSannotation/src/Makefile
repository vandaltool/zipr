
LIB=../lib/libMEDSannotation.a

OBJS=VirtualOffset.o MEDS_Register.o MEDS_AnnotationParser.o MEDS_InstructionCheckAnnotation.o 

all: $(OBJS)

$(OBJS): ../include/*.hpp 

clean:
	rm -f $(OBJS)

.cpp.o:
	g++ -g -c -I. -I../include $<
	ar rc $(LIB) $@
	cp $(LIB) ../../lib
