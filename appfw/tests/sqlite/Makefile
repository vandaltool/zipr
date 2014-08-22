.SUFFIXES: .o .c .cpp .exe .peasoup .lightpeasoup

all: testsqlite.basic.exe testsqlite.exe testsqlite.pstmt.exe testsqlite.pstmt.fp.exe
peasoup: testsqlite.basic.peasoup testsqlite.peasoup testsqlite.pstmt.peasoup testsqlite.pstmt.fp.peasoup
lightpeasoup: testsqlite.basic.lightpeasoup testsqlite.lightpeasoup testsqlite.pstmt.lightpeasoup testsqlite.pstmt.fp.lightpeasoup

.exe.lightpeasoup:
	$(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step isr=off

.exe.peasoup:
	$(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 

.o.exe:
	gcc -g $< -lsqlite3 -o $@

.c.o:
	gcc -c -I. $<

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup test*.light*peasoup messages_to_tne.log ps_tne_logfile.txt testpg*.exe.sigs appfw.db *.sigs
