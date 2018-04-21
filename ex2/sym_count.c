#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>


//GLOBAL definitions
int occurance = 0;
const char *charSearchQuery;
char *currentBuffer;
int openedFile;

//Decleration for Handler
void handler(int sig);
void exitWithError(const char *msg, int openedFile, const char *sharedFile);

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
		exitWithError("\nwrong use in function! please insert\n./a.out <filePath> <charToLookFor>\n", -1, "notRelevant");
	}
	if (access(filePath,R_OK) != 0) {
		
		exitWithError("\nFile does not exist or you do not have the read permissions\n", -1, "notRelevant");
	}
	else {
		openedFile = open(filePath, O_RDWR);
		if (openedFile < 0) {
			exitWithError("\nFailed opening the file\n", -1, "notRelevant");
		}
		lseek(openedFile, (size_t)0, SEEK_CUR);
		size_t  sizeOfFile = lseek(openedFile, (size_t)0, SEEK_END);
		lseek(openedFile, (size_t)0, SEEK_SET);
		char *sharedFileArray = (char *)mmap(NULL, sizeOfFile, PROT_READ, MAP_SHARED, openedFile, 0);
	}
	if ( strlen(charSearchQuery) != 1) {
		exitWithError("\ninvalid char input - the only usage is to search for one character\n", openedFile, filePath);
	}
	exitWithError("\n!!!!!!!!!!!!!!!!!!!opned file succesfully!!!!!!!!!!!!!!!!!!\n", openedFile,filePath);
	//Reading the data file and searching for the symbol
//	while ((!feof(openedFile))) {
//		fread(currentBuffer, sizeof(char), 64, openedFile);
//		pointer = strchr(currentBuffer, *charSearchQuery);
//		while (pointer != NULL) {
//			pointer = strchr(pointer+1, *charSearchQuery);
//			occurance++;
//		}
//	}
	//Sending SIGTERM to the finished process
//	kill(getpid(),SIGTERM);	
}

//Handler for the SIGTERM and SIGCONT signals
void handler(int sig){
       if (sig == SIGTERM) {
	       printf("Process %d finishes. Symbol %c. Instances %d.\n", getpid(), charSearchQuery[0], occurance);
	       exit(EXIT_SUCCESS);
       }	       
}

void exitWithError(const char *msg, int filed, const char *sharedFile) {
	perror(msg);
	if (filed >= 0) {
		close(filed);
		unlink(sharedFile);
	}
	exit(EXIT_FAILURE);
}

