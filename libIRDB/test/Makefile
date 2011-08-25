
.SUFFIXES: .exe .cpp 

PROGS=print_variant.exe list_programs.exe create_variant.exe create_variantir.exe read_variantir.exe clone.exe ilr.exe drop_variant.exe generate_spri.exe fill_in_cfg.exe fix_calls.exe fill_in_indtargs.exe

all: $(PROGS)

$(PROGS): ../lib/libIRDB-core.a

.cpp.exe:  $< ../lib/libIRDB-core.a
	g++ -g $< -I../include/ -I../../beaengine/include -L ../lib/ -lIRDB-core -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@

clean:
	rm -f $(PROGS)
