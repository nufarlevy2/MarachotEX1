#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>


//GLOBAL definitions
int occurance = 0;
const char *charSearchQuery;
char *currentBuffer;
FILE *openedFile;

//Decleration for Handler
void handler(int sig);

int main(int argc, char* argv[]) {
	//Setting a struct to handle the SIGSTOP and SIGTERM signals
	struct sigaction mySig;
	memset(&mySig, 0, sizeof(mySig));
	mySig.sa_handler = handler;
	mySig.sa_sigaction;
	sigaction(SIGTERM,&mySig, NULL);
	sigaction(SIGCONT,&mySig, NULL);

	//DEFINITIONS
	char *pointer;
        const char *filePath = argv[1];
        charSearchQuery = argv[2];
	currentBuffer = calloc(64, sizeof(char));

	//Checks on user input
	if (argc != 3) {
		printf("\n%s\n%s\n","wrong use in function! please insert",
				"./a.out <filePath> <charToLookFor>");
		free(currentBuffer);
		exit(EXIT_FAILURE);
	}
	if (access(filePath,R_OK) != 0) {
		printf("\n%s\n","File does not exist or you do not have the read permissions");
		free(currentBuffer);
		exit(EXIT_FAILURE);
	}
	else {
		openedFile = fopen(filePath, "rb");
	}
	if ( strlen(charSearchQuery) != 1) {
		printf("\n%s\n","invalid char input - the only usage is to search for one character");
		free(currentBuffer);
		fclose(openedFile);
		exit(EXIT_FAILURE);
	}
	//Reading the data file and searching for the symbol
	while ((!feof(openedFile))) {
		fread(currentBuffer, sizeof(char), 64, openedFile);
		pointer = strchr(currentBuffer, *charSearchQuery);
		while (pointer != NULL) {
			pointer = strchr(pointer+1, *charSearchQuery);
			occurance++;
			//Stoping the process after founding the symbol
			printf("Process %d, symbol %s, going to sleep\n", getpid(),charSearchQuery);
			kill(getpid(),SIGSTOP);
		}
	}
	//Free all alocated memory and killing the process
	free(currentBuffer);
	fclose(openedFile);
	kill(getpid(),SIGTERM);	
}

//Handler for the SIGTERM and SIGCONT signals
void handler(int sig){
       if (sig == SIGTERM) {
	       printf("Process %d finishes. Symbol %c. Instances %d.\n", getpid(), charSearchQuery[0], occurance);
	       free(currentBuffer);
               fclose(openedFile);
	       exit(EXIT_SUCCESS);						            }
       else if (sig == SIGCONT) {
	       printf("Process %d continues\n",getpid());
       }
}

