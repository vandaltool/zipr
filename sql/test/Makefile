
all: libsqlfw test1.exe test2.exe

test2.exe: test2.o sqlfw.c
	gcc test2.o sqlite3.o -o test2.exe -L. -lsqlfw
	
test1.exe: test1.o sqlfw.c
	gcc test1.o sqlite3.o -o test1.exe -L. -lsqlfw
#	gcc test1.o sqlite3.o -o test1.exe -lpthread

test1.o: test1.c
	gcc -c test1.c

test2.o: test2.c
	gcc -c test2.c

sqlite3.o: sqlite3.c sqlite3.h sqlfw.c
	cp ../sqlite-autoconf-3071300/sqlite3.c .
	cp ../sqlite-autoconf-3071300/sqlite3.h .
	cat sqlfw.c >> sqlite3.c
	gcc -fPIC -DSQLITE_OS_UNIX=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEBUG=1 -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c

libsqlfw: sqlite3.o
	gcc -shared -o libsqlfw.so sqlite3.o 

clean:
	rm *.o *.tmp
