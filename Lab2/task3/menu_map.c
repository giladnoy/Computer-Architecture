#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
struct fun_desc { /*from cs website */
  char *name;
  char (*fun)(char);
};

char censor(char c) {
  if(c == '!')
    return '.';
  else
    return c;
}
 
/*2a: Implement the map function that receives a pointer to a char (a pointer to a char array), an integer, and a pointer to a function. 
Map returns a new array (after allocating space for it), such that each value in the new array is the result of applying the function f on the corresponding character in the input array.*/

char encrypt(char c){
	if (c >= 0x20 && c <= 0x7E)
		c += 3;
	return c;
} /* Gets a char c and returns its encrypted form by adding 3 to its value. 
          If c is not between 0x20 and 0x7E it is returned unchanged */

char decrypt(char c){
	if (c >= 0x20 && c <= 0x7E)
		c -= 3;
	return c;
} /* Gets a char c and returns its decrypted form by reducing 3 to its value. 
            If c is not between 0x20 and 0x7E it is returned unchanged */

char xprt(char c){
	printf("0x%X\n",c);
	return c;
} /* xprt prints the value of c in a hexadecimal representation followed by a 
           new line, and returns c unchanged. */

char cprt(char c){
	if (c >= 0x20 && c <= 0x7E)
		printf("%c\n",c);
	else
		printf(".\n");
	return c;
} /* If c is a number between 0x20 and 0x7E, cprt prints the character of ASCII value c followed 
                    by a new line. Otherwise, cprt prints the dot ('.') character. After printing, cprt returns 
                    the value of c unchanged. */

char my_get(char c){
	return fgetc(stdin);
} /* Ignores c, reads and returns a character from stdin using fgetc. */

char quit(char c){
	exit(0);
	return 0;
} /* Gets a char c, and ends the program using c as the return value */

struct fun_desc functions[] = {
	{ "Censor", censor },
	{ "Encrypt", encrypt },
	{ "Decrypt", decrypt },
	{ "Print hex", xprt },
	{ "print string", cprt },
	{ "Get String", my_get },
	{ "Quit", quit },
	{ NULL, NULL },
};


char* map(char *array, int array_length, char (*f) (char)){
	int i;
  	char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
 	 for (i = 0 ; i < array_length ; i++) {
 	 	mapped_array[i] = f(array[i]);
 	 }
 	 return mapped_array;
}


int main(int argc, char **argv){
	char *carray = (char*)(calloc(5,sizeof(char)));
	int size = 5;
	int c;
	int i = 0;
	char *temp = carray;
	int hbound = sizeof(functions)/sizeof(struct fun_desc)-1;
	while (1) {
		printf("Please choose a function:\n");	
		do {
			printf("%d) %s\n",i,functions[i].name);
			i++;
		} while (functions[i].name != NULL);
		i = 0;
		printf("Option: ");
		c = getchar();
		while (getc(stdin) != '\n');
		c -= 48;
		if (c >= 0 && c <= hbound) {
			printf("Within bounds\n");
			carray = map(carray,size,functions[c].fun);
			free (temp);
			temp = carray;
			printf("DONE.\n\n");
		}
		else {
			printf("\nNot within bounds\n");
			break;
		}
	}
	return 0;
}
 
