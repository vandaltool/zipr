
CC="cc -I/opt/csw/include"
CXX="CC -I/opt/csw/include"

all:
	cd src;make CC=$(CC) CXX=$(CXX)
	./install_libs.sh
	cd test;make CC=$(CC) CXX=$(CXX)


clean:
	cd src;make clean
	cd test;make  clean
