

PROGS=ret_shadow_stack.exe

CC=g++
CFLAGS= -g -Wall
INCLUDE=-I. -I../include -I../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../libtransform/include/ -I../transforms
LIBS=-L../../lib -lxform -lIRDB-core -lIRDB-cfg -lBeaEngine_s_d -lpqxx  -lMEDSannotation -ltransform ../transforms/Rewrite_Utility.o


programs=ret_shadow_stack.exe

.SUFFIXES: .o .c .exe .cpp .hpp

all: $(programs)
	@echo "---------------------------------------------------"
	@echo "- Return Shadow Stack directory -- Build complete -"
	@echo "---------------------------------------------------"

$(all_objs): *.hpp

*.o: *.hpp

.cpp.o: 
	$(CC) $(INCLUDE) $(CFLAGS)  -c $< 

clean:
	rm -f *.o core *.exe

$(programs): ../../lib/*.a


ret_shadow_stack.exe:  rss_driver.o rss_instrument.o
	$(CC) $(CFLAGS) $^ $(INCLUDE) $(LIBS) -o $@

