#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

int occurance = 0;
const char *charSearchQuery;
const char *sigTermMsg = "\n\n\nfinish\n";

void handler(int sig) {
       if (sig == SIGTERM) {	
	       printf("Process %d finishes. Symbol %c. Instances %d.\n", getpid(), charSearchQuery[0], occurance);
	       exit(EXIT_SUCCESS); 
       }
       else if (sig == SIGCONT) {
	       printf("Process %d continues\n",getpid());
       }
}


int main(int argc, char* argv[]) {
	struct sigaction mySig;
	memset(&mySig, 0, sizeof(mySig));
	mySig.sa_handler = handler;
	mySig.sa_sigaction;
//	mySig.sa_flags = SA_SIGINFO;
	sigaction(SIGTERM,&mySig, NULL);
//	signal(SIGTERM,handleSignalTerm);
	sigaction(SIGCONT,&mySig, NULL);
	// back to my code
	int rootProgPID = getpid();
	FILE *openedFile;
	char *pointer;
	if (argc != 3) {
		printf("\n%s\n%s\n","wrong use in function! please insert",
				"./a.out <filePath> <charToLookFor>");
		exit(-1);
	}
	char *currentBuffer = calloc(64, sizeof(char));
	const char *filePath = argv[1];
	charSearchQuery = argv[2];
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
	while ((!feof(openedFile))) {
		fread(currentBuffer, sizeof(char), 64, openedFile);
		pointer = strchr(currentBuffer, *charSearchQuery);
		while (pointer != NULL) {
			pointer = strchr(pointer+1, *charSearchQuery);
			occurance++;
			printf("Process %d, symbol %s, going to sleep\n", getpid(),charSearchQuery);
			kill(getpid(),SIGSTOP);
		}
	}
	kill(getpid(),SIGTERM);	
}
