# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

CWE_369_Example_1_bad.ncexe: CWE_369_Example_1_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_369_Example_1_bad.conf

CWE_369_Example_1_bad: CWE_369_Example_1_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_369_Example_1_bad.ps_analyze.log
	cp CWE_369_Example_1_bad.conf CWE_369_Example_1_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_369_Example_1_bad.conf

CWE_369_Example_1_bad.run: CWE_369_Example_1_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_369_Example_1_bad.ncexe

CWE_369_Example_1_good.ncexe: CWE_369_Example_1_good.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_369_Example_1_good.conf

CWE_369_Example_1_good: CWE_369_Example_1_good.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_369_Example_1_good.ps_analyze.log
	cp CWE_369_Example_1_good.conf CWE_369_Example_1_good.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_369_Example_1_good.conf

CWE_369_Example_1_good.run: CWE_369_Example_1_good
	../../../scripts/test.pl $(TESTFLAGS) CWE_369_Example_1_good.ncexe

clean:
	rm -f  CWE_369_Example_1_bad.ncexe CWE_369_Example_1_good.ncexe
	rm -f  CWE_369_Example_1_bad CWE_369_Example_1_good
	rm -f  summary.csv logfail.txt logfile.txt CWE_369_Example_1_bad.asm CWE_369_Example_1_bad.idb CWE_369_Example_1_bad.ncexe* stratafier.o.exe *.conf CWE_369_Example_1_good.asm CWE_369_Example_1_good.idb CWE_369_Example_1_good.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  CWE_369_Example_1_bad.ncexe CWE_369_Example_1_good.ncexe

analyze: targets CWE_369_Example_1_bad CWE_369_Example_1_good

runall:  CWE_369_Example_1_bad.run CWE_369_Example_1_good.run

