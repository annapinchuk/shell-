CC=gcc
Flags= -g

all: myshell 

run:
	./myshell

myshell: shell.o 
	$(CC) $(Flags) -o myshell shell.o 

shell.o: shell.c
	$(CC) $(Flags) -c shell.c 

.PHONY: clean all

clean:
	rm -f *.o *.a myshell