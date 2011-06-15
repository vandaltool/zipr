
all: simple list_programs

simple: simple.cpp ../lib/libIRDB.a
	g++ -g simple.cpp -I../include/ -L ../lib/ -lIRDB -lpqxx -o simple 


list_programs: list_programs.cpp ../lib/libIRDB.a
	g++ -g list_programs.cpp -I../include/ -L ../lib/ -lIRDB -lpqxx -o list_programs 
