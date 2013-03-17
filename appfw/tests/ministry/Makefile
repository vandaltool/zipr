# Current assumption:
#   Peasoupified demo binaries are executed via Web server cgi-bin facilities
#   cgi-bin scripts are in /usr/lib/cgi-bin

BINARIES=spelldescription oscommand
PEASOUP_BINARIES=spelldescription.peasoup oscommand.peasoup

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

oscommand.peasoup: oscommand
	$(PEASOUP_HOME)/tools/ps_analyze.sh oscommand oscommand.peasoup --step concolic=off --step integertransform=off --step p1transform=off

# need to run these once to create the diagnostics.out files
runonce: $(BINARIES)
	-./oscommand.peasoup
	-./spelldescription.peasoup

# the web server will run as user nobody and does not
# have the proper permissions to run these binaries.
# we change permissions to allow anyone to execute the peasoup_executable
# directory and write to the diagnostics.out file
# to make sure the diagnostics.out file exist, we run the executables once
webhack: runonce
	chmod og+rwx peasoup_exec*
	chmod og+rw peasoup_exec*/diagnostics.out

install: $(PEASOUP_BINARIES) webhack
	sudo cp $(BINARIES) $(PEASOUP_BINARIES) /usr/lib/cgi-bin

clean:
	rm -rf *.o *.tmp *.exe peasoup_executable_directory* messages_to_tne.log ps_tne_logfile.txt appfw.db *.sigs *.peasoup $(BINARIES) $(PEASOUP_BINARIES)

peasoup: $(PEASOUP_BINARIES)

