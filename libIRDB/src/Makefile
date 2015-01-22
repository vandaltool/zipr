

dirs=core cfg util syscall 

all:
	for i in ${dirs};  do echo ${PWD}; cd $$i; make || exit 1; cd ..; done

clean:
	for i in ${dirs};  do echo ${PWD}; cd $$i; make clean || exit 1; cd ..; done
