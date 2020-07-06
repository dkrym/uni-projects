PROGRAMS=webinfo
CC=g++
CFLAGS=-std=c++98 -Wall -pedantic -g

all: $(PROGRAMS)

webinfo: webinfo.o error.o
	$(CC) $(CFLAGS) -o $@  webinfo.o error.o

webinfo.o: webinfo.cpp webinfo.h
	$(CC) $(CFLAGS) -c $< -o $@

error.o: error.cpp error.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean clean-exe clean-all

clean: 
	rm -f *.log *.out *.toc *.aux *.o *.log *~ 

cleanexe:
	rm -f $(PROGRAMS)

cleanall: clean cleanexe
