#define _GNU_SOURCE /*strsignal was not recognized by compiler,
this method was taken from stackoverflow to solve this problem*/

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "LineParser.h"
#include "JobControl.h"
#include <termios.h>
#include <fcntl.h>

#define INPUT_MAX 2048

/*Global variables*/
int debug = 0, pipeline = 0;
pid_t shellpgid = 0;
job* job_list = NULL;
struct termios termios_p;
int pipefd[2];/*for lab 6, input and output pipeline fd*/

/*Change of input/output redirection*/
void redirect(cmdLine* pCmdLine) {
	if (pCmdLine->inputRedirect) { /*Argument "<" was used for redirecting input*/
		close(STDIN_FILENO); 
		open(pCmdLine->inputRedirect, O_RDONLY); /*The inputRedirect replaces the stdin fd*/
		if (debug)
			fprintf(stderr,"debug: STDIN was replaced with %s\n",pCmdLine->inputRedirect);
	}
	if (pCmdLine->outputRedirect) {/*Argument ">" was used for redirecting output*/
		close(STDOUT_FILENO);
		open(pCmdLine->outputRedirect,O_WRONLY | O_CREAT | O_APPEND, 0777);/*outputRedirect replaces stdout,
		if file exists, the new data will be appended, otherwise new file will be opened */
		if (debug)
			fprintf(stderr,"debug: STDOUT was replaced with %s\n",pCmdLine->outputRedirect);
	}
}

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

void dprint(char *str){
	if (debug)
		fprintf(stderr, "%s\n",str);
}

void executeFork(cmdLine *pCmdLine) {
	job* tmp;
	pid_t pid,pid2;
	tmp = addJob(&job_list, pCmdLine->arguments[0]);    /*add job to job_list*/
	tmp->status = RUNNING;	
	switch (pid = fork()) {
		case -1:
			fprintf(stderr, "Fork execution failed\n");
			break;
		case 0:
			if (pipeline) {/*The output of the first process will be the input of the 2nd*/
				close(STDOUT_FILENO);
				dup(pipefd[1]); /*The write end of the pipeline will replace stdout fd*/
				close(pipefd[1]);
				dprint("1st process redirected output to pipeline write side and deleted original fd");
			}
			else {
				defaultSignals();
				redirect(pCmdLine);
			}
			execvp(pCmdLine->arguments[0],pCmdLine->arguments);
			perror("Executing program error");
			_exit(1);
			break;
		default:
			debugPID(pid);							/*job status = running*/
			if (setpgid(pid,pid))
				perror("Setting child group pid error");
			tmp->pgid = pid; 									/*child pgid is set to its own PID*/
			if (pCmdLine->blocking || (pipeline && pCmdLine->next->blocking)) {
				runJobInForeground(&job_list, tmp, 1, &termios_p, shellpgid);
			}
			else
				runJobInBackground(tmp, 0);
			if (pipeline) {
				close(pipefd[1]); /*Closing shell writing stream to prevent error reading of EOF*/
				switch (pid2 = fork()) {
					case -1:
						fprintf(stderr, "Fork execution failed\n");
						break;
					case 0:
						close(STDIN_FILENO);
						dup(pipefd[0]); /*the pipeline read stream will replace input stream*/
						close(pipefd[0]);
						dprint("2nd process redirected input to pipeline read side and deleted original pipeline fd");
						execvp(pCmdLine->next->arguments[0],pCmdLine->next->arguments);
						perror("Executing 2nd program error");
						_exit(1);
					default:
						debugPID(pid);
						close(pipefd[0]);
						dprint("Parent process closed both sides of pipeline");
						waitpid(pid, NULL, 0);
						waitpid(pid2, NULL, 0);
						dprint("Finished waiting for children processes");
						break;	
				}
			}
			break;
	}
}

void execute(cmdLine *pCmdLine) {
	job* tmp = NULL;
	pipeline = (pCmdLine->next != NULL);/*If the cmd line includes arg "|" there is a list of cmdLines*/
	if (pipeline) dprint("Pipeline mode enabled");
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
					perror("Change directory error");
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
	tcgetattr(STDIN_FILENO, &termios_p);			/*Save default terminal attributes*/
	setpgid(shellpgid, shellpgid);  				/*set group pid of shell to its own PID*/
	if (debug) {
		fprintf(stderr, "shell command prompt initialized...\n");
	}
	
	if (debug) fprintf(stderr,"Pipeline opened: input fd:%d, output fd:%d",pipefd[0],pipefd[1]);
	while (1) {
		if (pipe(pipefd) == -1) {
			perror("Pipe error");
			_exit(1);
		}
		getcwd(pathName, PATH_MAX+1);
		printf("%s> ", pathName);
		fgets(inputBuffer, INPUT_MAX, stdin);
		pCmdLine = parseCmdLines(inputBuffer);
		debugCMD(pCmdLine->arguments[0]);
		if (!pCmdLine){
			perror("Parse error");
			continue;
		}
		execute(pCmdLine);
		if (!pipeline) { /*if no pipeline was used, the pipes will be closed for new ones to be opened*/
			close(pipefd[0]);
			close(pipefd[1]);
		}
	}
	return 1;
}