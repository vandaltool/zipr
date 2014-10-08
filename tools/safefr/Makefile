

INCLUDES= -I $(SECURITY_TRANSFORMS_HOME)/include -I$(SECURITY_TRANSFORMS_HOME)/beaengine/include -I $(SECURITY_TRANSFORMS_HOME)/libIRDB/include/ -I$(SECURITY_TRANSFORMS_HOME)/libMEDSannotation/include/ 
LIBS= -L$(SECURITY_TRANSFORMS_HOME)/lib -lIRDB-core -lIRDB-cfg -lpqxx -L $(SECURITY_TRANSFORMS_HOME)/beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -lMEDSannotation

OPT=-g
.SUFFIXES: .exe .cpp 

PROGS=fill_in_safefr.exe 

all: $(PROGS)

$(PROGS): ../../lib/*


.o.exe:  $< $(SECURITY_TRANSFORMS_HOME)/lib/*.a
	g++ $< $(INCLUDES) $(LIBS) $(OPT)  -o $@

.cpp.o:  $< 
	g++ $< $(INCLUDES) $(LIBS) $(OPT) -o $@ -c

clean:
	rm -f $(PROGS) *.o
