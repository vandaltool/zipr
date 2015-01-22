CC=g++
CFLAGS= -g -Wall -DCGC
INCLUDE=-I. -I../include -I../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../libtransform/include/ -I../transforms -I../../include
LIBS=-L../../lib -lxform -lIRDB-core -lIRDB-cfg -lIRDB-util -lIRDB-syscall -lBeaEngine_s_d -lpqxx  -lMEDSannotation -ltransform ../transforms/Rewrite_Utility.o

program=cinderella_prep.exe

all: $(program)
	@echo "-----------------------------------------------"
	@echo "- CGC libc dynamic inference engine -- Build complete -"
	@echo "-----------------------------------------------"

OBJS=cinderella_prep.o cinderella_prep_driver.o


.SUFFIXES: .o .c .exe .cpp .hpp

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CC) $(INCLUDE) $(CFLAGS)  -c $< 
	@#
	@# build dependencies --  http://scottmcpeak.com/autodepend/autodepend.html
	@#
	$(CC) -MM $(INCLUDE) $(CFLAGS) $*.cpp > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp


clean:
	rm -f *.o core *.exe *.d

$(program): ../../lib/*.a

$(program):  $(OBJS)
	$(CC) $(CFLAGS) $^ $(INCLUDE) $(LIBS) -o $@

