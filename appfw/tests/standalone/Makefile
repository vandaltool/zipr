APPFW_LIB=$(SECURITY_TRANSFORMS_HOME)/appfw/lib/libappfw.so64

all: sqlattackdetect.exe 

sqlattackdetect.o: sqlattackdetect.c
	gcc -c sqlattackdetect.c

sqlattackdetect.exe: sqlattackdetect.o
	gcc sqlattackdetect.o -o sqlattackdetect.exe $(APPFW_LIB)

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* test*.peasoup messages_to_tne.log ps_tne_logfile.txt appfw.db *.sigs
