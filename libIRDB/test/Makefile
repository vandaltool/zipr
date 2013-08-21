
.SUFFIXES: .exe .cpp 

PROGS=print_variant.exe list_programs.exe create_variant.exe create_variantir.exe read_variantir.exe clone.exe ilr.exe \
	drop_variant.exe generate_spri.exe fill_in_cfg.exe fix_calls.exe fill_in_indtargs.exe unfix_calls.exe \
	calc_conflicts.exe find_strings.exe

all: $(PROGS)

$(PROGS): ../lib/libIRDB-core.a ../lib/libIRDB-cfg.a  ../../lib/libBeaEngine_s_d.a

#read_ehframe.exe: unwind-pe.h 
#	g++ -w -fpermissive -g read_ehframe.cpp -DTEST -I../include/ -I../../beaengine/include -L ../lib/ -lIRDB-core -lIRDB-cfg -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@

fill_in_indtargs.exe: read_ehframe.o fill_in_indtargs.o check_thunks.o
	g++ -g fill_in_indtargs.o read_ehframe.o check_thunks.o -I../include/ -I../../beaengine/include -L ../lib/ -lIRDB-core -lIRDB-cfg -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@


.o.exe:  $< ../lib/libIRDB-core.a ../lib/libIRDB-cfg.a
	g++ -g $< -I../include/ -I../../beaengine/include -L ../lib/ -lIRDB-core -lIRDB-cfg -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@

.cpp.o:  $< 
	g++ -g $< -I../include/ -I../../beaengine/include -I../../include -L../lib/ -lIRDB-core -lIRDB-cfg -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@ -c

read_ehframe.o: read_ehframe.cpp
	g++ -w -fpermissive -g $^ -I../../include -I../include/ -I../../beaengine/include -L ../lib/ -lIRDB-core -lIRDB-cfg -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@ -c

clean:
	rm -f $(PROGS) *.o
