/*int encrypt(int c, int enc) {
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
	if (input == stdin) task2 message will be printed if no input file used
		fprintf(stdout, "Enter text to encode: \n");
	currChar = fgetc(input);
	if (strlen(fileName) > 0) { task2 user requested printing output to File
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

 task1b
int getEncValue(char *str) {
	if (str[0] == '+')
		return str[1] - '0';
	 str[0] == '-'
	return (-1) * (str[1] - '0');
}

int main(int argc, char **argv) {
	int enc = 0;
	int ind;
	char fileName[30] = "";
	FILE *input = stdin;
	for (ind = 1 ; ind < argc ; ind++) {
		task1c
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
		task2
		if (strcmp(argv[ind],"-o") == 0) {
			fprintf(stdout, "Enter output file: \n");
			fgets(fileName, 30, stdin);
			fileName[strlen(fileName)-1] = '\0'; removing the \n character
		}
	}
	printInput(input, fileName ,enc);
	return 0;
}*/

#include "util.h"

#define SYS_READ 3 /* arg2 file descriptor, arg3 pointer to input buffer, arg4 buffer size max, returns number of bytes received */
#define SYS_WRITE 4 /* arg2 file descriptor, arg3 pointer to output buffer, arg4 count of bytes to send, returns number of bytes sent*/
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define OPEN 5 /* arg2 the file pathname, arg3 file access flags, arg4 file permission (0777, 0644)*/
#define O_WRONLY 1
#define O_RDONLY 0
#define LSEEK 19
#define CLOSE 6 /*arg2 file descriptor */
#define O_CREATE 64

void printDebug(int call, int ret, int debug) {
	char *callSTR;
	char *retSTR;
	if (debug) {
		system_call(SYS_WRITE, STDERR, "ID: ", 4);
		callSTR = itoa(call);
		system_call(SYS_WRITE, STDERR, callSTR, strlen(callSTR));
		system_call(SYS_WRITE, STDERR, ", Return Value: ", 16);
		retSTR = itoa(ret);
		system_call(SYS_WRITE, STDERR, retSTR, strlen(retSTR));
		system_call(SYS_WRITE, STDERR, "\n", 1);
	}
}

void printDebugFiles(char *input, char *output, int debug) {
	if (debug) {
		system_call(SYS_WRITE, STDERR, "Input File: ", strlen("Input File: "));
		system_call(SYS_WRITE, STDERR, input, strlen(input));
		system_call(SYS_WRITE, STDOUT, ", Output File: ", strlen(", Output File: "));
		system_call(SYS_WRITE, STDOUT, output, strlen(output));
		system_call(SYS_WRITE, STDERR, "\n", 1);
	}
}

int main(int argc, char *argv[]) {
	char input[1];
	char *inputName = "stdin";
	char *outputName = "stdout";
	int debug = 0;
	int ret = 0;
	int infile = STDIN;
	int outfile = STDOUT;
	int i;
	for (i = 1 ; i < argc ; i++) {
		if (strcmp(argv[i], "-d") == 0)
			debug = 1;
		if (strcmp(argv[i],"-i") == 0) {
			if (argc > i + 1) {
				inputName = argv[i+1];
				infile = system_call(OPEN, argv[i + 1], O_RDONLY, 0777);
				printDebug(OPEN, infile, debug);
				if (infile < 0) {
					ret = system_call(SYS_WRITE, STDERR, "File cannot be found or opened for reading\n", 43);
					printDebug(SYS_WRITE, ret, debug);
					return 1;
				}
			}
		}
		if (strcmp(argv[i],"-o") == 0) {
			if (argc > i + 1) {
				outputName = argv[i+1];
				outfile = system_call(OPEN, argv[i + 1], O_CREATE + O_WRONLY, 0777);
				printDebug(OPEN, outfile, debug);
				if (outfile < 0) {
					ret = system_call(SYS_WRITE, STDERR, "File cannot be opened for writing\n", 34);
					printDebug(SYS_WRITE, ret, debug);
					return 1;
				}
			}
		}
	}
	ret = system_call(SYS_READ, infile, input, 1);
	printDebug(SYS_READ, ret, debug);
	while (input[0] != '\n' || input[0] > 0) {
		if (input[0] >= 'A' && input[0] <= 'Z')
			input[0] += 32; /*upper case to lower case*/
		ret = system_call(SYS_WRITE, outfile, input, 1);
		printDebug(SYS_WRITE, ret, debug);
		ret = system_call(SYS_READ, infile, input, 1);
		printDebug(SYS_READ, ret, debug);
	}
	ret = system_call(SYS_WRITE, outfile, "\n", 1);
	printDebug(SYS_WRITE, ret, debug);
	if (outfile != STDOUT) { /* file was opened for writing and should be closed */
		ret = system_call(CLOSE, outfile);
		printDebug(CLOSE, ret, debug);
	}
	if (infile != STDIN) { /* file was opened for closing and should be closed */
		ret = system_call(CLOSE, infile);
		printDebug(CLOSE, ret, debug);
	}
	printDebugFiles(inputName, outputName, debug);
	return 0;
} 
