all:	elfio bea
	cd libIRDB; make all
	cd xform; make all
	cd libMEDSannotation; make all
	cd libtransform; make all
	cd tools; make all
	cd appfw; make all

clean: elfio_clean
	cd beaengine; cmake .; make clean
	cd libIRDB; make clean
	cd xform; make clean
	cd libMEDSannotation; make clean
	cd libtransform; make clean
	cd tools; make clean
	cd appfw; make clean
	rm lib/*

bea:	
	cd beaengine; cmake .; make all
	if [ ! -f lib/libBeaEngine_s_d.a -o ./beaengine/lib/Linux.gnu.Debug/libBeaEngine_s_d.a -nt lib/libBeaEngine_s_d.a ]; then cp ./beaengine/lib/Linux.gnu.Debug/libBeaEngine_s_d.a lib/libBeaEngine_s_d.a; fi



ELFIO_DIR=third_party/ELFIO/elfio-2.2

elfio: 	third_party/elfio-2.2.tar.gz
	if [ ! -d $(ELFIO_DIR) ]; then mkdir -p third_party/ELFIO; cd third_party/ELFIO; tar xpzvf ../elfio-2.2.tar.gz; fi
	cd $(ELFIO_DIR); if [ ! -f Makefile ]; then ./configure --prefix=${SECURITY_TRANSFORMS_HOME};  fi; make all 

elfio_clean:
	rm -Rf third_party/ELFIO
