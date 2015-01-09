
INC=-I../include -I../third_party/ELFIO/elfio-2.2 -I$(SECURITY_TRANSFORMS_HOME)/include -I$(SECURITY_TRANSFORMS_HOME)/libIRDB/include  -I$(SECURITY_TRANSFORMS_HOME)/beaengine/include



SRCS=zipr.cpp zipr_options.cpp zipr_stats.cpp memory_space.cpp main.cpp
OBJS=$(subst .cpp,.o, $(SRCS))
EXE=zipr.exe

CXX=g++
CXXFLAGS=$(INC) -g  -DCGC
LDFLAGS=-L$(SECURITY_TRANSFORMS_HOME)/lib
LIBS=-lpqxx -lpq -lIRDB-core -lBeaEngine_s_d



all: $(EXE)

-include $(OBJS:.o=.d)

$(EXE): $(OBJS)
	g++ $(LDFLAGS) $(OBJS) $(LIBS) -o $@

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
	rm -f $(OBJS) $(EXE) *.d *.o
