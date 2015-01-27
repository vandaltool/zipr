sample.globals.o: sample.globals.c
	gcc -c sample.globals.c -fno-stack-protector
#	gcc -mcmodel=large -fno-pic -fno-PIC -c sample.globals.c -fno-stack-protector
	
sample.globals.exe: sample.globals.o
	gcc sample.globals.o -o $@

sample.o: sample.c
	gcc -c sample.c -fno-stack-protector
	
sample: sample.o
	gcc sample.o -o sample.exe

sample.shadow: sample
	$(PEASOUP_HOME)/tools/ps_analyze.fptr.sh sample.fptrannot sample.exe sample.shadow

sample_loop.o: sample_loop.c
	gcc -c sample_loop.c -fno-stack-protector
	
sample_loop: sample_loop.o
	gcc sample_loop.o -o sample_loop.exe

sample_loop.shadow: sample_loop
	$(PEASOUP_HOME)/tools/ps_analyze.fptr.sh sample_loop.fptrannot sample_loop.exe sample_loop.shadow

bus.exe: bus.c
	gcc bus.c -o bus.exe -lpthread

bus.peasoup: bus.exe
	$(PEASOUP_HOME)/tools/ps_analyze.sh bus.exe bus.peasoup

bus.shadow: bus.exe
	$(PEASOUP_HOME)/tools/ps_analyze.fptr.sh bus.fptrannot bus.exe bus.shadow

clean:
	rm -fr *.o peasoup_executable* *.peasoup bus.exe  *.shadow *.exe
