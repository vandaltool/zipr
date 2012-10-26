APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so
generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testosc.exe 

testosc.o: testosc.c
	gcc -c testosc.c

testosc.exe: testosc.o
	gcc testosc.o -o testosc.exe 
	$(generate_signatures) testosc.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt testosc*.exe.sigs appfw.db

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testosc.exe testosc.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	
run: all
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe "-l; cat /etc/passwd"
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe "-lth; rm /tmp/foobar"
