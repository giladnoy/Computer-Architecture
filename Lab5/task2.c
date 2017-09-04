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
#include "JobControl.h"
#include <termios.h>

#define INPUT_MAX 2048

/*Global variables*/
int debug = 0;
pid_t shellpgid = 0;
job* job_list = NULL;
struct termios termios_p;

/*signal handler, announcing ignored signal*/
void nope(int signal){
	char *sigStr = strsignal(signal);
	printf("\nThe signal ""%s"" was ignored\n", sigStr);
}

/*Singal ignore and return to default functions:*/
void ignoreSignals() {
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	signal(SIGQUIT, nope);
	signal(SIGTSTP, nope);
	signal(SIGCHLD, nope);
	if (debug)
		fprintf(stderr, "Signals rehandled\n");
}

void defaultSignals() {
	signal(SIGTTIN,SIG_DFL);
	signal(SIGTTOU,SIG_DFL);
	signal(SIGQUIT,SIG_DFL);
	signal(SIGTSTP,SIG_DFL);
	signal(SIGCHLD,SIG_DFL);
	if (debug)
		fprintf(stderr, "Signals handler set to default\n");
}

/*print new proccess pid to stderr*/
void debugPID(int pid) {
	if (debug)
		fprintf(stderr, "PID: %d\n", pid);
}

/*Print command to stderr*/
void debugCMD(char *cmd) {
	if (debug)
		fprintf(stderr, "Executing Command: %s\n",cmd);
}

void executeFork(cmdLine *pCmdLine) {
	job* tmp;
	pid_t pid;
	tmp = addJob(&job_list, pCmdLine->arguments[0]);    /*add job to job_list*/
	tmp->status = RUNNING; 	
	switch (pid = fork()) {
		case -1:
			fprintf(stderr, "Fork execution failed\n");
			break;
		case 0:
			defaultSignals();
			execvp(pCmdLine->arguments[0],pCmdLine->arguments);
			perror("Error");
			_exit(1);
			break;
		default:
			debugPID(pid);							/*job status = running*/
			if (setpgid(pid,pid))
				perror("Error");
			tmp->pgid = pid; 									/*child pgid is set to its own PID*/
			if (pCmdLine->blocking) {
				runJobInForeground(&job_list, tmp, 1, &termios_p, shellpgid);
			}
			else
				runJobInBackground(tmp, 0);
			break;
	}
}

void execute(cmdLine *pCmdLine) {
	job* tmp = NULL;
	switch (pCmdLine->arguments[0][0]) {
		case 'b':
			if (strcmp(pCmdLine->arguments[0],"bg")==0) {
				tmp = findJobByIndex(job_list, atoi(pCmdLine->arguments[1]));
				runJobInBackground(tmp, 1);
				return;
			}
			break;
		case 'c':
			if (strcmp(pCmdLine->arguments[0],"cd")==0) {
				if (chdir(pCmdLine->arguments[1])) {
					perror("Error");
				}
				return;
			}
			break;
		case 'f':
			if (strcmp(pCmdLine->arguments[0],"fg")==0) {
				tmp = findJobByIndex(job_list, atoi(pCmdLine->arguments[1]));
				runJobInForeground(&job_list, tmp, 1, &termios_p, shellpgid);
				return;
			}
			break;
		case 'j':
			if (strcmp(pCmdLine->arguments[0],"jobs")==0) {
				printJobs(&job_list);
				return;
			}
			break;
		case 'q':
			if (strcmp(pCmdLine->arguments[0],"quit") == 0) {
				freeJobList(&job_list);
				printf("Bye Bye\n");
				exit(0);
			}
			break;
		}
	executeFork(pCmdLine);
}

int main(int argc, char *argv[]) {
	char pathName[PATH_MAX];
	char inputBuffer[INPUT_MAX];
	int i;
	cmdLine *pCmdLine;
	if (argc>1) {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i],"-d") == 0) {
				debug = 1;
				fprintf(stderr, "Debugging enabled\n");
			}
		}
	}
	ignoreSignals();
	shellpgid = getpid();
	if (debug)
		fprintf(stderr, "Shell PID: %d\n", shellpgid);
	if (tcgetattr(STDIN_FILENO, &termios_p)) 			/*Save default terminal attributes*/
		perror("Error");
	if (setpgid(shellpgid, shellpgid))  				/*set group pid of shell to its own PID*/
		perror("Error");
	if (debug) {
		fprintf(stderr, "shell command prompt initialized...\n");
	}
	while (1) {
		fflush(stdout);
		getcwd(pathName, PATH_MAX+1);
		printf("%s> ", pathName);
		fgets(inputBuffer, INPUT_MAX, stdin);
		pCmdLine = parseCmdLines(inputBuffer);
		fflush(stdin);
		debugCMD(pCmdLine->arguments[0]);
		if (pCmdLine == NULL){
			perror("Error: ");
			continue;
		}
		execute(pCmdLine);
	}
	return 1;
}