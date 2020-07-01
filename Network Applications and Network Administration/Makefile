#
# ISA projekt: ndpv6
# @author David Krym xkrymd00@stud.fit.vutbr.cz
# @file Makefile
#

PROGRAMS=ndpv6
CC=gcc
CFLAGS=-std=gnu99 -Wall -pedantic 

all: CFLAGS += -lpcap
all: $(PROGRAMS)


ndpv6: main.o ndpv6_printer.o ndpv6_packet.o ndpv6_neigh_cache.o ndpv6_ipv6.o ndpv6_if.o ndpv6_icmpv6.o
	$(CC) $(CFLAGS) -o $@ main.o ndpv6_printer.o ndpv6_packet.o ndpv6_neigh_cache.o ndpv6_ipv6.o ndpv6_if.o ndpv6_icmpv6.o

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@  

ndpv6_printer.o: ndpv6_printer.c ndpv6_printer.h
	$(CC) $(CFLAGS) -c $< -o $@  

ndpv6_packet.o: ndpv6_packet.c ndpv6_packet.h
	$(CC) $(CFLAGS) -c $< -o $@  

ndpv6_neigh_cache.o: ndpv6_neigh_cache.c ndpv6_neigh_cache.h
	$(CC) $(CFLAGS) -c $< -o $@  

ndpv6_ipv6.o: ndpv6_ipv6.c ndpv6_ipv6.h
	$(CC) $(CFLAGS) -c $< -o $@  

ndpv6_if.o: ndpv6_if.c ndpv6_if.h
	$(CC) $(CFLAGS) -c $< -o $@  

ndpv6_icmpv6.o: ndpv6_icmpv6.c ndpv6_icmpv6.h
	$(CC) $(CFLAGS) -c $< -o $@  



manual.pdf: dokumentace/manual.tex
	pdflatex dokumentace/manual.tex
	pdflatex dokumentace/manual.tex

.PHONY: pack clean clean-exe clean-all

pack: *.c *.h Makefile manual.pdf Readme 
	tar cf xkrymd00.tar *.c *.h Makefile manual.pdf Readme

clean: 
	rm -f *.log *.out *.toc *.aux *.o *.log *~ 

cleanexe:
	rm -f $(PROGRAMS) *.tar manual.pdf

cleanall: clean cleanexe
