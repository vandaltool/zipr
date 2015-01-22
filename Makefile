
all:
	cd chopzero_src; make
	cd cgc_spri; make
	#cd tools/pin; make

clean:
	cd chopzero_src; make clean
	cd examples; make clean
	cd demos; make clean
	cd cgc_spri; make clean
	
