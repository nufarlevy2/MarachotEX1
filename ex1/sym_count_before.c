#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

volatile sig_atomic_t stop = 0;

void signalHandler(int signal) {
	switch (signal) {
		int pid;
		case SIGTERM:
			stop = 1;
			break;
		case SIGCONT:
			pid = getpid();
			printf("Process %d continues\n",pid);
			break;
	}
}

int main(int argc, char* argv[]) {
        
	// from git project on web - how to handle signals in unix
	struct sigaction sa;
	sa.sa_handler  = &signalHandler;
	// back to my code
	int rootProgPID = getpid();
	FILE *openedFile;
	char *pointer;
	int occurance = 0;
	if (argc != 3) {
		printf("\n%s\n%s\n","wrong use in function! please insert",
				"./a.out <filePath> <charToLookFor>");
		exit(-1);
	}
	char *currentBuffer = calloc(64, sizeof(char));
	const char *filePath = argv[1];
	const char *charSearchQuery = argv[2];
	int pid;

	if (access(filePath,R_OK) != 0) {
		printf("\n%s\n","File does not exist or you do not have the read permissions");
		exit(-1);
	}
	else {
		openedFile = fopen(filePath, "rb");
	}
	if ( strlen(charSearchQuery) != 1) {
		printf("\n%s\n","invalid char input - the only usage is to search for one character");
		exit(-1);
	}
	if (stop == 1) {
		pid = getpid();
		printf("Process %d finishes. Symbol %s. Instance %d.\n",pid, charSearchQuery, occurance);
		return  0;
	}
	while (true) {
		if (stop == 1) {
			pid = getpid();
	                printf("Process %d finishes. Symbol %s. Instance %d.\n",pid, charSearchQuery, occurance);
	                return  0;
		}
		while (!feof(openedFile) && stop != 1) {
			fread(currentBuffer, sizeof(char), 64, openedFile);
			pointer = strchr(currentBuffer, *charSearchQuery);
			while (pointer != NULL) {
				pointer = strchr(pointer+1, *charSearchQuery);
				occurance++;
				pid = fork();
				if (pid > 0) {
					printf("Process %d, symbol %s, going to sleep\n", getpid(),charSearchQuery);
					kill(getpid(),SIGSTOP);
				}
				if (stop == 1) {
					pid = getpid();
					printf("Process %d finishes. Symbol %s. Instance %d.\n",pid, charSearchQuery, occurance);
			                return  0;
			        }
			}
		}
		printf("num of occurances = %d\n",occurance);
		return(0);
	}
}
