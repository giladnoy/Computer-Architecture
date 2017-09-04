all: menu_map

menu_map: menu_map.o
	gcc -m32 -g -Wall -o menu_map menu_map.o

menu_map.o: menu_map.c
	gcc -m32 -g -ansi -Wall -c -o menu_map.o menu_map.c

clean:
	rm -f *.o menu_map 
