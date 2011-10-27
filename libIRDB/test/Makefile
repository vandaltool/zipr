
.SUFFIXES: .exe .cpp 

PROGS=print_variant.exe list_programs.exe create_variant.exe create_variantir.exe read_variantir.exe clone.exe ilr.exe \
	drop_variant.exe generate_spri.exe fill_in_cfg.exe fix_calls.exe fill_in_indtargs.exe unfix_calls.exe calc_conflicts.exe read_ehframe.exe

all: $(PROGS)

$(PROGS): ../lib/libIRDB-core.a ../lib/libIRDB-cfg.a 

read_ehframe.exe: unwind-pe.h

.cpp.exe:  $< ../lib/libIRDB-core.a ../lib/libIRDB-cfg.a
	g++ -g $< -I../include/ -I../../beaengine/include -L ../lib/ -lIRDB-core -lIRDB-cfg -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@

clean:
	rm -f $(PROGS)
