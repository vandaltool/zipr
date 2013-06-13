
all:
	cd chopzero_src; make
	cd tools/pin; make

clean:
	cd chopzero_src; make clean
	cd examples; make clean
	cd demos; make clean
	
