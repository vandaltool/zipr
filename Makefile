
all:
	cd src;make

install: all
	cp src/zipr.exe bin

clean:
	cd src;make clean
