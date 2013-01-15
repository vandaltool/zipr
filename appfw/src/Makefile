LIBAPPFW_DIR=../lib

CFLAGS=-g -DSHOW_TAINT_MARKINGS 

all: libappfw 

./sqlite-autoconf-3071300/sqlite3.c ./sqlite-autoconf-3071300/sqlite3.h sqlite3.h:
	tar -zxvf ../../third_party/sqlite-autoconf-3071300.tar.gz --no-anchored --wildcards 'sqlite3.[ch]'
	cat ./sqlite-autoconf-3071300/sqlite3.c | sed "s/sqlite3/appfw_sqlite3/g" > appfw_sqlite3.c
	cat ./sqlite-autoconf-3071300/sqlite3.h | sed "s/sqlite3/appfw_sqlite3/g" > appfw_sqlite3.h
	cp ./sqlite-autoconf-3071300/sqlite3.h .

appfw_sqlite3.o: sqlfw.c ./sqlite-autoconf-3071300/sqlite3.c ./sqlite-autoconf-3071300/sqlite3.h sqlite3.h
	cat appfw_sqlite3.c sqlfw.c > tmp.c
	gcc $(CFLAGS) -fPIC -DSQLITE_OS_UNIX=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEBUG=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_4_BYTE_ALIGNED_MALLOC -c tmp.c -o appfw_sqlite3.o
	rm tmp.c

appfw.o: appfw.c
	gcc $(CFLAGS) -c appfw.c

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

oscfw.o: oscfw.c
	gcc $(CFLAGS) -c oscfw.c

xq_hook.o: xq_hook.cpp
	g++ $(CFLAGS) -c $^  -I/usr/include/libxml2

xq_parser.o: xq_parser.cpp
	g++ $(CFLAGS)  -c $<

xqfw.o: xqfw.c
	gcc $(CFLAGS) -c xqfw.c

libappfw: appfw_sqlite3.o pg_hook.o mysql_hook.o appfw.o sqlite3_hook.o oscfw.o osc_hook.o osc_parse.o xq_hook.o xq_parser.o xqfw.o
	g++ $(CFLAGS) -shared -fPIC -o $(LIBAPPFW_DIR)/libappfw.so $^ -ldl

clean:
	rm -fr *.o *.tmp *.so appfw_sqlite3.c appfw_sqlite3.h sqlite3.h sqlite-autoconf-3071300 
