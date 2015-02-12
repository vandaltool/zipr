
LIB=../lib/libMEDSannotation.a

OBJS=VirtualOffset.o MEDS_Register.o MEDS_AnnotationParser.o MEDS_InstructionCheckAnnotation.o MEDS_SafeFuncAnnotation.o MEDS_ProblemFuncAnnotation.o MEDS_FRSafeAnnotation.o FuncExitAnnotation.o MEDS_FPTRShadowAnnotation.o MEDS_FuncPrototypeAnnotation.o

all: $(OBJS)

$(OBJS): ../include/*.hpp 

clean:
	rm -f $(OBJS) *.o 

.cpp.o:
	g++ -g -c -I. -I../include $<
	ar rc $(LIB) $@
	cp $(LIB) ../../lib
