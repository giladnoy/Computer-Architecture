all: task1c

task1ca: task1c.o
	gcc -m32 -g -Wall -o task1c task1c.o 

task1ca.o: task1c.c
	gcc -m32 -g -ansi -Wall -c -o task1c.o task1c.c

clean:
	rm -f *.o task1c