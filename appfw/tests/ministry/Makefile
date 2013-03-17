BINARIES=spelldescription oscommand

all: $(BINARIES)

httpparse.o: httpparse.c
	gcc -I/usr/include/mysql -c httpparse.c

spelldescription.o: spelldescription.c
	gcc -I/usr/include/mysql -c spelldescription.c

spelldescription: spelldescription.o httpparse.o
	gcc spelldescription.o httpparse.o -o spelldescription -lmysqlclient

spelldescription.peasoup: spelldescription
	$(PEASOUP_HOME)/tools/ps_analyze.sh spelldescription spelldescription.peasoup --step concolic=off --step integertransform=off --step p1transform=off

oscommand.o: oscommand.c
	gcc -c oscommand.c

oscommand: oscommand.o httpparse.o
	gcc oscommand.o httpparse.o -o oscommand 

runonce: $(BINARIES)
	-./oscommand.peasoup
	-./spelldescription.peasoup

webhack: runonce
	chmod og+rwx peasoup_exec*
	chmod og+rw peasoup_exec*/diagnostics.out

install: webhack
	sudo cp $(BINARIES) /usr/lib/cgi-bin

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* messages_to_tne.log ps_tne_logfile.txt appfw.db *.sigs *.peasoup $(BINARIES)

peasoup: all
	$(PEASOUP_HOME)/tools/ps_analyze.sh oscommand oscommand.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	$(PEASOUP_HOME)/tools/ps_analyze.sh spelldescription spelldescription.peasoup --step concolic=off --step integertransform=off --step p1transform=off
	

