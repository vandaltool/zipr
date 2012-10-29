LIBAPPFW_DIR=../lib

CFLAGS=-g -DSHOW_TAINT_MARKINGS

all: libappfw 

extract_sql:
	tar -zxvf ../../third_party/sqlite-autoconf-3071300.tar.gz --no-anchored --wildcards 'sqlite3.[ch]'
	cat ./sqlite-autoconf-3071300/sqlite3.c | sed "s/sqlite3/appfw_sqlite3/g" > appfw_sqlite3.c
	cat ./sqlite-autoconf-3071300/sqlite3.h | sed "s/sqlite3/appfw_sqlite3/g" > appfw_sqlite3.h
	cp ./sqlite-autoconf-3071300/sqlite3.h .

appfw_sqlite3.o: extract_sql sqlfw.c
	cat sqlfw.c >> appfw_sqlite3.c
	gcc -fPIC -DSQLITE_OS_UNIX=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEBUG=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_4_BYTE_ALIGNED_MALLOC -c appfw_sqlite3.c

appfw.o: appfw.c
	gcc $(CFLAGS) -c appfw.c

oscfw.o: oscfw.c
	gcc $(CFLAGS) -c oscfw.c

pg_hook.o: pg_hook.c
	gcc $(CFLAGS) -I/usr/include/postgresql -c pg_hook.c

mysql_hook.o: mysql_hook.c
	gcc $(CFLAGS) -I/usr/include/mysql -c mysql_hook.c

sqlite3_hook.o: sqlite3_hook.c
	gcc $(CFLAGS) -c sqlite3_hook.c

osc_hook.o: osc_hook.c
	gcc $(CFLAGS) -c osc_hook.c

osc_parse.o: osc_parse.cpp
	g++ $(CFLAGS)  -c $<
	
libappfw: appfw_sqlite3.o pg_hook.o mysql_hook.o appfw.o sqlite3_hook.o oscfw.o osc_hook.o osc_parse.o
	g++ $(CFLAGS) -shared -ldl -fPIC -o $(LIBAPPFW_DIR)/libappfw.so appfw_sqlite3.o mysql_hook.o pg_hook.o appfw.o sqlite3_hook.o oscfw.o osc_hook.o osc_parse.o

clean:
	rm -fr *.o *.tmp *.so appfw_sqlite3.c appfw_sqlite3.h sqlite3.h sqlite-autoconf-3071300 
