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
int openedFile;
char *sharedFileArray;
size_t sizeOfFile;
int sharedFile;

//Decleration for Handler
void handler(int sig);
void exitWithError(const char *msg, int openedFile, int sharedFile);

int main(int argc, char* argv[]) {
	//Setting a struct to handle the SIGSTOP and SIGTERM signals
	struct sigaction mySig;
	memset(&mySig, 0, sizeof(mySig));
	mySig.sa_handler = handler;
	mySig.sa_sigaction;
	sigaction(SIGPIPE,&mySig, NULL);

	//DEFINITIONS
        const char *filePath = argv[1];
        charSearchQuery = argv[2];
	off_t offset = 0;
	char nameOfFifo[12] = "/tmp/abcdea";
	char finalString[60] = {0};
	//Checks on user input
	if (argc != 3) {
		exitWithError("\nwrong use in function! please insert\n./a.out <filePath> <charToLookFor>\n", -1, -1);
	}
	if (access(filePath,R_OK) != 0) {
		
		exitWithError("\nFile does not exist or you do not have the read permissions\n", -1, -1);
	}
	else {
		// Opening the file and loading it to mmap
		openedFile = open(filePath, O_RDWR);
		if (openedFile < 0) {
			exitWithError("\nFailed opening the file\n", -1, -1);
		}
		lseek(openedFile, (size_t)0, SEEK_CUR);
		sizeOfFile = lseek(openedFile, (size_t)0, SEEK_END);
		lseek(openedFile, (size_t)0, SEEK_SET);
		sharedFileArray = (char *)mmap(NULL, sizeOfFile, PROT_READ, MAP_SHARED, openedFile, 0);
	}
	if ( strlen(charSearchQuery) != 1) {
		exitWithError("\ninvalid char input - the only usage is to search for one character\n", openedFile, -1);
	}
	//Reading the data from mmap and counting the symbol
	nameOfFifo[10] = charSearchQuery[0];
	sharedFile = open(nameOfFifo, O_WRONLY);
	if (sharedFile == -1) {
		exitWithError("Failed to open fifo\n", openedFile,sharedFile);
	}
	while ( offset < sizeOfFile) {
		if (charSearchQuery[0] == sharedFileArray[offset]) {
			occurance++;
		}
		offset++;
	}
	close(openedFile);
	snprintf(finalString, sizeof(finalString), "Process %d finished. Symbol %c. Instances %d.\n", getpid(), charSearchQuery[0], occurance);
	if (write(sharedFile, finalString, sizeof(finalString)) == -1){
		exitWithError("Faled to write to fifo\n", openedFile, sharedFile);
	}
	if (munmap(sharedFileArray, sizeOfFile) == -1) {
		exitWithError("Faled munmap!\n", openedFile, sharedFile);
	}
	if (openedFile >= 0) {
	 	close(openedFile);
	}
	if (sharedFile >= 0) {
		close(sharedFile);
	}
	exit(EXIT_SUCCESS);
}

//Handler for the SIGTERM and SIGCONT signals
void handler(int sig){
       if (sig == SIGPIPE) {
	       printf("SIGPIPE for process %d. Symbol %c. Counter %d. Leaving.", getpid(), charSearchQuery[0], occurance);
	       if (openedFile >= 0) {
      		       close(openedFile);
		       munmap(sharedFileArray, sizeOfFile);
	       }
	       if (sharedFile >=0) {
		       close(sharedFile);
	       }
	       exit(EXIT_SUCCESS);
       } 
}

void exitWithError(const char *msg, int openedFile, int sharedFile) {
	perror(msg);
	if (openedFile >= 0) {
		close(openedFile);
		munmap(sharedFileArray, sizeOfFile);
	}
	if (sharedFile >=0) {
		close(sharedFile);
	}
	exit(EXIT_FAILURE);
}

