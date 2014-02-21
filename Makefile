all:	lib include/targ-config.h elfio bea 
	cd libIRDB; make all
	cd xform; make all
	cd libMEDSannotation; make all
	cd libtransform; make all
	cd tools; make all
	cd appfw; if   /usr/bin/test ! -f Makefile -o Makefile.in -nt Makefile ; then ./configure ; fi ; make all

clean: elfio_clean
	cd beaengine; cmake -D CMAKE_C_COMPILER=`which gcc` -D CMAKE_CXX_COMPILER=`which g++` -D BEA_COMPILER=gnu .; make clean
	cd libIRDB; make clean
	cd xform; make clean
	cd libMEDSannotation; make clean
	cd libtransform; make clean
	cd tools; make clean
	cd appfw; make clean
	rm -f lib/*
	rm -f include/config.h

#bea_dir=Linux.gnu.Debug
bea_dir=SunOS.gnu.Debug

bea:	
	cd beaengine; cmake -D CMAKE_C_COMPILER=`which gcc` -D CMAKE_CXX_COMPILER=`which g++` -D BEA_COMPILER=gnu .; make all
	if   /usr/bin/test ! -f lib/libBeaEngine_s_d.a -o ./beaengine/lib/$(bea_dir)/libBeaEngine_s_d.a -nt lib/libBeaEngine_s_d.a ; then cp ./beaengine/lib/$(bea_dir)/libBeaEngine_s_d.a lib/libBeaEngine_s_d.a; fi



ELFIO_DIR=third_party/ELFIO/elfio-2.2

elfio: 	third_party/elfio-2.2.tar.gz
	if  /usr/bin/test ! -d $(ELFIO_DIR) ; then mkdir -p third_party/ELFIO; cd third_party/ELFIO; gtar xpzvf ../elfio-2.2.tar.gz; fi
	cd $(ELFIO_DIR); if [ ! -f Makefile ]; then ./configure --prefix=${SECURITY_TRANSFORMS_HOME};  fi; 
	cd $(ELFIO_DIR); make all 
	cd $(ELFIO_DIR); make install 
	#cd include/;  patch -p0 < ../third_party/elfio.patch

elfio_clean:
	rm -Rf third_party/ELFIO
	rm -Rf include/elfio

include/targ-config.h: .PHONY
	if   /usr/bin/test ! -f $@ -o include/`uname -m`/config.h -nt $@ ; then cp include/`uname -m`/config.h $@; fi

.PHONY:

lib: 
	mkdir lib
	
