LIBAPPFW_DIR=../lib

#CFLAGS=-g -DSHOW_TAINT_MARKINGS 
#CFLAGS=-DSHOW_TAINT_MARKINGS -O
CFLAGS=-O -fPIC

all: libappfw.so

./sqlite-autoconf-3071300/sqlite3.c ./sqlite-autoconf-3071300/sqlite3.h sqlite3.h:
	tar -zxvf ../../third_party/sqlite-autoconf-3071300.tar.gz --no-anchored --wildcards 'sqlite3.[ch]'
	cat ./sqlite-autoconf-3071300/sqlite3.c | sed "s/sqlite3/appfw_sqlite3/g" > appfw_sqlite3.c
	cat ./sqlite-autoconf-3071300/sqlite3.h | sed "s/sqlite3/appfw_sqlite3/g" > appfw_sqlite3.h
	cp ./sqlite-autoconf-3071300/sqlite3.h .

appfw_sqlite3.o: sqlfw.c ./sqlite-autoconf-3071300/sqlite3.c ./sqlite-autoconf-3071300/sqlite3.h sqlite3.h
	cat appfw_sqlite3.c sqlfw.c > tmp.c
	gcc $(CFLAGS) -fPIC -DSQLITE_OS_UNIX=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEBUG=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_4_BYTE_ALIGNED_MALLOC -c tmp.c -o appfw_sqlite3.o
	rm tmp.c

appfw.o: appfw.cpp
	g++ $(CFLAGS) -c appfw.cpp

pg_hook.o: pg_hook.c
	gcc $(CFLAGS) -I/usr/include/postgresql -c pg_hook.c

mysql_hook.o: mysql_hook.c
	gcc $(CFLAGS) -I/usr/include/mysql -c mysql_hook.c

sqlite3_hook.o: sqlite3_hook.c
	gcc $(CFLAGS) -c sqlite3_hook.c

hooker.o: hooker.c
	gcc $(CFLAGS) -c hooker.c

osc_hook.o: osc_hook.c
	gcc $(CFLAGS) -c osc_hook.c

osc_parse.o: osc_parse.cpp
	g++ $(CFLAGS)  -c $<

oscfw.o: oscfw.c
	gcc $(CFLAGS) -c oscfw.c

xqil_major:
	apt-cache show libxqilla-dev|grep Filename|sed -e "s/.*dev_//" -e "s/\..*//" > xqil_major

xqil_minor:
	apt-cache show libxqilla-dev|grep Filename|sed -e "s/.*dev_..//" -e "s/\..*//" > xqil_minor

xq_hook.o: xq_hook.cpp xqil_major xqil_minor
	g++ $(CFLAGS) -DXQILLA_MAJOR_VER=`cat xqil_major` -DXQILLA_MINOR_VER=`cat xqil_minor` -c $<  -I/usr/include/libxml2

xq_parser.o: xq_parser.cpp
	g++ $(CFLAGS)  -c $<

xqfw.o: xqfw.c
	gcc $(CFLAGS) -c xqfw.c

appfw_ldap.o: appfw_ldap.c
	gcc $(CFLAGS) -c appfw_ldap.c

openldap_hook.o: openldap_hook.c
	gcc $(CFLAGS) -I/usr/include -c $< 

libappfw.so: appfw_sqlite3.o pg_hook.o mysql_hook.o appfw.o sqlite3_hook.o oscfw.o osc_hook.o osc_parse.o xq_hook.o xq_parser.o xqfw.o openldap_hook.o appfw_ldap.o hooker.o
	g++ $(CFLAGS) -shared -fPIC -o libappfw.so $^ -ldl
	cp libappfw.so $(LIBAPPFW_DIR)/libappfw.so 

clean:
	rm -fr *.o *.tmp *.so appfw_sqlite3.c appfw_sqlite3.h sqlite3.h sqlite-autoconf-3071300  xqil_major xqil_minor
