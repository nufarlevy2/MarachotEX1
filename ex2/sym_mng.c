#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>


//GLOBAL definitions
int* PIDs;

//Decleration for functions
bool checkForInt(char *str2Check);
void insertChildsToAnArray(int* array, int ppid);
void exitWithError(const char *msg, int* PIDs);
void handler(int sig);

int main(int argc, char* argv[]) {
	//Setting a struct to handle the SIGPIPE signal
	struct sigaction mySig;
	memset(&mySig,0,sizeof(mySig));
	mySig.sa_handler = handler;
	mySig.sa_sigaction;
	sigaction(SIGPIPE,&mySig,NULL);

	//DEFINITIONS part 1
	system("rm -f /tmp/abcde*");
	pid_t rootProgPID = getpid();
	int  openedFile;
	char symbol[2] = {0};
	int i = 0;
	char nameOfFifo[12] = "/tmp/abcdea";
	char buff[217];
	int readBytes;
	int PIDsSize = 0;
	
	//First checkup
	if (argc != 3) {
		exitWithError("\nWrong use in function! please insert\n./prog <filePath> <chartersToLookFor>\n", NULL);
	}
	//DEFINITIONS part 2
	int pointer = 0;
	char *filePath = argv[1];
	char* charArraySearchQuery = argv[2];
	size_t patternSize = strlen(argv[2]);
	PIDs = (int*)malloc(patternSize*sizeof(int));

//	int PIDsFile = open("PIDs",O_RDWR | O_CREAT, 0600);
//	if (PIDsFile < 0) {
//		exitWithError("Failed to open tmpFileFor PIDs\n");
//	}
//	char *PIDs = (int *)mmap(NULL,strlen(charArraySearchQuery)*sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED, PIDsFile, 0);
//	if (PIDs < 0) {
//		exitWithError("Failed to mmap PIDs\n");
//	}
	int pid;
	//All checks
	if (access(filePath,R_OK) != 0) {
		exitWithError("\nFile does not exist or you do not have the read permissions\n", PIDs);
	}
	//Sending childs to their execution prog with each symbol in the pattern
	for (i = 0; i < strlen(charArraySearchQuery); i++) {
	        symbol[0] = argv[2][i];
		symbol[1] = '\0';
		nameOfFifo[10] = symbol[0]; 
		if (mkfifo(nameOfFifo, 0600) == -1) {
			exitWithError("Failed to mkfifo\n",PIDs);
		}
		char* argvForSymcount[] = {"./sym_count", filePath, symbol, NULL};
		if ((pid = fork()) == 0) {
		      	if (execvp(argvForSymcount[0], argvForSymcount) < 0) {
				exitWithError("ERROR with executing child process\n", PIDs);
			}
			break;
		}
		else if (pid == -1) {
			exitWithError("Failed to fork\n", PIDs);
		}
		PIDsSize++;
	}
	//sleeping for 1 second
	sleep(1);
	//Executing a shell command to insert all childs to an array
//	insertChildsToAnArray(PIDs, rootProgPID);
	//Going through all the processes in the list
	while (patternSize > 0) {
		nameOfFifo[10] = argv[2][pointer];
		int fd = open(nameOfFifo, O_RDONLY);
		//In case the process is finished (exited eith success) getting it out of the list
      		if (readBytes = read(fd, buff, 216) > 0) {
			if(strstr(buff, "finished") > 0) {
				close(fd);
				unlink(nameOfFifo);
				printf("%s",buff);
				charArraySearchQuery[pointer] = charArraySearchQuery[patternSize-1];
				charArraySearchQuery[patternSize-1] = '\0';
				patternSize--;
				pointer = 0;
			}
			else {
				pointer++;
			}
		}
		if (readBytes == -1) {
			exitWithError("failed to readBytes\n",PIDs);
		}
		//In case the process is stopped, continue it and increasing it's stopped counter
		if (pointer == patternSize) {
			pointer = 0;
		}
	}
	//Free all alocated arguments from memory and exit the program successfuly
	if (PIDs != NULL) {
		free(PIDs);
	}
	system("rm -f /tmp/abcde*");
	exit(EXIT_SUCCESS);
}
// Function that execute a shell command to insert all childs to an array
void insertChildsToAnArray(int* array, pid_t ppid) {
	//I got help from stack overflow
	char *buffer = NULL;
	char command[50] = {0};
	FILE *resultCommandFile;
	size_t commandSize = 50;
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

void exitWithError(const char *msg, int* PIDs) {
	perror(msg);
	if (PIDs != NULL) {
	        free(PIDs);
	}
	system("rm -f /tmp/abcde*");
	exit(EXIT_FAILURE);
}

void handler(int sig) {
	if (sig == SIGPIPE) {
		printf("something");
		if (PIDs != NULL) {
			free(PIDs);
		}
		system("rm -f /tmp/abcde*");
		exit(EXIT_SUCCESS);
	}
}



