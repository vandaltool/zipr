APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so
generate_signatures=$(PEASOUP_HOME)/tools/generate_string_signatures.sh

all: testosc.exe testosc_semi.exe testosc_samesig.exe testosc_exec_args.exe

testosc.o: testosc.c
	gcc -g -c testosc.c

testosc.exe: testosc.o 
	gcc testosc.o -o testosc.exe 
	$(generate_signatures) testosc.exe

testosc_semi.o: testosc_semi.c
	gcc -g -c testosc_semi.c

testosc_semi.exe: testosc_semi.o 
	gcc testosc_semi.o -o testosc_semi.exe 
	$(generate_signatures) testosc_semi.exe

testosc_samesig.o: testosc_samesig.c
	gcc -g -c testosc_samesig.c

testosc_samesig.exe: testosc_samesig.o 
	gcc testosc_samesig.o -o testosc_samesig.exe 
	$(generate_signatures) testosc_samesig.exe

testosc_exec_args.o: testosc_exec_args.c
	gcc -g -c testosc_exec_args.c

testosc_exec_args.exe: testosc_exec_args.o 
	gcc testosc_exec_args.o -o testosc_exec_args.exe 
	$(generate_signatures) testosc_exec_args.exe

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt testosc*.exe.sigs appfw.db

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh testosc.exe testosc.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testosc_semi.exe testosc_semi.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh testosc_samesig.exe testosc_samesig.exe.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	
run: all
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe 1 "-l; cat /etc/passwd"
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe 2 "-l; cat /etc/passwd"
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe 3 "-l; cat /etc/passwd"

run2: all
	LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe 1 "-lth; touch /tmp/foobar"

test_semi: testosc_semi.exe
	APPFW_VERBOSE=1 DATA="ls -lta" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc_semi.exe.sigs ./testosc_semi.exe 
	APPFW_VERBOSE=1 DATA="  " LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc_semi.exe.sigs ./testosc_semi.exe 

test_samesig: testosc_samesig.exe
	APPFW_VERBOSE=1 DATA="-foobar" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc_samesig.exe.sigs ./testosc_samesig.exe 

test_exec_args: testosc_exec_args.exe
	VERY_VERBOSE=1 VERBOSE=1 APPFW_VERBOSE=1 DATA="-ltar" LD_PRELOAD=$(APPFW_LIB) APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc_exec_args.exe.sigs ./testosc_exec_args.exe 
