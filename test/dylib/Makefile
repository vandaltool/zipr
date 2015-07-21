all: dylib dylib_test math_test readline_test

dylib: dylib.c dylib.h Makefile
	gcc -shared -fpic dylib.c -olibdylib.so

dylib_test: dylib dylib_test.c Makefile
	gcc -L. -o dylib_test dylib_test.c -ldylib
math_test: math_test.c Makefile
	gcc -o math_test math_test.c -lm
readline_test: readline_test.c Makefile
	gcc -o readline_test readline_test.c -lreadline

libs/libreadline.so.6:
	mkdir -p libs/
	$$PEASOUP_HOME/tools/ps_analyze.sh /lib/x86_64-linux-gnu/libreadline.so.6 libs/libreadline.so.6 --backend zipr
libs/libdylib.so:
	mkdir -p libs/
	$$PEASOUP_HOME/tools/ps_analyze.sh libdylib.so libs/libdylib.so --backend zipr
libs/libm.so.6:
	mkdir -p libs/
	$$PEASOUP_HOME/tools/ps_analyze.sh /lib/x86_64-linux-gnu/libm.so.6 libs/libm.so.6 --backend zipr

test: dylib_test libs/libdylib.so math_test libs/libm.so.6 readline_test libs/libreadline.so.6
	LD_LIBRARY_PATH=libs/ ldd ./dylib_test
	LD_LIBRARY_PATH=libs/ ./dylib_test > dylib_test.protected.out
	ldd ./dylib_test
	./dylib_test > dylib_test.out
	diff dylib_test.protected.out dylib_test.out
	rm -rf dylib_test*.out
	LD_LIBRARY_PATH=libs/ ldd ./math_test	
	LD_LIBRARY_PATH=libs/ ./math_test > math_test.protected.out
	ldd ./math_test
	./math_test > math_test.out
	diff math_test.protected.out math_test.out
	rm -rf math_test*.out
	LD_LIBRARY_PATH=libs/ ldd ./readline_test	
	echo "asdf" | LD_LIBRARY_PATH=libs/ ./readline_test > readline_test.protected.out
	ldd ./readline_test	
	echo "asdf" | ./readline_test > readline_test.out
	diff readline_test.protected.out readline_test.out
	rm -rf readline_test*.out
clean:
	rm -rf libs dylib dylib_test math_test libdylib.so readline_test
	rm -rf peasoup_executable_directory*
