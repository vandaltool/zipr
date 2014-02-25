APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so64

#all: sqlattackdetect.exe testfastdetect.exe testslowdetect.exe s3.exe
all: s3.exe

sqlattackdetect.o: sqlattackdetect.c
	gcc -c -I../../src sqlattackdetect.c

sqlattackdetect.exe: sqlattackdetect.o
	gcc sqlattackdetect.o -o sqlattackdetect.exe $(APPFW_LIB)

testfastdetect.o: testfastdetect.c
	gcc -c -I../../src testfastdetect.c

testfastdetect.exe: testfastdetect.o
	gcc testfastdetect.o -o testfastdetect.exe $(APPFW_LIB)

testslowdetect.o: testslowdetect.c
	gcc -c -I../../src testslowdetect.c

testslowdetect.exe: testslowdetect.o
	gcc testslowdetect.o -o testslowdetect.exe $(APPFW_LIB)

s3.o: s3.c
	gcc -c -I../../src s3.c

s3.exe: s3.o
	gcc s3.o -o s3.exe $(APPFW_LIB)

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt appfw.db *.sigs
