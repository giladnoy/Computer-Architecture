#include "JobControl.h"

/**
* Receive a pointer to a job list and a new command to add to the job list and adds it to it.
* Create a new job list if none exists.
**/
job* addJob(job** job_list, char* cmd){
	job* job_to_add = initializeJob(cmd);
	
	if (*job_list == NULL){
		*job_list = job_to_add;
		job_to_add -> idx = 1;
	}	
	else{
		int counter = 2;
		job* list = *job_list;
		while (list -> next !=NULL){
			printf("adding %d\n", list->idx);
			list = list -> next;
			counter++;
		}
		job_to_add ->idx = counter;
		list -> next = job_to_add;
	}
	return job_to_add;
}


/**
* Receive a pointer to a job list and a pointer to a job and removes the job from the job list 
* freeing its memory.
**/
void removeJob(job** job_list, job* tmp){
	if (*job_list == NULL)
		return;
	job* tmp_list = *job_list;
	if (tmp_list == tmp){
		*job_list = tmp_list -> next;
		freeJob(tmp);
		return;
	}
		
	while (tmp_list->next != tmp){
		tmp_list = tmp_list -> next;
	}
	tmp_list -> next = tmp -> next;
	freeJob(tmp);
	
}

/**
* receives a status and prints the string it represents.
**/
char* statusToStr(int status)
{
  static char* strs[] = {"Done", "Suspended", "Running"};
  return strs[status + 1];
}


/**
*   Receive a job list, and print it in the following format:<code>[idx] \t status \t\t cmd</code>, where:
    cmd: the full command as typed by the user.
    status: Running, Suspended, Done (for jobs that have completed but are not yet removed from the list).
  
**/
void printJobs(job** job_list){

	job* tmp = *job_list;
	updateJobList(job_list, FALSE);
	while (tmp != NULL){
		printf("[%d]\t %s \t\t %s", tmp->idx, statusToStr(tmp->status),tmp -> cmd); 
		if (tmp -> cmd[strlen(tmp -> cmd)-1]  != '\n')
			printf("\n");
		job* job_to_remove = tmp;
		tmp = tmp -> next;
		if (job_to_remove->status == DONE)
			removeJob(job_list, job_to_remove);
		
	}
 
}


/**
* Receive a pointer to a list of jobs, and delete all of its nodes and the memory allocated for each of them.
*/
void freeJobList(job** job_list){
	while(*job_list != NULL){
		job* tmp = *job_list;
		*job_list = (*job_list) -> next;
		freeJob(tmp);
	}
	
}


/**
* receives a pointer to a job, and frees it along with all memory allocated for its fields.
**/
void freeJob(job* job_to_remove){
	kill(job_to_remove->pgid,SIGKILL); 	/*action was added to halt all running jobs*/
	free (job_to_remove->cmd);
	free (job_to_remove->tmodes);
	free (job_to_remove);
}



/**
* Receive a command (string) and return a job pointer. 
* The function needs to allocate all required memory for: job, cmd, tmodes
* to copy cmd, and to initialize the rest of the fields to NULL: next, pigd, status 
**/

job* initializeJob(char* cmd){
	job* ret;
	ret = (job*) malloc(sizeof(job));
	ret->cmd = cmd;
	ret->idx=0;
	ret->pgid=0;
	ret->status=0;
	ret->tmodes= (struct termios*)malloc(sizeof(struct termios));
	ret->next=NULL;
	return ret;
}


/**
* Receive a job list and and index and return a pointer to a job with the given index, according to the idx field.
* Print an error message if no job with such an index exists.
**/
job* findJobByIndex(job* job_list, int idx){
  job* currJob = job_list;
  while (currJob!=NULL) {
  	if (currJob->idx == idx)
  		break;
  	currJob=currJob->next;
  }
  return currJob;
}


/**
* Receive a pointer to a job list, and a boolean to decide whether to remove done
* jobs from the job list or not. 
**/
void updateJobList(job **job_list, int remove_done_jobs){
	job* tmp = *job_list;
	while (tmp != NULL) {
		if (tmp->status == RUNNING) {
			if (waitpid(tmp->pgid,NULL,WNOHANG)){ /*check if proccess is not running*/
				tmp->status = DONE;
				if (remove_done_jobs) {
					printf("[%d]\t %s \t\t %s", tmp->idx, statusToStr(tmp->status),tmp -> cmd); 
					if (tmp -> cmd[strlen(tmp -> cmd)-1]  != '\n')
						printf("\n");
					job* job_to_remove = tmp;
					tmp = tmp -> next;
					if (job_to_remove->status == DONE)
						removeJob(job_list, job_to_remove);
				}
			}
		}
		tmp = tmp->next;
	}
}

/** 
* Put job j in the foreground.  If cont is nonzero, restore the saved terminal modes and send the process group a
* SIGCONT signal to wake it up before we block.  Run updateJobList to print DONE jobs.
**/

void runJobInForeground (job** job_list, job *j, int cont, struct termios* shell_tmodes, pid_t shell_pgid){
	int status = 0;
	if (j == NULL) {
		printJobs(job_list);
		return;
	}
	if (waitpid(j->pgid,&j->status,WNOHANG)) { /*nonzero return value means the job is done*/
		updateJobList(job_list, TRUE);
	}
	else {
		if (tcsetpgrp(STDIN_FILENO, j->pgid))  					/*put job in foreground*/
			perror("Error");
		if (cont && (j->status == SUSPENDED)){
			if (tcsetattr(STDIN_FILENO, TCSADRAIN, j->tmodes))  /*setting the attributes of the terminal to the job's*/
			perror("Error"); 
			kill(j->pgid, SIGCONT); 							/*continue job if halted*/		
		}
		waitpid(j->pgid,&status,WUNTRACED); 
		if (WIFSTOPPED(status)) { 						/*checking if job received Ctrl^Z (SIGTSTP) */
			j->status = SUSPENDED;
		}			
		else {
			j->status = DONE;
		}
		
		if (tcsetpgrp (STDIN_FILENO, shell_pgid)) 				/*putting the shell back in the foreground*/
			perror("Error");
		if (tcgetattr(STDIN_FILENO,j->tmodes))					/*Save current terminal attributes to job tmode*/
			perror("Error");
		tcsetattr(STDIN_FILENO, TCSADRAIN, shell_tmodes); 		/*restore terminal's attributes*/
		updateJobList(job_list, TRUE);
	}
}

/** 
* Put a job in the background.  If the cont argument is nonzero, send
* the process group a SIGCONT signal to wake it up.  
**/

void runJobInBackground (job *j, int cont){	
	j->status = RUNNING;
	if (cont)
		kill(j->pgid, SIGCONT); 
}
