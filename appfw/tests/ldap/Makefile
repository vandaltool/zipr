APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so64
generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testldapintercept.exe testldapintercept2.exe testldapintercept3.exe testldapintercept4.exe

testldapintercept.o: testldapintercept.c
	gcc -c testldapintercept.c

testldapintercept.exe: testldapintercept.o
	gcc testldapintercept.o -o testldapintercept.exe -lldap
	$(generate_signatures) testldapintercept.exe

testldapintercept2.o: testldapintercept2.c
	gcc -c testldapintercept2.c

testldapintercept2.exe: testldapintercept2.o
	gcc testldapintercept2.o -o testldapintercept2.exe -lldap
	$(generate_signatures) testldapintercept2.exe

testldapintercept3.o: testldapintercept3.c
	gcc -c testldapintercept3.c

testldapintercept3.exe: testldapintercept3.o
	gcc testldapintercept3.o -o testldapintercept3.exe -lldap
	$(generate_signatures) testldapintercept3.exe

testldapintercept4.o: testldapintercept4.c
	gcc -c testldapintercept4.c

testldapintercept4.exe: testldapintercept4.o
	gcc testldapintercept4.o -o testldapintercept4.exe -lldap
	$(generate_signatures) testldapintercept4.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt appfw.db *.sigs

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testldapintercept.exe testldapintercept.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testldapintercept.exe testldapintercept2.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testldapintercept.exe testldapintercept3.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testldapintercept.exe testldapintercept4.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off --step ilr=off --step determine_program=off
	
run: 
	APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testldapintercept.exe.sigs ./testldapintercept.exe; 
#APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testldapintercept.exe.sigs ./testldapintercept2.exe; APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testldapintercept.exe.sigs ./testldapintercept3.exe; APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testldapintercept.exe.sigs ./testldapintercept4.exe 
