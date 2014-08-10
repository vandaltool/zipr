
LIB=../lib/libMEDSannotation.a

OBJS=VirtualOffset.o MEDS_Register.o MEDS_AnnotationParser.o MEDS_InstructionCheckAnnotation.o MEDS_SafeFuncAnnotation.o

all: $(OBJS)

$(OBJS): ../include/*.hpp 

clean:
	rm -f $(OBJS) *.o 

.cpp.o:
	g++ -g -c -I. -I../include $<
	ar rc $(LIB) $@
	cp $(LIB) ../../lib
