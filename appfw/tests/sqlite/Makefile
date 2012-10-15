APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so

generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testsqlite.exe

testsqlite.o: testsqlite.c
	gcc -c -I. testsqlite.c

testsqlite.exe: testsqlite.o
	gcc testsqlite.o -o testsqlite.exe -lsqlite3
	$(generate_signatures) testsqlite.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt testpg*.exe.sigs appfw.db *.sigs

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testsqlite.exe testsqlite.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	
run: all
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testsqlite.exe.sigs ./testsqlite.exe "David' or '0'='0"
