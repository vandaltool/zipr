APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so

generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testsqlite.exe testsqlite.pstmt.exe

testsqlite.o: testsqlite.c
	gcc -c -I. testsqlite.c

testsqlite.exe: testsqlite.o
	gcc -v testsqlite.o -o testsqlite.exe -lsqlite3
	$(generate_signatures) testsqlite.exe

testsqlite.pstmt.o: testsqlite.pstmt.c
	gcc -c -I. testsqlite.pstmt.c

testsqlite.pstmt.exe: testsqlite.pstmt.o
	gcc -v testsqlite.pstmt.o -o testsqlite.pstmt.exe -lsqlite3
	$(generate_signatures) testsqlite.pstmt.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt testpg*.exe.sigs appfw.db *.sigs

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testsqlite.exe testsqlite.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testsqlite.pstmt.exe testsqlite.pstmt.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	
run: all
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testsqlite.exe.sigs ./testsqlite.exe "David' or '0'='0"
