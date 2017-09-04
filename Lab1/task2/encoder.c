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

void printInput(FILE *input, char *fileName, int enc) {
	char currChar;
	FILE *output;
	if (input == stdin) /*task2 message will be printed if no input file used */
		fprintf(stdout, "Enter text to encode: \n");
	currChar = fgetc(input);
	if (strlen(fileName) > 0) { /*task2 user requested printing output to File */
		output = fopen(fileName,"w");
		while (currChar != EOF) {
			fputc(toLowerCase(currChar, enc), stdout);
			fputc(toLowerCase(currChar, enc), output);
			currChar = fgetc(input);
		}
	}
	else {
		while (currChar != EOF) {
			fputc(toLowerCase(currChar, enc),stdout);
			currChar = fgetc(input);
		}
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
	char fileName[30] = "";
	FILE *input = stdin;
	for (ind = 1 ; ind < argc ; ind++) {
		/*task1c*/
		if (strcmp(argv[ind],"-i") == 0) {
			if (argc > ind + 1) {
				input = fopen(argv[ind + 1], "r");
				if (!input) {
					fprintf(stderr, "File cannot be found or opened for reading");
					return 1;
				}
			}
		}
		if ((argv[ind][0] == '-' || argv[ind][0] == '+') && (argv[ind][1] >= '0' && argv[ind][1] <= '9'))
			enc = getEncValue(argv[ind]);
		/*task2*/
		if (strcmp(argv[ind],"-o") == 0) {
			fprintf(stdout, "Enter output file: \n");
			fgets(fileName, 30, stdin);
			fileName[strlen(fileName)-1] = '\0'; /*removing the \n character*/
		}
	}
	printInput(input, fileName ,enc);
	return 0;
}