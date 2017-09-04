#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int debug = 0;

void dprint(char *str) { /*debug print*/
	if (debug)
		fprintf(stderr,"%s\n",str);
}

int main(int argc, char *argv[]) {
	int pipefd[2];
	int pid1,pid2, writefd, readfd, i;
	if (argc>1) {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i],"-d") == 0) {
				debug = 1;
				fprintf(stderr, "Debugging enabled\n");
			}
		}
	}
	if (pipe(pipefd) == -1) {
		perror("Pipe error");
		_exit(1);
	}
	dprint("(parent_process>forking…)");
	switch (pid1 = fork()) {
		case -1:
			fprintf(stderr, "1st Fork execution failed\n");
			_exit(1);
			break;
		case 0:
			close(STDOUT_FILENO);
			writefd = dup(pipefd[1]);
			dprint("(child1>redirecting stdout to the write end of the pipe…)");
			close(pipefd[1]);
			dprint("(child1>going to execute cmd: ls -l)");
			execvp("ls", (char*[]){"ls","-l",0});
			fprintf(stderr,"Error: 1st child process failed to execute command, quitting...\n");
			_exit(1);
			break;
		default:
			if (debug) fprintf(stderr,"(parent_process>created process with id: %d)\n",pid1);
			dprint("(parent_process>closing the write end of the pipe…)");
			close(pipefd[1]);
			dprint("(parent_process>forking…)");
			switch (pid2 = fork()) {
				case -1:
					fprintf(stderr, "2nd Fork execution failed\n");
					_exit(1);
					break;
				case 0:
					close(STDIN_FILENO);
					dprint("(child2>redirecting stdin to the read end of the pipe…)");
					readfd = dup(pipefd[0]);
					close(pipefd[0]);
					dprint("(child2>going to execute cmd: tail -n 2)");
					execvp("tail",(char*[]){"tail","-n","2",0});
					fprintf(stderr,"Error: 2st child process failed to execute command, quitting...\n");
					_exit(1);
					break;
				default:
					if (debug) fprintf(stderr,"(parent_process>created process with id: %d)\n",pid2);
					dprint("(parent_process>closing the read end of the pipe…)");
					close(pipefd[0]);
					dprint("(parent_process>waiting for child processes to terminate…)");
					waitpid(pid1,NULL,0);
					waitpid(pid2,NULL,0);
					dprint("(parent_process>exiting…)");
					break;	
			}
			break;	
	}
	return 0;
}