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

elfio:
	cd ELFIO-1.0.3; if [ ! -f Makefile ]; then ./configure --prefix=${SECURITY_TRANSFORMS_HOME};  fi; make all 
	if [ ! -f lib/libELFIO.a -o ELFIO-1.0.3/ELFIO/libELFIO.a -nt lib/libELFIO.a ]; then cd ELFIO-1.0.3; make install; fi

elfio_clean:
	cd ELFIO-1.0.3; if [ ! -f Makefile ]; then ./configure --prefix=${SECURITY_TRANSFORMS_HOME};  fi; make clean
