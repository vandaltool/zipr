all: test
define maketest
Test$(1): test$(1).c
	gcc -fno-stack-protector -o Test$(1) test$(1).c
testTest$(1): Test$(1)
	./Test$(1)
	LD_PRELOAD=../noh.so setarch $$$$(uname -m) --addr-no-randomize ./Test$(1)
	LD_PRELOAD=../noh.so setarch $$$$(uname -m) --addr-no-randomize ./Test$(1)
	NUMVARIANTS=4 VARIANTINDEX=0 LD_PRELOAD=../noh.so setarch $$$$(uname -m) --addr-no-randomize ./Test$(1)
	NUMVARIANTS=4 VARIANTINDEX=1 LD_PRELOAD=../noh.so setarch $$$$(uname -m) --addr-no-randomize ./Test$(1)
	NUMVARIANTS=4 VARIANTINDEX=2 LD_PRELOAD=../noh.so setarch $$$$(uname -m) --addr-no-randomize ./Test$(1)
	NUMVARIANTS=4 VARIANTINDEX=3 LD_PRELOAD=../noh.so setarch $$$$(uname -m) --addr-no-randomize ./Test$(1)
endef
tests=1 2
$(foreach i,$(tests),$(eval $(call maketest,$(i))))
clean:
	rm Test*
test: $(foreach i,$(tests),testTest$(i))
