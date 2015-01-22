
LIB=../../lib/libIRDB-syscall.a

OBJS=syscall.o

CXX=g++ 
EXTRA_CXXFLAGS=-g -c -I. -I../../include -I../../../beaengine/include

all: $(OBJS)

-include $(OBJS:.o=.d)

clean:
	rm -f $(OBJS) *.d

.cpp.o:
	ar rc $(LIB) $@

%.o: %.cpp
	$(CXX) -c  $(EXTRA_CXXFLAGS) $*.cpp
	$(AR) -r $(LIB) $*.o
	@#
	@# build dependencies --  http://scottmcpeak.com/autodepend/autodepend.html
	@#
	$(CXX) -MM $(EXTRA_CXXFLAGS) $*.cpp > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

