INCLUDES= -I../../../../include -I../../../include/ -I../../../../beaengine/include 
LIBS=-L ../../../lib/ -lIRDB-core -lIRDB-cfg -lIRDB-util -lpqxx -L ../../../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d 
#OPT=-g -fPIE -fPIC -O2
# OPT=-g -O2  oops no switch tables generated with -O2
OPT=-g  -O
.SUFFIXES: .exe .cpp .peasoup

PROGS=jmp1.exe jmp2.exe
PEASOUP_PROGS=jmp1.peasoup jmp2.peasoup

all: $(PROGS)
peasoup: $(PEASOUP_PROGS)

$(PROGS): ../../../../lib/*

.exe.peasoup: $<
	$(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ --step integertransform=off --step p1transform=off --step ilr=off

.o.exe:  $< ../lib/libIRDB-core.a ../lib/libIRDB-cfg.a
	g++ $< $(INCLUDES) $(LIBS) $(OPT)  -o $@

.cpp.o:  $< 
	g++ $< $(INCLUDES) $(LIBS) $(OPT) -o $@ -c

clean:
	rm -fr $(PROGS) *.o peasoup_executable* *.peasoup
