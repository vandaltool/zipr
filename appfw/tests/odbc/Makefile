APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so

generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testodbc.exe testodbc.normal.exe testodbc.dlsym.exe odbc_exec_direct.exe testodbc.system.exe testodbc.system.dlsym.exe

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

odbc_exec_direct.o: odbc_exec_direct.c
	gcc -I/usr/include -c odbc_exec_direct.c -g

odbc_exec_direct.exe: odbc_exec_direct.o
	gcc odbc_exec_direct.o -o odbc_exec_direct.exe -lodbc
	$(generate_signatures) odbc_exec_direct.exe

testodbc.system.o: testodbc.system.c
	gcc -I/usr/include -c testodbc.system.c -g

testodbc.system.exe: testodbc.system.o
	gcc testodbc.system.o -o testodbc.system.exe -lodbc 
	$(generate_signatures) testodbc.system.exe

testodbc.system.dlsym.o: testodbc.system.dlsym.c
	gcc -I/usr/include -c testodbc.system.dlsym.c -g

testodbc.system.dlsym.exe: testodbc.system.dlsym.o
	gcc testodbc.system.dlsym.o -o testodbc.system.dlsym.exe -lodbc -ldl
	$(generate_signatures) testodbc.system.dlsym.exe

clean:
	rm -rf core *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt test*.exe.sigs appfw.db odbc_exec_direct.exe.peasoup *odbc*.sigs

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testodbc.system.exe testodbc.system.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testodbc.system.dlsym.exe testodbc.system.dlsym.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testodbc.exe testodbc.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh odbc_exec_direct.exe odbc_exec_direct.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	
	
test.odbc:
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
	QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 

test.normal:
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.normal.exe.sigs ./testodbc.normal.exe 
	QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.normal.exe.sigs ./testodbc.normal.exe 

test.dlsym:
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.dlsym.exe.sigs ./testodbc.dlsym.exe 
	APPFW_VERBOSE=1 QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.dlsym.exe.sigs ./testodbc.dlsym.exe 

test.system:
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.system.exe.sigs ./testodbc.system.exe ls
	APPFW_VERBOSE=1 QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.system.exe.sigs ./testodbc.system.exe ls

run: all
#	./testodbc.exe 
#	APPFW_VERBOSE=1 LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.dlsym.exe 
#	QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
