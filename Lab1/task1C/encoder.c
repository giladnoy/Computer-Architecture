#include <stdio.h> 
#include <string.h>

int encrypt(int c, int enc) {
	if (c >= 'a' && c <= 'z') {
		c += enc;
		if (c > 'z')
			c = 96 + (c - 122);
		if (c < 'a')
			c = 123 - (97 - c);
	}
	return c;
}

int toLowerCase(int c, int enc) {
	if (c >= 'A' && c <= 'Z')
		c += 32;
	return encrypt(c,enc);
}

void printInput(FILE *input, int enc) {
	char currChar;
	currChar = fgetc(input);
	while (currChar != EOF) {
		fputc(toLowerCase(currChar, enc),stdout);
		currChar = fgetc(input);
	}
}

/* task1b */
int getEncValue(char *str) {
	if (str[0] == '+')
		return str[1] - '0';
	/* str[0] == '-' */
	return (-1) * (str[1] - '0');
}

int main(int argc, char **argv) {
	int enc = 0;
	int ind;
	FILE *input = stdin;
	for (ind = 1 ; ind < argc ; ind++) {
		if (strcmp(argv[ind],"-i") == 0) {
			if (argc > ind + 1) {
				input = fopen(argv[ind + 1], "r");
				if (!input) {
					fprintf(stderr, "File cannot be found or opened for reading");
					return 1;
				}
			}
		}
		else {
			if ((argv[ind][0] == '-' || argv[ind][0] == '+') && (argv[ind][1] >= '0' && argv[ind][1] <= '9'))
			enc = getEncValue(argv[1]);
		}
	}
	printInput(input ,enc);
	return 0;
}