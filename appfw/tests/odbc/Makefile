APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so

generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testodbc.exe 

testodbc.o: testodbc.c
	gcc -I/usr/include -c testodbc.c -g

testodbc.exe: testodbc.o
	gcc testodbc.o -o testodbc.exe -lodbc
	$(generate_signatures) testodbc.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt testpg*.exe.sigs appfw.db 

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testodbc.exe testodbc.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	
run: all
#	./testodbc.exe 
	APPFW_VERBOSE=1 LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
#	QUERY_DATA="' or 1 = 1; -- " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testodbc.exe.sigs ./testodbc.exe 
