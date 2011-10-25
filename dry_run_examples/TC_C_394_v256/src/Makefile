# Author: mc2zk
# Reconstruction of how binaries may be built

SS_CC = gcc
SS_CFLAGS = -static
COMMON_FLAGS=-O0 -Wall -fmessage-length=0 -fno-stack-protector
DEBUG_FLAGS=-g3
SRCS=business.c interfile.c interfile_2.c
DEBUG_OBJS=business_debug.o interfile_debug.o interfile_2_debug.o business_debug.o
RELEASE_OBJS=interfile_2_release.o interfile_release.o business_release.o

all: business_debug business_release

%_debug.o: %.c
	$(SS_CC)  $(COMMON_FLAGS) $(DEBUG_FLAGS) -c $< -o $@

business_debug: $(DEBUG_OBJS)
	$(SS_CC)  $(COMMON_FLAGS) $(SS_CFLAGS) $(DEBUG_FLAGS) $? -o $@

%_release.o: %.c
	$(SS_CC)  $(COMMON_FLAGS) -std=c99 -c $< -o $@

business_release: $(RELEASE_OBJS)
	$(SS_CC)  $(COMMON_FLAGS) $(SS_CFLAGS) -std=c99 $? -o $@

clean:
	rm -f *.o business_debug business_release

