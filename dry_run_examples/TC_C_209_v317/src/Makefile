# Author: mc2zk
# Reconstruction of how binaries may be built

SS_CC = gcc
COMMON_FLAGS=-O0 -Wall -fmessage-length=0 -fno-stack-protector
DEBUG_FLAGS=-g3
SRCS=business.c

all: business_debug business_release

business_debug.o: $(SRCS)
	$(SS_CC)  $(COMMON_FLAGS) $(DEBUG_FLAGS) -c $< -o $@

business_debug: business_debug.o
	$(SS_CC)  $(COMMON_FLAGS) $(DEBUG_FLAGS) $< -o $@

business_release.o: $(SRCS)
	$(SS_CC)  $(COMMON_FLAGS) -std=c99 -c $< -o $@

business_release: business_release.o
	$(SS_CC)  $(COMMON_FLAGS) -std=c99 $< -o $@

clean:
	rm -f *.o business_debug business_release

