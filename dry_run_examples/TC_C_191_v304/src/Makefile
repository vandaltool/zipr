# Author: mc2zk
# Reconstruction of how binaries may be built

SS_CC = gcc
SS_CFLAGS = -static
COMMON_FLAGS=-O0 -Wall -fmessage-length=0 -fno-stack-protector
DEBUG_FLAGS=-g3
SRCS=recaman.c

all: recaman_debug recaman_release

recaman_debug.o: $(SRCS)
	$(SS_CC)  $(COMMON_FLAGS) $(DEBUG_FLAGS) -c $< -o $@

recaman_debug: recaman_debug.o
	$(SS_CC)  $(COMMON_FLAGS) $(SS_CFLAGS) $(DEBUG_FLAGS) $< -o $@

recaman_release.o: $(SRCS)
	$(SS_CC)  $(COMMON_FLAGS) -std=c99 -c $< -o $@

recaman_release: recaman_release.o
	$(SS_CC)  $(COMMON_FLAGS) $(SS_CFLAGs) -std=c99 $< -o $@ 

clean:
	rm -f *.o recaman_debug recaman_release
