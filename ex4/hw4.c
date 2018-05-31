/*
 *  Taken from:
 *  https://computing.llnl.gov/tutorials/pthreads/#ConditionVariables
 *
 * */
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
char chunk[1024];
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

//decleration of help functions
int findMyIndex(char *file);
int findNextStep();
void *xor(void *t);
int writeToOutputFile();

//getting the thread index
int findMyIndex(char *file) {
	int i;
	int indexOfThread = 0;
	printf("Starting findMyIndex func\n");
	for (i = 0; i < numOfThreads; i++) {
		printf("In for loop of find my index inputFilesPointer[i]: %s, file is: %s\n", inputFilesPointer[i],file);
		if (strstr(inputFilesPointer[i],file)) {
			indexOfThread = i;
			printf("Found that the index of the thread is %d of file %s file in list is %s\n",indexOfThread, file, inputFilesPointer[i]);
			return indexOfThread;
		}
	}
	return indexOfThread;
}

int writeToOutputFile() {
	int rv;
	int i;
	printf("In writeToOutputFile()\n");
	rv = write(fdOutputFile, chunk, 1024);
	if (rv < 0) {
		printf("Could not write chunk to data file\n");
		return -1;
	}
	for (i = 0; i < numOfThreads; i++) {
		if (stillRunning[i] == true) {
			finishedXoring[i] = false;
		}
	}
	printf("Written all chunk to output file\n");
	for (i = 0; i < numOfThreads; i++) {
		finishedXoring[i] = false;
  		printf("After writen to file thread in place %d has putten false in finished xoring\n",i);
	}
	for (i = 0; i<1024; i++) {
		chunk[i] = '\0';
	}
	return 0;
}

// finding next step to thread - wake up another thread or write to buffer
// if find index return the index
// if all thread finished writing to chunk return -1
// if all threads reached eof return -2
int findNextStep() {
	int i;
	bool notFinished = false;
	printf("In findNextIndex()\n");
	for (i = 0; i < numOfThreads; i++) {
		if (finishedXoring[i] == '\0' || (stillRunning[i] == true && finishedXoring[i] == false)) {
			printf("Find next step for unfinished thread in index %d for file %s\n",i, inputFilesPointer[i]);
			notFinished = true;
			return i;
		}
		else if (stillRunning[i] == true) {
			printf("Still running!!!!!!\n");
			notFinished = true;
		}
	}
	if (notFinished) {
		printf("Not all threads are finished and all are done xoring\n");
		return -1;
	} else {
		printf("All threads reached EOF\n");
		return -2;
	}
	printf("Finished with findNextIndex() function\n");
}
//----------------------------------------------------------------------------
void *xor(void *t) {
  int i;
  char *file = (char*)t;
  int fd;
  char buffer[1024] = {'\0'};
  bool fileEnded = false;
  int numOfBytesRead;
  int nextThreadIndex;
  int rv;
  int indexOfThread;
//  pthread_t myTID = pthread_self();

  printf("In xor funxtion\n");
  //open the relevant file
  fd = open(file,O_RDONLY);
  if (fd < 0) {
	  printf("one of the threads could not open file %s\n",file);
	  pthread_exit(NULL);
  }
  printf("After open of input file\n");
  while (!fileEnded) {
	  printf("In while - file not ended\n");
	  numOfBytesRead = read(fd, &buffer, 1024);
	  printf("Num of bytes = %d\n",numOfBytesRead);
	  if (numOfBytesRead < 0) {
		  printf("Could not read from file - %s\n", file);
		  pthread_exit(NULL);
	  } if (numOfBytesRead < 1024) {
		  printf("Reached the end of the file - %s\n", file);
		  fileEnded = true;
	  }
	  printf("Before find my index\n");
	  indexOfThread = findMyIndex(file);
	  printf("After find my index\n");
      	  rv = pthread_mutex_lock(&writeToBuffer);
	  printf("After starting lock, Thread : %d, is in lock block\n", indexOfThread);
	  if (rv != 0) {
		  printf("ERROR in lock()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
	  printf("Need to do cond wait is %d\n",needToDoCondWait);
	  while (needToDoCondWait) {
		  rv = pthread_cond_wait(&wakeUp, &writeToBuffer);
	  }
	  printf("After wait for condition\n");
	  if (rv != 0) {
		  printf("ERROR in cond_wait()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
	  printf("Before xoring to buffer\n");
	  for (i = 0; i<1024; i++) {
		  chunk[i] = chunk[i]^buffer[i];
	  }
	  printf("Finished Xoring thread in index %d\n",indexOfThread);
	  stillRunning[indexOfThread] = !fileEnded;
	  finishedXoring[indexOfThread] = true;
	  printf("Before finding next thread\n");
	  nextThreadIndex = findNextStep();
	  printf("After finding next thread\n");
	  if (nextThreadIndex == -1) {
		printf("Before writing to file\n");
		rv = writeToOutputFile();
		printf("After writing to file\n");
		if (rv != 0) {
			printf("Written failed\n");
			pthread_exit(NULL);
		}
		printf("Before finding next after wriiting\n");
		nextThreadIndex = findNextStep();
		printf("After next thread finding\n");
	  } else if (nextThreadIndex == -2) {
		  printf("Got -2 in findNextStep function\n");
		  pthread_exit(NULL);
	  }
	  printf("Before wake up next thread in index %d\n",nextThreadIndex);
   	  rv = pthread_cond_broadcast(&wakeUp);
	  printf("After waking up the next thread in index %d\n",nextThreadIndex);
	  if (rv != 0) {                            
		  printf("ERROR in cond_signal()\n%s\n",strerror(rv));
		  pthread_exit(NULL);                            
	  }
	  printf("Before unlocking the thread\n");
	  rv = pthread_mutex_unlock(&writeToBuffer);
	  printf("After unlocking the thread\n");
	  if (rv != 0) {
		  printf("ERROR in Unlock()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
	  for (i = 0; i<1024; i++) {
		  chunk[i] = '\0';
	  }
	  sleep(0.1);
  }
  printf("Finished with file - %s\n",file);
  close(fd);
  pthread_exit(NULL);
}

//----------------------------------------------------------------------------
int main (int argc, char *argv[]) {
  int i;
  int rv;
  pthread_attr_t attr;

  //cheking the input of the user
  for (i = 0; i<1024; i++) {
 	  chunk[i] = '\0';
  }
  if (argc < 3) {
	  printf("\nWrong use in this function! please insert\n./hw4.c <outputFilePath> <inputFilesPath...>\n");
	  exit(EXIT_FAILURE);
  }
//  if (access(argv[1],W_OK) != 0) {
//	  printf("\nFile \"%s\" does not exist or you do not have the read permissions\n", argv[1]);
//	  exit(EXIT_FAILURE);
 // } 
  for (i = 2; i<argc; i++) {
	  if (access(argv[i],R_OK) != 0) {
		  printf("\nFile \"%s\" does not exist or you do not have the read permissions\n", argv[i]);
		  exit(EXIT_FAILURE);
	  }
  }
  fdOutputFile = open(argv[1], O_TRUNC | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IXUSR);
  if (fdOutputFile < 0) {
	  printf("could not open output file %s\n",argv[1]);
	  exit(EXIT_FAILURE);
  }
  numOfThreads = argc-2;
  inputFilesPointer = malloc((argc-2)*sizeof(*inputFilesPointer));
  if (inputFilesPointer == NULL) {
       	  printf("Malloc not succedded for inputFilesPointer\n");
  }
  for (i = 0; i<argc-2; i++) {
	  inputFilesPointer[i] = argv[i+2];
	  printf("\n\ninputFilesPointer[i]: i = %d, value = %s\n\n",i,inputFilesPointer[i]);
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
  printf("Num of threads are %d\n",numOfThreads);
  for (i = 0; i<numOfThreads; i++) {
	  rv = pthread_cond_init(&wakeUp, NULL);
	  if (rv != 0) {
		  printf("ERROR in cond_init()\n%s\n",strerror(rv));
		  exit(EXIT_FAILURE);
	  }
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
//  if (numOfThreads > 1) {
//	  printf("Sending signal from main to the first thread\n");
//	  rv = pthread_cond_signal(&conds[0]);
//	  if (rv != 0) {
//		  printf("ERROR in cond_signal()\n%s\n",strerror(rv));
//		  pthread_exit(NULL);
//	  }
//  }
  //Wait for all threads to complete
  for (i=0; i<numOfThreads; i++) {
	  rv = pthread_join(threads[i], NULL);
	  if (rv != 0) {                            
		  printf("ERROR in pthread_join()\n%s\n",strerror(rv));
		  exit(EXIT_FAILURE);
	  }
  }

  printf ("Main(): Waited on %d  threads. Done.\n", numOfThreads);

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
