

INCLUDES= -I ../../include -I../include/ -I../../beaengine/include 
LIBS=-L ../lib/ -lIRDB-core -lIRDB-cfg -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d 
OPT=-g
.SUFFIXES: .exe .cpp 

PROGS=print_variant.exe list_programs.exe create_variant.exe create_variantir.exe read_variantir.exe clone.exe ilr.exe \
	drop_variant.exe generate_spri.exe fill_in_cfg.exe fix_calls.exe fill_in_indtargs.exe unfix_calls.exe \
	calc_conflicts.exe find_strings.exe

all: $(PROGS)

$(PROGS): ../../lib/*


fill_in_indtargs.exe: read_ehframe.o fill_in_indtargs.o check_thunks.o
	g++ fill_in_indtargs.o read_ehframe.o check_thunks.o  $(INCLUDES) $(LIBS) $(OPT) -o $@


.o.exe:  $< ../lib/libIRDB-core.a ../lib/libIRDB-cfg.a
	g++ $< $(INCLUDES) $(LIBS) $(OPT)  -o $@

.cpp.o:  $< 
	g++ $< $(INCLUDES) $(LIBS) $(OPT) -o $@ -c

read_ehframe.o: read_ehframe.cpp
	g++ -w -fpermissive $(INCLUDES) $(LIBS)  $(OPT) $^ -o $@ -c

clean:
	rm -f $(PROGS) *.o
