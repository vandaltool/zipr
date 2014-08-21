APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so64
generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testmysql.exe testintercept.exe testintercept2.exe testintercept3.exe testintercept4.exe testintercept.pstmt.exe

testmysql.o: testmysql.c
	gcc -I/usr/include/mysql -c testmysql.c

testmysql.exe: testmysql.o
	gcc testmysql.o -o testmysql.exe -lmysqlclient
	$(generate_signatures) testmysql.exe

testintercept.o: testintercept.c
	gcc -I/usr/include/mysql -c testintercept.c

testintercept.exe: testintercept.o
	gcc testintercept.o -o testintercept.exe -lmysqlclient
	$(generate_signatures) testintercept.exe

testintercept2.o: testintercept2.c
	gcc -I/usr/include/mysql -c testintercept2.c

testintercept2.exe: testintercept2.o
	gcc testintercept2.o -o testintercept2.exe -lmysqlclient
	$(generate_signatures) testintercept2.exe

testintercept3.o: testintercept3.c
	gcc -I/usr/include/mysql -c testintercept3.c

testintercept3.exe: testintercept3.o
	gcc testintercept3.o -o testintercept3.exe -lmysqlclient
	$(generate_signatures) testintercept3.exe

testintercept4.o: testintercept4.c
	gcc -I/usr/include/mysql -c testintercept4.c

testintercept4.exe: testintercept4.o
	gcc testintercept4.o -o testintercept4.exe -lmysqlclient
	$(generate_signatures) testintercept4.exe

testintercept.pstmt.o: testintercept.pstmt.c
	gcc -I/usr/include/mysql -c testintercept.pstmt.c

testintercept.pstmt.exe: testintercept.pstmt.o
	gcc testintercept.pstmt.o -o testintercept.pstmt.exe -lmysqlclient
	$(generate_signatures) testintercept.pstmt.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt appfw.db *.sigs

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testintercept.exe testintercept.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testintercept2.exe testintercept2.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testmysql.exe testmysql.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testintercept.pstmt.exe testintercept.pstmt.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	
run: all
#	QUERY_DATA="David' or '0'='0" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testmysql.exe.sigs ./testmysql.exe 
#	QUERY_DATA="David' or '0'='0" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept.exe.sigs ./testintercept.exe 
#	QUERY_DATA="David' or '0'='0" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept2.exe.sigs ./testintercept.exe 

# Injections detected (Thanks to Billy for all these examples):
	APPFW_VERBOSE=1 QUERY_DATA="a'='a" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept2.exe.sigs ./testintercept2.exe 
#	APPFW_VERBOSE=1 QUERY_DATA="1<2" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept3.exe.sigs ./testintercept3.exe  (fixed)
#	APPFW_VERBOSE=1 QUERY_DATA="1!=1" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept3.exe.sigs ./testintercept3.exe 
#	APPFW_VERBOSE=1 QUERY_DATA="'<~'" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept2.exe.sigs ./testintercept2.exe 
#	APPFW_VERBOSE=1 QUERY_DATA="' xor 0=''" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept2.exe.sigs ./testintercept2.exe 
#	APPFW_VERBOSE=1 QUERY_DATA="myId" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept3.exe.sigs ./testintercept3.exe 
#	APPFW_VERBOSE=1 QUERY_DATA="'foobar' /* hello */" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept3.exe.sigs ./testintercept3.exe 
#	APPFW_VERBOSE=1 QUERY_DATA="'foobar' --x;" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept3.exe.sigs ./testintercept3.exe 

#==========================================
# Injections not detected still:
#==========================================
#
#	APPFW_VERBOSE=1 QUERY_DATA="'+'" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept2.exe.sigs ./testintercept2.exe 
#	APPFW_VERBOSE=1 QUERY_DATA="'-'" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept2.exe.sigs ./testintercept2.exe 
#
#                         
# mySQL allows commands in comments
#	APPFW_VERBOSE=1 QUERY_DATA="'/*!(if(0,143,387))*/'" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept3.exe.sigs ./testintercept3.exe 
#
#==========================================
# Altered functionality
#==========================================
# not support by sqlite3 parser (altered functionality on hex constants):
# sqlite3 doesn't allow hex values of this type
#	APPFW_VERBOSE=1 QUERY_DATA="0xaabb11" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testintercept4.exe.sigs ./testintercept4.exe 
