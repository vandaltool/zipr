INCLUDES= -I../../../../include -I../../../include/ -I../../../../beaengine/include 
LIBS=-L ../../../lib/ -lIRDB-core -lIRDB-cfg -lIRDB-util -lpqxx -L ../../../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d 
OPT=-g -fPIE -fPIC -O2 
#OPT=-O
.SUFFIXES: .exe .cpp .peasoup

PROGS=jmp1.exe jmp2.exe jmp.shared.exe
PEASOUP_PROGS=jmp1.peasoup jmp2.peasoup jmp.shared.peasoup

all: $(PROGS)
peasoup: $(PEASOUP_PROGS)

$(PROGS): ../../../../lib/*

.exe.peasoup: $<
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH):." $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ --step integertransform=off --step p1transform=off --step ilr=off

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
	rm -fr $(PROGS) *.o *.so peasoup_executable* *.peasoup 
