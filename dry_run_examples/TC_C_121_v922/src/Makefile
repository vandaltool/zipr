# Author: mc2zk
# Reconstruction of how binaries may be built

SS_CC = gcc
SS_CFLAGS=-static
COMMON_FLAGS=-O0 -Wall -fmessage-length=0 -fno-stack-protector
DEBUG_FLAGS=-g3
SRCS=solitaire.c

all: solitaire_debug solitaire_release

solitaire_debug.o: $(SRCS)
	$(SS_CC)  $(COMMON_FLAGS) $(DEBUG_FLAGS) -c $< -o $@

solitaire_debug: solitaire_debug.o
	$(SS_CC)  $(COMMON_FLAGS) $(SS_CFLAGS) $(DEBUG_FLAGS) $< -o $@

solitaire_release.o: $(SRCS)
	$(SS_CC)  $(COMMON_FLAGS) -std=c99 -c $< -o $@

solitaire_release: solitaire_release.o
	$(SS_CC)  $(COMMON_FLAGS) $(SS_CFLAGS) -std=c99 $< -o $@

clean:
	rm -f *.o solitaire_debug solitaire_release

