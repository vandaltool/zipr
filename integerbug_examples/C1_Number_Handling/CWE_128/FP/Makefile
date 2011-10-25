# DO NOT EDIT - This Makefile automatically generated
include ../../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../../scripts/test.pl $(TESTFLAGS) .

dbl_equals.ncexe: dbl_equals.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >dbl_equals.conf

dbl_equals: dbl_equals.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee dbl_equals.ps_analyze.log
	cp dbl_equals.conf dbl_equals.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> dbl_equals.conf

dbl_equals.run: dbl_equals
	../../../../scripts/test.pl $(TESTFLAGS) dbl_equals.ncexe

dbl_sqrt.ncexe: dbl_sqrt.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >dbl_sqrt.conf

dbl_sqrt: dbl_sqrt.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee dbl_sqrt.ps_analyze.log
	cp dbl_sqrt.conf dbl_sqrt.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> dbl_sqrt.conf

dbl_sqrt.run: dbl_sqrt
	../../../../scripts/test.pl $(TESTFLAGS) dbl_sqrt.ncexe

dbl_square.ncexe: dbl_square.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >dbl_square.conf

dbl_square: dbl_square.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee dbl_square.ps_analyze.log
	cp dbl_square.conf dbl_square.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> dbl_square.conf

dbl_square.run: dbl_square
	../../../../scripts/test.pl $(TESTFLAGS) dbl_square.ncexe

fp_equals.ncexe: fp_equals.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >fp_equals.conf

fp_equals: fp_equals.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee fp_equals.ps_analyze.log
	cp fp_equals.conf fp_equals.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> fp_equals.conf

fp_equals.run: fp_equals
	../../../../scripts/test.pl $(TESTFLAGS) fp_equals.ncexe

fp_sqrt.ncexe: fp_sqrt.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >fp_sqrt.conf

fp_sqrt: fp_sqrt.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee fp_sqrt.ps_analyze.log
	cp fp_sqrt.conf fp_sqrt.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> fp_sqrt.conf

fp_sqrt.run: fp_sqrt
	../../../../scripts/test.pl $(TESTFLAGS) fp_sqrt.ncexe

fp_square.ncexe: fp_square.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >fp_square.conf

fp_square: fp_square.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee fp_square.ps_analyze.log
	cp fp_square.conf fp_square.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> fp_square.conf

fp_square.run: fp_square
	../../../../scripts/test.pl $(TESTFLAGS) fp_square.ncexe

clean:
	rm -f  dbl_equals.ncexe dbl_sqrt.ncexe dbl_square.ncexe fp_equals.ncexe fp_sqrt.ncexe fp_square.ncexe
	rm -f  dbl_equals dbl_sqrt dbl_square fp_equals fp_sqrt fp_square
	rm -f  summary.csv logfail.txt logfile.txt dbl_equals.asm dbl_equals.idb dbl_equals.ncexe* stratafier.o.exe *.conf dbl_sqrt.asm dbl_sqrt.idb dbl_sqrt.ncexe* stratafier.o.exe *.conf dbl_square.asm dbl_square.idb dbl_square.ncexe* stratafier.o.exe *.conf fp_equals.asm fp_equals.idb fp_equals.ncexe* stratafier.o.exe *.conf fp_sqrt.asm fp_sqrt.idb fp_sqrt.ncexe* stratafier.o.exe *.conf fp_square.asm fp_square.idb fp_square.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  dbl_equals.ncexe dbl_sqrt.ncexe dbl_square.ncexe fp_equals.ncexe fp_sqrt.ncexe fp_square.ncexe

analyze: targets dbl_equals dbl_sqrt dbl_square fp_equals fp_sqrt fp_square

runall:  dbl_equals.run dbl_sqrt.run dbl_square.run fp_equals.run fp_sqrt.run fp_square.run

