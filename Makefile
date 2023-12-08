# set compiler
CC = g++

CFLAGS = -g -Wall -Wextra -O2

all: main

clean:
	$(RM) *.o
	$(RM) *.gch


client: client.o hashtable.o
	$(CC) $(CFLAGS) -o client client.o hashtable.o

server: server.o hashtable.o
	$(CC) $(CFLAGS) -o server server.o hashtable.o

client.o: client.cpp hashtable.hpp
	$(CC) $(CFLAGS) -c client.cpp

hashtable.o: hashtable.cpp hashtable.hpp
	$(CC) $(CFLAGS) -c hashtable.cpp

server.o: server.cpp hashtable.hpp
	$(CC) $(CFLAGS) -c server.cpp

