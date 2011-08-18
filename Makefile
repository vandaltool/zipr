all:
	cd ELFIO-1.0.3; make all install
	cd beaengine; make all
	cd libIRDB; make all
	cd xform; make all
	cd libIRDB; make all
	cd tools; make all

clean:
	cd ELFIO-1.0.3; make clean
	cd beaengine; make clean
	cd libIRDB; make clean
	cd xform; make clean
	cd tools; make clean
