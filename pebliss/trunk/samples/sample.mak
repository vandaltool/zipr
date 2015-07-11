PWD=$(shell pwd)
OUTDIR = ../out/
LIBPATH = ../../lib/libpebliss.a
NAME=$(shell basename $(PWD))
CXXFLAGS =  -Wall -I../../pe_lib

ifdef PE_DEBUG
CXXFLAGS  += -g -O0
endif

all: $(OUTDIR)$(NAME)

clean:
	rm -f $(NAME) *.o
	rm -f $(OUTDIR)$(NAME)

$(NAME): main.o
	$(CXX) -Wall $^ -lpebliss -L../../lib -o $(NAME) -liconv

main.o: $(LIBPATH)

$(OUTDIR)$(NAME): $(NAME)
	cp -d $(NAME) $(OUTDIR)

