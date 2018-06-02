#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

//global variables
char chunk[1024*1024];
pthread_t *thread_ids;
bool *stillRunning;
bool *finishedXoring;
int numOfThreads;
int fdOutputFile;
pthread_mutex_t writeToBuffer;
pthread_cond_t wakeUp;
pthread_t *threads;
char** inputFilesPointer;
bool needToDoCondWait = 0;
int sizeOfFile = 0;
int sizeOfChunk = 0;
bool WaitForWriteToBeFinished = true;

//decleration of help functions
int findMyIndex(char *file);
int findNextStep();
void *xor(void *t);
int writeToOutputFile();

//getting the thread index
int findMyIndex(char *file) {
	int i;
	int indexOfThread = 0;
	for (i = 0; i < numOfThreads; i++) {
		if (strstr(inputFilesPointer[i],file)) {
			indexOfThread = i;
			return indexOfThread;
		}
	}
	return indexOfThread;
}
//write chunk to output file
int writeToOutputFile() {
	int rv;
	int i;
	rv = write(fdOutputFile, chunk, sizeOfChunk);
	if (rv < 0) {
		printf("Could not write chunk to data file\n");
		return -1;
	}
	for (i = 0; i < numOfThreads; i++) {
		if (stillRunning[i] == true) {
			finishedXoring[i] = false;
		}
	}
	sizeOfFile += sizeOfChunk;
	sizeOfChunk = 0;
	for (i = 0; i<(1024*1024); i++) {
		chunk[i] = '\0';
	}
	WaitForWriteToBeFinished = false;
	return 0;
}

// finding next step to thread - wake up another thread or write to buffer
// if find index return the index
// else returns -1
int findNextStep() {
	int i;
	for (i = 0; i < numOfThreads; i++) {
		if ((stillRunning[i] == true && finishedXoring[i] == false)) {
			return i;
		}
	}
	return -1;
}
//----------------------------------------------------------------------------
void *xor(void *t) {
  int i;
  char *file = (char*)t;
  int fd;
  char buffer[1024*1024] = {'\0'};
  bool fileEnded = false;
  int numOfBytesRead;
  int nextThreadIndex;
  int rv;
  int indexOfThread;

  //open the relevant file
  fd = open(file,O_RDONLY);
  if (fd < 0) {
	  printf("ERROR one of the threads could not open file %s\n",file);
	  pthread_exit(NULL);
  }
  indexOfThread = findMyIndex(file);
  numOfBytesRead = read(fd, &buffer, (1024*1024));
  if (numOfBytesRead < 0) {
	  printf("Could not read from file - %s\n", file);
	  pthread_exit(NULL);
  } else if (numOfBytesRead == 0) {
	  fileEnded = true;
	  stillRunning[indexOfThread] = !fileEnded;
  }
  while (!fileEnded) {
      	  rv = pthread_mutex_lock(&writeToBuffer);
	  if (rv != 0) {
		  printf("ERROR in lock()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
	  for (i = 0; i<numOfBytesRead; i++) {
		  chunk[i] = chunk[i]^buffer[i];
		  buffer[i] = '\0';
	  }
	  
	  if (sizeOfChunk < numOfBytesRead) {
		  sizeOfChunk = numOfBytesRead;
	  }
	  finishedXoring[indexOfThread] = true;
	  nextThreadIndex = findNextStep();
	  if (nextThreadIndex < 0) {
		rv = writeToOutputFile();
		if (rv != 0) {
			printf("ERROR could not write to file\n");
			pthread_exit(NULL);
		}
		stillRunning[indexOfThread] = !fileEnded;
		rv = pthread_cond_broadcast(&wakeUp);
		if (rv != 0) {
			printf("ERROR in cond_broadcast()\n%s\n",strerror(rv));
			pthread_exit(NULL);
		}
	  } else {
		  pthread_cond_wait(&wakeUp, &writeToBuffer);
	  }
	  rv = pthread_mutex_unlock(&writeToBuffer);
	  if (rv != 0) {
		  printf("ERROR in Unlock()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
	  numOfBytesRead = read(fd, &buffer, (1024*1024));
	  if (numOfBytesRead < 0) {
		  printf("Could not read from file - %s\n", file);
		  pthread_exit(NULL);
	  } else if (numOfBytesRead == 0) {
		  fileEnded = true;
		  stillRunning[indexOfThread] = false;
	  }
  }
  close(fd);
  pthread_exit(NULL);
}

//----------------------------------------------------------------------------
int main (int argc, char *argv[]) {
  int i;
  int rv;
  pthread_attr_t attr;

  //cheking the input of the user
  for (i = 0; i<(1024*1024); i++) {
 	  chunk[i] = '\0';
  }
  if (argc < 3) {
	  printf("\nWrong use in this function! please insert\n./hw4.c <outputFilePath> <inputFilesPath...>\n");
	  exit(EXIT_FAILURE);
  }
  for (i = 2; i<argc; i++) {
	  if (access(argv[i],R_OK) != 0) {
		  printf("\nFile \"%s\" does not exist or you do not have the read permissions\n", argv[i]);
		  exit(EXIT_FAILURE);
	  }
  }
  fdOutputFile = open(argv[1], O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR);
  if (fdOutputFile < 0) {
	  printf("could not open output file %s\n",argv[1]);
	  exit(EXIT_FAILURE);
  }
  numOfThreads = argc-2;
  printf("Hello, creating %s from %d input files\n",argv[1], numOfThreads);
  inputFilesPointer = malloc((argc-2)*sizeof(*inputFilesPointer));
  if (inputFilesPointer == NULL) {
       	  printf("Malloc not succedded for inputFilesPointer\n");
  }
  for (i = 0; i<argc-2; i++) {
	  inputFilesPointer[i] = argv[i+2];
  }
  thread_ids = (pthread_t*)calloc(numOfThreads+1, sizeof(pthread_t));
  threads = (pthread_t*)calloc(numOfThreads+1, sizeof(pthread_t));
  stillRunning = (bool*)calloc(numOfThreads+1, sizeof(bool));
  finishedXoring = (bool*)calloc(numOfThreads+1, sizeof(bool));
  if (thread_ids == NULL || threads == NULL || stillRunning == NULL || finishedXoring == NULL) {
	  printf("cannot calloc one of the internal lists of the prog\n");
  }

  //Initialize mutex and condition variable objects
  pthread_mutex_init(&writeToBuffer, NULL);
  for (i = 0; i<numOfThreads; i++) {
	  rv = pthread_cond_init(&wakeUp, NULL);
	  if (rv != 0) {
		  printf("ERROR in cond_init()\n%s\n",strerror(rv));
		  exit(EXIT_FAILURE);
	  }
	  stillRunning[i] = true;
	  finishedXoring[i] = false;
  }

  //For portability, explicitly create threads in a joinable state
  rv = pthread_attr_init(&attr);
  if (rv != 0) {                  
	  printf("ERROR in attr_init()\n%s\n",strerror(rv));
	  exit(EXIT_FAILURE);            
  }
  rv = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if (rv != 0) {                                            
	  printf("ERROR in attr_init()\n%s\n",strerror(rv));      
	  exit(EXIT_FAILURE);                            
  }
  for (i = 0; i<numOfThreads; i++) {
	  rv = pthread_create(&threads[i], &attr, xor, (void *)argv[i+2]);
	  if (rv != 0) {
		  printf("ERROR in create_thread()\n%s\n",strerror(rv));
		  exit(EXIT_FAILURE);
	  }
  }
  sleep(1);
  //Wait for all threads to complete
  for (i=0; i<numOfThreads; i++) {
	  rv = pthread_join(threads[i], NULL);
	  if (rv != 0) {                            
		  printf("ERROR in pthread_join()\n%s\n",strerror(rv));
		  exit(EXIT_FAILURE);
	  }
  }

  printf ("Created %s with size %d bytes\n", argv[1], sizeOfFile);

  //Clean up and exit
  close(fdOutputFile);
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&writeToBuffer);
  pthread_cond_destroy(&wakeUp);
  free(thread_ids);
  free(threads);
  free(inputFilesPointer);
  free(stillRunning);
  free(finishedXoring);
  pthread_exit(NULL);
}
//============================== END OF FILE =================================
