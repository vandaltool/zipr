APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so
generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testmysql.exe testintercept.exe testintercept2.exe testintercept.pstmt.exe

testmysql.o: testmysql.c
	gcc -g -I/usr/include/mysql -c testmysql.c

testmysql.exe: testmysql.o
	gcc -g testmysql.o -o testmysql.exe -lmysqlclient
	$(generate_signatures) testmysql.exe

testintercept.o: testintercept.c
	gcc -g -I/usr/include/mysql -c testintercept.c

testintercept.exe: testintercept.o
	gcc -g testintercept.o -o testintercept.exe -lmysqlclient
	$(generate_signatures) testintercept.exe

testintercept2.o: testintercept2.c
	gcc -g -I/usr/include/mysql -c testintercept2.c

testintercept2.exe: testintercept2.o
	gcc -g testintercept2.o -o testintercept2.exe -lmysqlclient
	$(generate_signatures) testintercept2.exe

testintercept.pstmt.o: testintercept.pstmt.c
	gcc -g -I/usr/include/mysql -c testintercept.pstmt.c

testintercept.pstmt.exe: testintercept.pstmt.o
	gcc -g testintercept.pstmt.o -o testintercept.pstmt.exe -lmysqlclient
	$(generate_signatures) testintercept.pstmt.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt appfw.db *.sigs

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testintercept.exe testintercept.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testintercept2.exe testintercept2.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testmysql.exe testmysql.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testintercept.pstmt.exe testintercept.pstmt.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	
run: all
	@echo "---------------------------------------------------------------------"
	@echo Test 1:
	@echo "---------------------------------------------------------------------"
	QUERY_DATA="David' or '0'='0" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testmysql.exe.sigs ./testmysql.exe  bob
	@echo "---------------------------------------------------------------------"
	@echo Test 2:
	@echo "---------------------------------------------------------------------"
	QUERY_DATA="David' or '0'='0" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept.exe.sigs ./testintercept.exe 
	@echo "---------------------------------------------------------------------"
	@echo Test 3:
	@echo "---------------------------------------------------------------------"
	QUERY_DATA="David' or '0'='0" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept2.exe.sigs ./testintercept.exe 
