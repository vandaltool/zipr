

DIRS=safefr transforms spasm meds2pdb cover ret_shadow_stack selective_cfi

all:
	for i in $(DIRS); do cd $$i; make all; cd ..; done
#	cd spasm; make all
#	cd transforms; make all
#	cd meds2pdb; make all
#	cd cover; make all

clean:
	for i in $(DIRS); do cd $$i; make clean;  cd ..;done
#	cd spasm; make clean
#	cd transforms; make clean
#	cd meds2pdb; make clean
#	cd cover; make clean

