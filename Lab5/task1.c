#define _GNU_SOURCE /*strsignal was not recognized by compiler,
this method was taken from stackoverflow to solve this problem*/

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "LineParser.h"

#define INPUT_MAX 2048

int debug = 0;

void debugPID(int pid) {
	if (debug)
		fprintf(stderr, "PID: %d\n", pid);
}

void debugCMD(char *cmd) {
	if (debug)
		fprintf(stderr, "Executing Command: %s\n",cmd);
}

void execute(cmdLine *pCmdLine) {
	int pid;
	if (strcmp(pCmdLine->arguments[0],"cd")==0) {
		pid = chdir(pCmdLine->arguments[1]);
		if (pid != 0) {
			fprintf(stderr, "Change Directory operation failed\n");
		}
		return;
	}
	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "Forking execution failed\n");
		_exit(1);
		break;
	case 0:
		debugCMD(pCmdLine->arguments[0]);
		execvp(pCmdLine->arguments[0],pCmdLine->arguments);
		perror("Error");
		_exit(1);
		break;
	default:
		debugPID(pid);
		if (pCmdLine->blocking)
			waitpid(pid, NULL, 0);
		break;
	}
}

void nope(int signal){
	char *sigStr = strsignal(signal);
	printf("\nThe signal ""%s"" was ignored\n", sigStr);
}

int main(int argc, char *argv[]) {
	int i;
	if (argc>1) {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i],"-d") == 0) {
				debug = 1;
				fprintf(stderr, "Debugging enabled\n");
			}
		}
	}
	char pathName[PATH_MAX];
	char inputBuffer[INPUT_MAX];
	cmdLine *currCmd;
	signal(SIGQUIT, nope);
	signal(SIGTSTP, nope);
	signal(SIGCHLD, nope);
	while (1) {
		getcwd(pathName, PATH_MAX+1);
		printf("%s$ ", pathName);
		fgets(inputBuffer, INPUT_MAX, stdin);
		currCmd = parseCmdLines(inputBuffer);
		if (currCmd == NULL){
			perror("Error: ");
			continue;
		}
		if (strcmp(currCmd->arguments[0],"quit") == 0)
			exit(0);
		execute(currCmd);
	}
	return 1;
}