# Projekt IPK3
# Autor: David Krym
# Datum: 22.4.2011

CLIENT=rdtclient
SERVER=rdtserver
CC=gcc
CFLAGS= -std=gnu99 -Wall -Wextra

all: $(CLIENT) $(SERVER)

rdtclient: rdtclient.o
	$(CC) $(CFLAGS) -o $@  rdtclient.o

rdtserver: rdtserver.o list.o
	$(CC) $(CFLAGS) -o $@  rdtserver.o list.o

rdtclient.o: rdtclient.c rdtclient.h
	$(CC) $(CFLAGS) -c $< -o $@

rdtserver.o: rdtserver.c rdtserver.h
	$(CC) $(CFLAGS) -c $< -o $@

list.o: list.c list.h
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean clean-exe clean-all

clean: 
	rm -f *.log *.out *.toc *.aux *.o *.log *~ 

cleanexe:
	rm -f $(SERVER) $(CLIENT)

cleanall: clean cleanexe
