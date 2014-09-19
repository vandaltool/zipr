
INC=-I../include -I$(SECURITY_TRANSFORMS_HOME)/include -I$(SECURITY_TRANSFORMS_HOME)/libIRDB/include  -I$(SECURITY_TRANSFORMS_HOME)/beaengine/include



SRCS=main.cpp zipr.cpp
OBJS=$(subst .cpp,.o, $(SRCS))
HDRS=*.h ../include/*.h 
EXE=zipr.exe

CXX=g++
CXXFLAGS=$(INC) -g 
LDFLAGS=-L$(SECURITY_TRANSFORMS_HOME)/lib
LIBS=-lpqxx -lpq -lIRDB-core -lBeaEngine_s_d


all: $(EXE)

$(EXE): $(OBJS)
	g++ $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(OBJS): $(HDRS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@



clean:
	rm -f $(OBJS) $(EXE)
