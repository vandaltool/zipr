all: Tester
Tester: tester.c
	gcc -fno-stack-protector -o Tester tester.c
clean:
	rm Tester
test: Tester
	LD_PRELOAD=../noh.so setarch $$(uname -m) --addr-no-randomize ./Tester
	LD_PRELOAD=../noh.so setarch $$(uname -m) --addr-no-randomize ./Tester
	NUMVARIANTS=4 VARIANTINDEX=0 LD_PRELOAD=../noh.so setarch $$(uname -m) --addr-no-randomize ./Tester
	NUMVARIANTS=4 VARIANTINDEX=1 LD_PRELOAD=../noh.so setarch $$(uname -m) --addr-no-randomize ./Tester
	NUMVARIANTS=4 VARIANTINDEX=2 LD_PRELOAD=../noh.so setarch $$(uname -m) --addr-no-randomize ./Tester
	NUMVARIANTS=4 VARIANTINDEX=3 LD_PRELOAD=../noh.so setarch $$(uname -m) --addr-no-randomize ./Tester
