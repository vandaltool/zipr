APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so

generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testodbc.exe testodbc.normal.exe testodbc.dlsym.exe

testodbc.o: testodbc.c
	gcc -I/usr/include -c testodbc.c -g

testodbc.exe: testodbc.o
	gcc testodbc.o -o testodbc.exe -lodbc
	$(generate_signatures) testodbc.exe

testodbc.normal.o: testodbc.normal.c
	gcc -I/usr/include -c testodbc.normal.c

testodbc.normal.exe: testodbc.normal.o
	gcc testodbc.normal.o -o testodbc.normal.exe -lodbc
	$(generate_signatures) testodbc.normal.exe

testodbc.dlsym.o: testodbc.dlsym.c
	gcc -I/usr/include -c testodbc.dlsym.c

testodbc.dlsym.exe: testodbc.dlsym.o
	gcc testodbc.dlsym.o -o testodbc.dlsym.exe -lodbc
	$(generate_signatures) testodbc.dlsym.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt test*.exe.sigs appfw.db 

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testodbc.exe testodbc.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	
test.odbc:
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
	APPFW_VERBOSE=1 QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 

test.normal:
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.normal.exe.sigs ./testodbc.normal.exe 
	APPFW_VERBOSE=1 QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.normal.exe.sigs ./testodbc.normal.exe 

test.dlsym:
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.dlsym.exe.sigs ./testodbc.dlsym.exe 
	APPFW_VERBOSE=1 QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.dlsym.exe.sigs ./testodbc.dlsym.exe 

run: all
#	./testodbc.exe 
#	APPFW_VERBOSE=1 LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.dlsym.exe 
#	QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
