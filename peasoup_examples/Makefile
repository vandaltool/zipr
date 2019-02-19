
all:
	cd chopzero_src; make
	#if [ -d cgc_spri ]; then cd cgc_spri; make; fi
	$(PEDI_HOME)/pedi -m manifest.txt

clean:
	cd chopzero_src; make clean
	cd examples; make clean
	cd demos; make clean
	#cd cgc_spri; make clean
	
