CC=gcc
CFLAGS=-I.

filesys: filesys.o shell.o
	$(CC) -o filesys filesys.o shell.o
