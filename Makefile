
FLAGS= -Wall -ansi -pedantic -errors -std=gnu99

all: Server

Server: Server.o Client.o Graph.o
				 gcc Server.o Graph.o -o server -pthread -lm
				 gcc Client.o -o client
Server.o Client.o Graph.o: Server.c Client.c Graph.c
				 gcc -c ${FLAGS} Server.c Client.c Graph.c
clean:
	rm *.o Server Client 
