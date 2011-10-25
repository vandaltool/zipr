# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

CWE_195_Example_1_bad.ncexe: CWE_195_Example_1_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_195_Example_1_bad.conf

CWE_195_Example_1_bad: CWE_195_Example_1_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_195_Example_1_bad.ps_analyze.log
	cp CWE_195_Example_1_bad.conf CWE_195_Example_1_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_195_Example_1_bad.conf

CWE_195_Example_1_bad.run: CWE_195_Example_1_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_195_Example_1_bad.ncexe

CWE_195_Example_2_bad.ncexe: CWE_195_Example_2_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_195_Example_2_bad.conf

CWE_195_Example_2_bad: CWE_195_Example_2_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_195_Example_2_bad.ps_analyze.log
	cp CWE_195_Example_2_bad.conf CWE_195_Example_2_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_195_Example_2_bad.conf

CWE_195_Example_2_bad.run: CWE_195_Example_2_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_195_Example_2_bad.ncexe

CWE_195_Example_3_bad.ncexe: CWE_195_Example_3_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_195_Example_3_bad.conf

CWE_195_Example_3_bad: CWE_195_Example_3_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_195_Example_3_bad.ps_analyze.log
	cp CWE_195_Example_3_bad.conf CWE_195_Example_3_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_195_Example_3_bad.conf

CWE_195_Example_3_bad.run: CWE_195_Example_3_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_195_Example_3_bad.ncexe

CWE_195_Example_4_bad.ncexe: CWE_195_Example_4_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_195_Example_4_bad.conf

CWE_195_Example_4_bad: CWE_195_Example_4_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_195_Example_4_bad.ps_analyze.log
	cp CWE_195_Example_4_bad.conf CWE_195_Example_4_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_195_Example_4_bad.conf

CWE_195_Example_4_bad.run: CWE_195_Example_4_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_195_Example_4_bad.ncexe

gen: gen.cc
	$(CPP) $(CPPFLAGS)  $< -o $@ $(LINKOPT)
clean:
	rm -f  CWE_195_Example_1_bad.ncexe CWE_195_Example_2_bad.ncexe CWE_195_Example_3_bad.ncexe CWE_195_Example_4_bad.ncexe gen
	rm -f  CWE_195_Example_1_bad CWE_195_Example_2_bad CWE_195_Example_3_bad CWE_195_Example_4_bad
	rm -f  summary.csv logfail.txt logfile.txt CWE_195_Example_1_bad.asm CWE_195_Example_1_bad.idb CWE_195_Example_1_bad.ncexe* stratafier.o.exe *.conf CWE_195_Example_2_bad.asm CWE_195_Example_2_bad.idb CWE_195_Example_2_bad.ncexe* stratafier.o.exe *.conf CWE_195_Example_3_bad.asm CWE_195_Example_3_bad.idb CWE_195_Example_3_bad.ncexe* stratafier.o.exe *.conf CWE_195_Example_4_bad.asm CWE_195_Example_4_bad.idb CWE_195_Example_4_bad.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  CWE_195_Example_1_bad.ncexe CWE_195_Example_2_bad.ncexe CWE_195_Example_3_bad.ncexe CWE_195_Example_4_bad.ncexe gen

analyze: targets CWE_195_Example_1_bad CWE_195_Example_2_bad CWE_195_Example_3_bad CWE_195_Example_4_bad

runall:  CWE_195_Example_1_bad.run CWE_195_Example_2_bad.run CWE_195_Example_3_bad.run CWE_195_Example_4_bad.run

