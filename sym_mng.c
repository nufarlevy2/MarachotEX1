#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

//Decleration for functions
bool checkForInt(char *str2Check);
void insertChildsToAnArray(int* array, int ppid);

int main(int argc, char* argv[]) {
	//DEFINITIONS part 1
	pid_t rootProgPID = getpid();
	FILE *openedFile;
	char symbol;
	//First checkup
	if (argc != 4) {
		printf("\n%s\n%s\n","wrong use in function! please insert",
				"./prog <filePath> <charToLookFor> <terminationBound>");
		exit(EXIT_FAILURE);
	}
	//DEFINITIONS part 2
	char *currentBuffer = calloc(64, sizeof(char));
	char *filePath = argv[1];
	char* charArraySearchQuery = argv[2];
	int* PIDs = (int*)malloc(strlen(charArraySearchQuery)*sizeof(int));
	int* stopedCounter = (int*)malloc(strlen(charArraySearchQuery)*sizeof(int));
	int PIDsPointer = 0;
	int PIDsSize = 0;
	int terminationBound = 1000;
	int pid;
	int wPIDstatus;
	int *wstatus = calloc(1,sizeof(int));
	//All checks
	if (access(filePath,R_OK) != 0) {
		printf("\n%s\n","File does not exist or you do not have the read permissions");
		free(currentBuffer);
	        free(PIDs);
	        free(stopedCounter);
		free(wstatus);
		exit(EXIT_FAILURE);
	} else {
		openedFile = fopen(filePath, "rb");
	}
	if ( checkForInt(argv[3]) != true ) {
		printf("\n%s\n","invalid terminationBound input - Please give a number for the termination bound");
		free(currentBuffer);
	        free(PIDs);
	        free(stopedCounter);
		free(wstatus);
		exit(EXIT_FAILURE);
	} else {
		terminationBound = atoi(argv[3]);
	}
	//Sending childs to their execution prog with each symbol in the pattern
	for ( int i = 0; i < strlen(charArraySearchQuery); i++) {
	        symbol = argv[2][i];
		char* argvForSymcount[] = {"./sym_count", filePath, &symbol, NULL};
		if ((pid = fork()) == 0) {
		      	if (execvp(argvForSymcount[0], argvForSymcount) < 0) {
				printf("ERROR with executing child process with pid: %d\n", getpid());
				free(currentBuffer);
				free(PIDs);
			        free(stopedCounter);
				free(wstatus);
				exit(EXIT_FAILURE);
			}
			break;
		}
		PIDsSize++;
	}
	//sleeping for 1 second
	sleep(1);
	//Executing a shell command to insert all childs to an array
	insertChildsToAnArray(PIDs, rootProgPID);
	//Going through all the processes in the list
	while (PIDsSize > 0) {
		//Getting their status
		wPIDstatus = waitpid(PIDs[PIDsPointer], wstatus, WUNTRACED | WCONTINUED);
			if (wPIDstatus == -1) {
				printf("ERROR in waitpid()\n");
				free(currentBuffer);
			        free(PIDs);
			        free(stopedCounter);
				exit(EXIT_FAILURE);
			}
			//In case the process is finished (exited eith success) getting it out of the list
      			if (WIFEXITED(*wstatus)) {
				PIDs[PIDsPointer] = PIDs[PIDsSize-1];
				PIDs[PIDsSize-1] = 0;
				stopedCounter[PIDsPointer] = stopedCounter[PIDsSize-1];
				stopedCounter[PIDsSize-1] = 0;
				PIDsSize--;
				PIDsPointer = 0;
			//In case the process is stopped, continue it and increasing it's stopped counter
			} else if (WIFSTOPPED(*wstatus)) {
				stopedCounter[PIDsPointer]++;
				//If the stopped counter got to the termination bound kill it and get it out of the list
				if (stopedCounter[PIDsPointer] == terminationBound) {
					kill(PIDs[PIDsPointer],SIGTERM);
					kill(PIDs[PIDsPointer],SIGCONT);
					PIDs[PIDsPointer] = PIDs[PIDsSize-1];
					stopedCounter[PIDsPointer] = stopedCounter[PIDsSize-1];
					PIDs[PIDsSize-1] = 0;
					stopedCounter[PIDsSize-1] = 0;
					PIDsSize--;
					PIDsPointer = 0;
				}
				//Else: continue the process and move on to the next process in the list
				else {
					kill(PIDs[PIDsPointer],SIGCONT);
					PIDsPointer++;
					if (PIDsPointer == PIDsSize) {
						PIDsPointer = 0;
		      			}
				}
			}
	}
	//Free all alocated arguments from memory and exit the program successfuly
	free(currentBuffer);
	free(PIDs);
	free(stopedCounter);
	free(wstatus);
        exit(EXIT_SUCCESS);
}
//Function that checks if a string is an integer
bool checkForInt(char *str2Check) {
	int len = strlen(str2Check);
	for ( int i=0 ; i<len ; i++) {
		if (str2Check[i] < 48 || str2Check[i] > 57)
			return false;
	}
	return true;
}
// Function that execute a shell command to insert all childs to an array
void insertChildsToAnArray(int* array, pid_t ppid) {
	//I got help from stack overflow
	char *buffer = NULL;
	char command[50] = {0};
	int commandSize = 50;
	FILE *resultCommandFile;
	int pointer = 0;
	sprintf(command, "ps -fade | awk '$3==%u {print $2}'", ppid);
	resultCommandFile = (FILE*)popen(command,"r");
	while(getline(&buffer,&commandSize,resultCommandFile) >= 0) {
		array[pointer] = atoi(buffer);
		pointer++;
	}
	free(buffer);
	fclose(resultCommandFile);
	return;
}
