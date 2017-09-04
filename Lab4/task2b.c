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
#define BUF_SIZE 8192
#define GETDENTS 141 /*arg2 file source, arg3 target buffer, arg4 max byte to get*/

typedef struct ent {
	int inode; /*struct code*/
	int offset; /*offset of next struct */
	short len; /*length of current struct */
	char buf[]; /*pointer to name of file */
}ent;

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

void printDirentDebug(char* name, int length, int debug) {
	char *lengthStr;
	if (debug) {
		system_call(SYS_WRITE, STDERR, "Dirent Name: ", strlen("Dirent Name: "));
		system_call(SYS_WRITE, STDERR, name, strlen(name));
		system_call(SYS_WRITE, STDERR, ", Dirent length: ", strlen(", Dirent length: "));
		lengthStr = itoa(length);
		system_call(SYS_WRITE, STDERR, lengthStr, strlen(lengthStr));
		system_call(SYS_WRITE, STDERR, "\n", 1);
	}
}

int main(int argc, char *argv[]){
	char suffix = 0;
	char direntBuff[BUF_SIZE];
	char endChar = 0;
	int debug = 0;
	int dir = 0;
	int byteCounter = 32;
	int totalbytes = 0;
	int ret;
	int i;
	ent *currDirent;
	for (i = 1 ; i < argc ; i++) {
		if (strcmp(argv[i], "-d") == 0)
			debug = 1;
		if (strcmp(argv[i], "-s") == 0) {
			if (argc > i + 1) {
				suffix = argv[i + 1][0];
			}
		}
	}
	dir = system_call(OPEN, ".", O_RDONLY, 0777);
	printDebug(OPEN, dir, debug);
	totalbytes = system_call(GETDENTS, dir, direntBuff, BUF_SIZE);
	printDebug(GETDENTS, totalbytes, debug);
	ret = system_call(SYS_WRITE, STDOUT, "File names: \n", strlen("File names: \n"));
	printDebug(SYS_WRITE, ret, debug);
	while (byteCounter < totalbytes) {
		currDirent = (ent*) (direntBuff + byteCounter);
		endChar = currDirent->buf[strlen(currDirent->buf) - 1]; /*final character of descr. of current dent*/
		if (suffix != 0 && suffix != endChar){ /* if "-s" option is enabled and the last char is not equal to the suffix received as argument*/
			byteCounter += currDirent->len;
			continue;
		}
		ret = system_call(SYS_WRITE, STDOUT, currDirent->buf, strlen(currDirent->buf));
		printDebug(SYS_WRITE, ret, debug);
		printDirentDebug(currDirent->buf, currDirent->len, debug);
		ret = system_call(SYS_WRITE, STDOUT, "\n", 1);
		printDebug(SYS_WRITE, ret, debug);
		byteCounter += currDirent->len;
	}
	return 0;
}