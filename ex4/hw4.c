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
char chunk[1024] = {0};
pthread_t *thread_ids;
bool *stillRunning;
bool *finishedXoring;
int numOfThreads;
int fdOutputFile;
pthread_mutex_t writeToBuffer;
pthread_cond_t *conds;
pthread_t *threads;

//decleration of help functions
int findMyIndex(pthread_t tid);
int findNextStep();
void *xor(void *t);
int writeToOutputFile();

//getting the thread index
int findMyIndex(pthread_t tid) {
	int i;
	int indexOfThread;
	printf("Starting findMyIndex func\n");
	for (i = 0; i < numOfThreads || thread_ids[i] != '\0' ; i++) {
		if (thread_ids[i] == tid) {
			indexOfThread = i;
			printf("Found that the index of the thread is %d\n",indexOfThread);
		}
	}
	if (indexOfThread == 0) {
		thread_ids[i] = tid;
		indexOfThread = i;
		printf("Index not found in list so added in index %d\n",indexOfThread);
	}
	return indexOfThread;
}

int writeToOutputFile() {
	int rv;
	int i;

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
	return 0;
}

// finding next step to thread - wake up another thread or write to buffer
// if find index return the index
// if all thread finished writing to chunk return -1
// if all threads reached eof return -2
int findNextStep() {
	int i;
	bool notFinished = false;

	for (i = 0; i < numOfThreads; i++) {
		if (finishedXoring[i] == '\0' || (stillRunning[i] == true && finishedXoring[i] == false)) {
			printf("Find next step for unfinished thread in index %d\n",i);
			return i;
		}
		else if (stillRunning[i] == true) {
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
}
//----------------------------------------------------------------------------
void *xor(void *t) {
  int i;
  char *file = (char*)t;
  int fd;
  char buffer[1024] = {0};
  bool fileEnded = false;
  int numOfBytesRead;
  int indexOfThread;
  int nextThreadIndex;
  int rv;
  pthread_t myTID = pthread_self();

  //open the relevant file
  fd = open(file,O_RDONLY);
  if (fd < 0) {
	  printf("one of the threads could not open file %s\n",file);
	  pthread_exit(NULL);
  }

  while (!fileEnded) {
	  numOfBytesRead = read(fd, &buffer, 1024);
	  if (numOfBytesRead < 0) {
		  printf("Could not read from file - %s\n", file);
		  pthread_exit(NULL);
	  } if (numOfBytesRead < 1024) {
		  printf("Reached the end of the file - %s\n", file);
		  fileEnded = true;
	  }
      	  rv = pthread_mutex_lock(&writeToBuffer);
	  if (rv != 0) {
		  printf("ERROR in lock()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
	  rv = pthread_cond_wait(&conds[indexOfThread], &writeToBuffer);
	  if (rv != 0) {
		  printf("ERROR in cond_wait()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
	  indexOfThread = findMyIndex(myTID);
	  for (i = 0; i<1024; i++) {
		  chunk[i] = chunk[i]^buffer[i];
	  }
	  printf("Finished Xoring thread in index %d\n",indexOfThread);
	  stillRunning[indexOfThread] = fileEnded;
	  finishedXoring[indexOfThread] = true;
	  nextThreadIndex = findNextStep();
	  if (nextThreadIndex == -1) {
		rv = writeToOutputFile();
		if (rv != 0) {
			printf("Written failed\n");
			pthread_exit(NULL);
		}
		nextThreadIndex = findNextStep();
	  } else if (nextThreadIndex == -2) {
		  pthread_exit(NULL);
	  }
   	  rv = pthread_cond_signal(&conds[nextThreadIndex]);
	  if (rv != 0) {                            
		  printf("ERROR in cond_signal()\n%s\n",strerror(rv));
		  pthread_exit(NULL);                            
	  }
	  rv = pthread_mutex_unlock(&writeToBuffer);
	  if (rv != 0) {
		  printf("ERROR in Unlock()\n%s\n",strerror(rv));
		  pthread_exit(NULL);
	  }
  }
  printf("Finished with file - %s\n",file);
  pthread_exit(NULL);
}

//----------------------------------------------------------------------------
int main (int argc, char *argv[]) {
  int i;
  int rv;
  pthread_attr_t attr;


  //cheking the input of the user
  if (argc < 3) {
	  printf("\nWrong use in this function! please insert\n./hw4.c <outputFilePath> <inputFilesPath...>\n");
	  exit(EXIT_FAILURE);
  }
  if (access(argv[1],W_OK) != 0) {
	  printf("\nFile \"%s\" does not exist or you do not have the read permissions\n", argv[1]);
	  exit(EXIT_FAILURE);
  } 
  for (i = 2; i<argc; i++) {
	  if (access(argv[i],R_OK) != 0) {
		  printf("\nFile \"%s\" does not exist or you do not have the read permissions\n", argv[i]);
		  exit(EXIT_FAILURE);
	  }
  }
  fdOutputFile = open(argv[1], O_TRUNC);
  if (fdOutputFile < 0) {
	  printf("could not open output file %s\n",argv[1]);
	  exit(EXIT_FAILURE);
  }
  numOfThreads = argc-2;
  thread_ids = (pthread_t*)calloc(numOfThreads+1, sizeof(pthread_t));
  threads = (pthread_t*)calloc(numOfThreads+1, sizeof(pthread_t));
  conds = (pthread_cond_t*)calloc(numOfThreads+1, sizeof(pthread_cond_t));
  stillRunning = (bool*)calloc(numOfThreads+1, sizeof(bool));
  finishedXoring = (bool*)calloc(numOfThreads+1, sizeof(bool));
  if (thread_ids == NULL || threads == NULL || conds == NULL || stillRunning == NULL || finishedXoring == NULL) {
	  printf("cannot calloc one of the internal lists of the prog\n");
  }

  //Initialize mutex and condition variable objects
  pthread_mutex_init(&writeToBuffer, NULL);
  for (i = 0; i<numOfThreads; i++) {
	  rv = pthread_cond_init(&conds[i], NULL);
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
  
  //Wait for all threads to complete
  for (i=0; i<numOfThreads; i++) {
	  rv = pthread_join(threads[i], NULL);
	  if (rv != 0) {                            
		  printf("ERROR in attr_init()\n%s\n",strerror(rv));
		  exit(EXIT_FAILURE);
	  }
  }

  printf ("Main(): Waited on %d  threads. Done.\n", numOfThreads);

  //Clean up and exit
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&writeToBuffer);
  pthread_cond_destroy(conds);
  free(thread_ids);
  free(threads);
  free(conds);
  free(stillRunning);
  free(finishedXoring);
  pthread_exit(NULL);
}
//============================== END OF FILE =================================
