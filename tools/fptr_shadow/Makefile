PROGS=fptr_shadow64.exe

CC=g++
CFLAGS= -g -Wall
INCLUDE=-I. -I../include -I../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../libtransform/include/ -I../transforms
LIBS=-L../../lib -lxform -lIRDB-core -lIRDB-cfg -lBeaEngine_s_d -lpqxx  -lMEDSannotation -ltransform ../transforms/Rewrite_Utility.o


programs=fptr_shadow64.exe

.SUFFIXES: .o .c .exe .cpp .hpp

all: $(programs)
	@echo "-------------------------------------------------------"
	@echo "- Function Pointer Shadow directory -- Build complete -"
	@echo "-------------------------------------------------------"

$(all_objs): *hpp

.cpp.o: *.hpp
	$(CC) $(INCLUDE) $(CFLAGS)  -c $< 

clean:
	rm -f *.o core *.exe

$(programs): ../../lib/*.a


fptr_shadow64.exe:  fptr_shadow_driver.o fptr_shadow_instrument64.o
	$(CC) $(CFLAGS) $^ $(INCLUDE) $(LIBS) -o $@

