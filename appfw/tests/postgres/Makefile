APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so

generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testpg1.exe testpg2.exe testpg3.exe testpg4.exe

testpg1.o: testpg1.c
	gcc -I/usr/include/postgresql -c testpg1.c

testpg2.o: testpg2.c
	gcc -I/usr/include/postgresql -c testpg2.c

testpg3.o: testpg3.c
	gcc -I/usr/include/postgresql -c testpg3.c

testpg4.o: testpg4.c
	gcc -I/usr/include/postgresql -c testpg4.c

testpg1.exe: testpg1.o
	gcc testpg1.o -o testpg1.exe -lpq
	$(generate_signatures) testpg1.exe

testpg2.exe: testpg2.o
	gcc testpg2.o -o testpg2.exe -lpq
	$(generate_signatures) testpg2.exe

testpg3.exe: testpg3.o
	gcc testpg3.o -o testpg3.exe -lpq
	$(generate_signatures) testpg3.exe

testpg4.exe: testpg4.o
	gcc testpg4.o -o testpg4.exe -lpq
	$(generate_signatures) testpg4.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt testpg*.exe.sigs appfw.db 

peasoup: all
	APPFW_VERBOSE=1 $(PEASOUP_HOME)/tools/ps_analyze.sh testpg1.exe testpg1.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testpg2.exe testpg2.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testpg3.exe testpg3.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testpg4.exe testpg4.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	
run: all
#	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testpg4.exe.sigs ./testpg4.exe "bob"
#	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testpg3.exe.sigs ./testpg3.exe "select * from xyz;"
#	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testpg3.exe.sigs ./testpg3.exe "' or 1=1; --"
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testpg1.exe.sigs ./testpg1.exe "' or 1 = 1; --"
#	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testpg2.exe.sigs ./testpg2.exe "' or 1=1; --"
