INC=$(ZIPR_HOME)/callbacks/inferfn


# prince driver cannot link against CGC library
CALLBACK_LIBS=$(ZIPR_HOME)/callbacks/lib/callbacks.a

all: prince.exe

clean:
	rm -f *.o *.exe

prince.o: prince.c
	gcc -I$(INC) -c prince.c -o prince.o
	
prince.exe: prince.o
	gcc prince.o -o prince.exe 

test.o: test.c
	gcc -I$(INC) -c test.c -o test.o
	
test.exe: test.o
	gcc test.o -o test.exe $(CALLBACK_LIBS)
