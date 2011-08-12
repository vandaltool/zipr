
.SUFFIXES: .exe .cpp 

PROGS=p1_transform_v2.exe

all: $(PROGS)

#$(PROGS): ../lib/libIRDB.a

.cpp.exe:  $< p1_transform_v2.cpp
	g++ -g $< -I../../libIRDB/include/ -I../../beaengine/include -L ../../libIRDB/lib/ -lIRDB -lpqxx -L ../../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d -o $@

#print_variant: print_variant.cpp ../lib/libIRDB.a
#	g++ -g print_variant.cpp -I../include/ -L ../lib/ -lIRDB -lpqxx -o print_variant 
#
#
#list_programs: list_programs.cpp ../lib/libIRDB.a
#	g++ -g list_programs.cpp -I../include/ -L ../lib/ -lIRDB -lpqxx -o list_programs 

clean:
	rm -f $(PROGS)

