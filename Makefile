

all:
	cd chopzero_src; make
	cd tools;make

clean:
	cd chopzero_src; make clean
	cd examples; make clean
	cd tools; make clean
	
