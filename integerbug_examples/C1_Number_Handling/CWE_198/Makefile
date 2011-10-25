# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

use_of_incorrect_byte_order.ncexe: use_of_incorrect_byte_order.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >use_of_incorrect_byte_order.conf

use_of_incorrect_byte_order: use_of_incorrect_byte_order.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee use_of_incorrect_byte_order.ps_analyze.log
	cp use_of_incorrect_byte_order.conf use_of_incorrect_byte_order.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> use_of_incorrect_byte_order.conf

use_of_incorrect_byte_order.run: use_of_incorrect_byte_order
	../../../scripts/test.pl $(TESTFLAGS) use_of_incorrect_byte_order.ncexe

use_of_incorrect_byte_order_safe.ncexe: use_of_incorrect_byte_order.c
	$(CC) $(CFLAGS)  -DSAFE $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  -DSAFE $< -o $@ $(LINKOPT) >use_of_incorrect_byte_order_safe.conf

use_of_incorrect_byte_order_safe: use_of_incorrect_byte_order_safe.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee use_of_incorrect_byte_order_safe.ps_analyze.log
	cp use_of_incorrect_byte_order_safe.conf use_of_incorrect_byte_order_safe.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> use_of_incorrect_byte_order_safe.conf

use_of_incorrect_byte_order_safe.run: use_of_incorrect_byte_order_safe
	../../../scripts/test.pl $(TESTFLAGS) use_of_incorrect_byte_order_safe.ncexe

clean:
	rm -f  use_of_incorrect_byte_order.ncexe use_of_incorrect_byte_order_safe.ncexe
	rm -f  use_of_incorrect_byte_order use_of_incorrect_byte_order_safe
	rm -f  summary.csv logfail.txt logfile.txt use_of_incorrect_byte_order.asm use_of_incorrect_byte_order.idb use_of_incorrect_byte_order.ncexe* stratafier.o.exe *.conf use_of_incorrect_byte_order_safe.asm use_of_incorrect_byte_order_safe.idb use_of_incorrect_byte_order_safe.ncexe* $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  use_of_incorrect_byte_order.ncexe use_of_incorrect_byte_order_safe.ncexe

analyze: targets use_of_incorrect_byte_order use_of_incorrect_byte_order_safe

runall:  use_of_incorrect_byte_order.run use_of_incorrect_byte_order_safe.run

