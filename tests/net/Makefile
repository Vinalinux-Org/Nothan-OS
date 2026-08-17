# tests/net — the protocol stack compiled for the host and judged by a script.
#
# Deliberately not part of the kernel build: this links with the host compiler
# against host stubs, and mixing the two toolchains in one Makefile is the
# first step towards mixing them in one binary.
#
#   make -C tests/net check

TOP     := ../..
KERNEL  := $(TOP)/nothan-kernel

CC      := gcc
CFLAGS  := -O1 -g -Wall -Wextra -Wno-unused-parameter \
           -I stub -I $(KERNEL)/include

SRCS    := harness.c \
           $(KERNEL)/net/net_core.c \
           $(KERNEL)/net/arp.c \
           $(KERNEL)/net/ipv4.c \
           $(KERNEL)/net/icmp.c \
           $(KERNEL)/net/udp.c

harness: $(SRCS) stub/nothan/wait.h stub/asm/barrier.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

check: harness
	@python3 test_net.py

clean:
	rm -f harness

.PHONY: check clean
