

PROGS=fix_rets.exe

CXX=g++
CXXFLAGS= 
INCLUDE=-I. -I../include -I../xform -I../../beaengine/include -I../../libIRDB/include/ -I../../libMEDSannotation/include/ -I../libtransform/include/ -I../transforms
CXXFLAGS=   $(INCLUDE) -Wall
LIBS=-L../../lib -lxform -lIRDB-core -lIRDB-cfg -lBeaEngine_s_d -lpqxx  -lMEDSannotation -ltransform# ../transforms/Rewrite_Utility.o


OBJS=fix_rets.o fix_rets_driver.o
programs=fix_rets.exe

.SUFFIXES: .o .c .exe .cpp .hpp

all: $(programs)
	@echo "---------------------------------------------"
	@echo "- Fix Rets directory -- Build complete -"
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

fix_rets.exe:   $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(INCLUDE) $(LIBS) -o $@

