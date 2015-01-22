

PROGS=selective_cfi.exe

CXX=g++
INCLUDE=-I. -I../include -I../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../libtransform/include/ -I../transforms
LIBS=-L../../lib -lxform -lIRDB-core -lIRDB-cfg -lBeaEngine_s_d -lpqxx  -lMEDSannotation -ltransform ../transforms/Rewrite_Utility.o
CXXFLAGS= -g -Wall $(INCLUDE)


OBJS=scfi_driver.o scfi_instr.o
programs=selective_cfi.exe

.SUFFIXES: .o .c .exe .cpp .hpp

all: $(programs)
	@echo "---------------------------------------------"
	@echo "- Selective CFI directory -- Build complete -"
	@echo "---------------------------------------------"


-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CXX) -c  $(CXXFLAGS) $*.cpp 
	@#
	@# build dependencies --  http://scottmcpeak.com/autodepend/autodepend.html
	@#
	$(CXX) -MM $(CXXFLAGS) $*.cpp > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp
	
clean:
	rm -f *.o core *.exe

$(programs): ../../lib/*.a

selective_cfi.exe:   $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(INCLUDE) $(LIBS) -o $@

