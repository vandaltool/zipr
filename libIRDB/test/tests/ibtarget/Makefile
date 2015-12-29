INCLUDES= -I../../../../include -I../../../include/ -I../../../../beaengine/include 
#LIBS=-L ../../../lib/ -lIRDB-core -lIRDB-cfg -lIRDB-util -lpqxx -L ../../../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d 
#OPT=-fPIE -fPIC -O
#OPT=-g -fPIE -fPIC
OPT=-O
.SUFFIXES: .exe .cpp .protected

PROGS=jmp1.exe jmp2.exe jmp.shared.exe icall.exe
PEASOUP_PROGS=jmp1.protected jmp2.protected jmp.shared.protected icall.protected

all: $(PROGS)
protected: $(PEASOUP_PROGS)

$(PROGS): ../../../../lib/*

.exe.protected: $<
#	FIX_CALLS_FIX_ALL_CALLS=1 LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ --backend zipr --step selective_cfi=on 
	FIX_CALLS_FIX_ALL_CALLS=1 LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ --backend zipr --step selective_cfi=on --step-option selective_cfi:--color
#	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ --backend zipr --stop_after pdb_register

.o.exe:  $< ../lib/libIRDB-core.a ../lib/libIRDB-cfg.a
	g++ $< $(INCLUDES) $(LIBS) $(OPT)  -o $@

.cpp.o:  $< 
	g++ $< $(INCLUDES) $(LIBS) $(OPT) -o $@ -c

jmp.shared.so: jmp.shared.cpp jmp2.shared.cpp
	g++ -fPIC -c jmp.shared.cpp
	g++ -fPIC -c jmp2.shared.cpp
	g++ -shared -Wl,-soname,libjmp.so -o libjmp.so jmp.shared.o jmp2.shared.o

jmp.shared.exe: jmp.shared.so jmp.main.cpp
	g++ -c jmp.main.cpp
	g++ jmp.main.o -L. -ljmp -o $@

clean:
	rm -fr $(PROGS) *.o *.so peasoup_executable* *.protected *.tmp.orig 
