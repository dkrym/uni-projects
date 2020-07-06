CLIENT=client
SERVER=server
CC=g++
CFLAGS=-std=c++98 -Wall -pedantic -g

all: $(CLIENT) $(SERVER)

client: client.o error.o
	$(CC) $(CFLAGS) -o $@  client.o error.o

server: server.o error.o
	$(CC) $(CFLAGS) -o $@  server.o error.o

client.o: client.cpp
	$(CC) $(CFLAGS) -c $< -o $@

server.o: server.cpp
	$(CC) $(CFLAGS) -c $< -o $@

error.o: error.cpp error.h
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean clean-exe clean-all

clean: 
	rm -f *.log *.out *.toc *.aux *.o *.log *~ 

cleanexe:
	rm -f $(SERVER) $(CLIENT)

cleanall: clean cleanexe
