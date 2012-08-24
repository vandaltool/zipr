LIBAPPFW_DIR=../lib

all: libappfw 

extract_sql:
	tar -zxvf ../../third_party/sqlite-autoconf-3071300.tar.gz --no-anchored --wildcards 'sqlite3.[ch]'
	cp ./sqlite-autoconf-3071300/sqlite3.c .
	cp ./sqlite-autoconf-3071300/sqlite3.h .

sqlite3.o: sqlite3.c sqlite3.h sqlfw.c
	cat sqlfw.c >> sqlite3.c
	gcc -fPIC -DSQLITE_OS_UNIX=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEBUG=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_4_BYTE_ALIGNED_MALLOC -c sqlite3.c

appfw.o: appfw.c
	gcc -c appfw.c

pg_hook.o: pg_hook.c
	gcc -DSHOW_TAINT_MARKINGS -I/usr/include/postgresql -c pg_hook.c
	
libappfw: extract_sql sqlite3.o pg_hook.o appfw.o
	gcc -shared -ldl -fPIC -o $(LIBAPPFW_DIR)/libappfw.so sqlite3.o pg_hook.o appfw.o 

clean:
	rm -fr *.o *.tmp *.so sqlite3.c sqlite3.h sqlite-autoconf-3071300
