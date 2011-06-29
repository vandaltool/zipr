
.SUFFIXES: .exe .cpp 

PROGS=print_variant.exe list_programs.exe create_variant.exe create_variantir.exe read_variantir.exe clone.exe ilr.exe drop_variant.exe generate_spri.exe fill_in_cfg.exe fix_calls.exe

all: $(PROGS)

$(PROGS): ../lib/libIRDB.a

.cpp.exe:  $< ../lib/libIRDB.a
	g++ -g $< -I../include/ -I../../beaengine/include -L ../lib/ -lIRDB -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@

#print_variant: print_variant.cpp ../lib/libIRDB.a
#	g++ -g print_variant.cpp -I../include/ -L ../lib/ -lIRDB -lpqxx -o print_variant 
#
#
#list_programs: list_programs.cpp ../lib/libIRDB.a
#	g++ -g list_programs.cpp -I../include/ -L ../lib/ -lIRDB -lpqxx -o list_programs 

clean:
	rm -f $(PROGS)
