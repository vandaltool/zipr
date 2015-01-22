

DIRS=safefr transforms spasm meds2pdb cover ret_shadow_stack selective_cfi cgclibc inferfn cinderella watch_syscall

all:
	for i in $(DIRS); do cd $$i; make all; cd ..; done

clean:
	for i in $(DIRS); do cd $$i; make clean;  cd ..;done

