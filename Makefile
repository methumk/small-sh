CC = gcc -std=gnu99 -g -Wall

all: smallsh 

smallsh: main.c 
	$(CC) main.c -o smallsh

clean:
	rm -f smallsh