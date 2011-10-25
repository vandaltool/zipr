# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

dumbledore_store_wrap_around_error.ncexe: dumbledore_store_wrap_around_error.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >dumbledore_store_wrap_around_error.conf

dumbledore_store_wrap_around_error: dumbledore_store_wrap_around_error.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee dumbledore_store_wrap_around_error.ps_analyze.log
	cp dumbledore_store_wrap_around_error.conf dumbledore_store_wrap_around_error.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> dumbledore_store_wrap_around_error.conf

dumbledore_store_wrap_around_error.run: dumbledore_store_wrap_around_error
	../../../scripts/test.pl $(TESTFLAGS) dumbledore_store_wrap_around_error.ncexe

wrap_around_error.ncexe: wrap_around_error.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >wrap_around_error.conf

wrap_around_error: wrap_around_error.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee wrap_around_error.ps_analyze.log
	cp wrap_around_error.conf wrap_around_error.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> wrap_around_error.conf

wrap_around_error.run: wrap_around_error
	../../../scripts/test.pl $(TESTFLAGS) wrap_around_error.ncexe

clean:
	rm -f  dumbledore_store_wrap_around_error.ncexe wrap_around_error.ncexe
	rm -f  dumbledore_store_wrap_around_error wrap_around_error
	rm -f  summary.csv logfail.txt logfile.txt dumbledore_store_wrap_around_error.asm dumbledore_store_wrap_around_error.idb dumbledore_store_wrap_around_error.ncexe* stratafier.o.exe *.conf wrap_around_error.asm wrap_around_error.idb wrap_around_error.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  dumbledore_store_wrap_around_error.ncexe wrap_around_error.ncexe

analyze: targets dumbledore_store_wrap_around_error wrap_around_error

runall:  dumbledore_store_wrap_around_error.run wrap_around_error.run

