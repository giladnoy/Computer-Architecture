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

typedef struct envVar {
	char* name;
	char* value;
	struct envVar* next;
} envVar;

/*Global variables*/
int debug = 0, pipeline = 0;
pid_t shellpgid = 0;
job* job_list = NULL;
struct termios termios_p;
/*for lab 6*/
int pipefd[2];
envVar* varList = NULL;

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
			defaultSignals();
			redirect(pCmdLine);
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
						defaultSignals();
						redirect(pCmdLine->next);
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

envVar* createVar(envVar* var ,char* name, char* value){
	if (var) {
		if (strcmp(var->name,name) == 0) {
			if (debug) fprintf(stderr,"Replacing variable \"%s\" from value \"%s\" to value \"%s\"\n",var->name,var->value,value);
			var->value = value;
		}
		else {
			var->next = createVar(var->next, name, value);
		}
	}
	else {
		var = (envVar*) malloc(sizeof(envVar));
		var->name = name;
		var->value = value;
		var->next = NULL;
		if (debug) fprintf(stderr,"Creation of new variable \"%s\" with value \"%s\" was successful\n",name,value);
	}
	return var;
}

void printVars() {
	envVar* cur = varList;
	printf("%-10s %s\n","NAME","VALUE");
	while (cur) {
		printf("%-10s %s\n",cur->name,cur->value);
		cur = cur->next;
	}
	printf("\n");
}

char* getValue(char* name) {
	char *value = NULL;
	envVar* cur = varList;
	while (cur) {
		if (strcmp(cur->name, name) == 0) {
			if (debug) fprintf(stderr, "The name $%s has a value of \"%s\"\n", cur->name, cur->value);
			value = cur->value;
			break;
		}
		cur = cur->next;
	}
	return value;
}

int getVars(cmdLine *c) {
	int i = 0, counter = 0;
	char* value;
	while (c->arguments[i]) {
		if (c->arguments[i][0] == '$') {
			value = getValue(&c->arguments[i][1]);
			if (value) {
				if (replaceCmdArg(c,i,value)) {
					if (debug) fprintf(stderr, "Arg %d's value was changed to \"%s\"\n",i,c->arguments[i]);
					counter++;
				}
				else fprintf(stderr,"Variable replacement error: index %d is out of range\n",i);
			}
			else fprintf(stderr,"No value was found for name $%s\n", &c->arguments[i][1]);
			
		}
		if (strcmp(c->arguments[i],"~") == 0) {
			if (replaceCmdArg(c,i,getenv("HOME"))) {
				if (debug) fprintf(stderr, "Arg %d's value was changed to \"%s\"\n",i,c->arguments[i]);
				counter++;
			}
			else fprintf(stderr,"Variable replacement error: index %d is out of range\n",i);
		}
		i++;
	}
	return counter;
}

envVar* deleteVar(envVar* var, char *toDelete) {
	envVar* toGet = NULL;
	if (var) {
		if (strcmp(var->name,toDelete) == 0) {
			toGet = var->next;
			free(var);
			if (debug) fprintf(stderr,"Variable \"%s\" was freed\n",toDelete);
		}
		else {
			var->next = deleteVar(var->next, toDelete);
			toGet = var;
		}
	}
	else {
		fprintf(stderr, "Delete error: variable \"%s\" not found\n",toDelete);
	}
	return toGet;
}

void deleteList() {
	envVar* cur = varList;
	while (cur != NULL) {
		cur = deleteVar(cur, cur->name);
	}
}

void execute(cmdLine *pCmdLine) {
	int counter;
	job* tmp = NULL;
	pipeline = (pCmdLine->next != NULL);/*If the cmd line includes arg "|" there is a list of cmdLines*/
	counter = getVars(pCmdLine);
	if (debug) fprintf(stderr,"%d arguments were modified by an environmental variable\n",counter);
	if (pipeline) {
		dprint("Pipeline mode enabled");
		counter = getVars(pCmdLine->next);
		if (debug) fprintf(stderr,"%d arguments were modified by an environmental variable in 2nd process\n",counter);

	}
	switch (pCmdLine->arguments[0][0]) {
		case 'b':
			if (strcmp(pCmdLine->arguments[0],"bg")==0) {
				tmp = findJobByIndex(job_list, atoi(pCmdLine->arguments[1]));
				runJobInBackground(tmp, 1);
				return;
			}
			break;
		case 'c':
			if (strcmp(pCmdLine->arguments[0],"cd") == 0) {
				if (chdir(pCmdLine->arguments[1])) {
					perror("Change directory error");
				}
				return;
			}
			break;
		case 'd':
			if (strcmp(pCmdLine->arguments[0],"delete") == 0){
				varList = deleteVar(varList, pCmdLine->arguments[1]);
				return;
			}	
		case 'e':
			if (strcmp(pCmdLine->arguments[0],"env") == 0) {
				printVars();
				return;
			}
		case 'f':
			if (strcmp(pCmdLine->arguments[0],"fg") == 0) {
				tmp = findJobByIndex(job_list, atoi(pCmdLine->arguments[1]));
				runJobInForeground(&job_list, tmp, 1, &termios_p, shellpgid);
				return;
			}
			break;
		case 'j':
			if (strcmp(pCmdLine->arguments[0],"jobs")== 0) {
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
		case 's':
			if (strcmp(pCmdLine->arguments[0],"set") == 0)	{
				if (debug) fprintf(stderr,"Setting variable \"%s\" with the value \"%s\"\n",pCmdLine->arguments[1],pCmdLine->arguments[2]);
				varList = createVar(varList, pCmdLine->arguments[1], pCmdLine->arguments[2]);
				return;
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
	
	while (1) {
		if (pipe(pipefd) == -1) {
			perror("Pipe error");
			_exit(1);
		}
		if (debug) fprintf(stderr,"Pipeline opened: input fd:%d, output fd:%d\n",pipefd[0],pipefd[1]);
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
	deleteList();
	return 1;
}