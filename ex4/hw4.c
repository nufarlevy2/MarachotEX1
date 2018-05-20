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

//global variables
char chunk[1024] = {0};
//pid_t *thread_ids;
int numOfThreads;
pthread_mutex_t writeToBuffer;
pthread_cond_t *conds;
//pthread_t *threads;

//----------------------------------------------------------------------------
void *xor(void *t)
{
  int i;
  char *file = (char*)t;
  int fd;
  char buffer[1024] = {0};
  bool fileEnded = false;
  int numOfBytesRead;
//  int indexOfThread;
//  pid_t myTID = gettid();

  //getting the thread index
//  for (i = 0; i < numOfThreads || thread_ids[i] != '\0' ; i++) {
//	  if (thread_ids[i] == myTID) {
//		  indexOfThread = i;
//	  }
//  }
//  if (indexOfThread == 0) {
//	  thread_ids[i] = myTID;
//  }

  //open the relevant file
  fd = open(file,O_TRUNC);
  if (fd < 0) {
	  printf("one of the threads could not open file %s\n",file);
	  pthread_exit(NULL);
  }

  while (!fileEnded) {
	  numOfBytesRead = read(fd, &buffer, 1024);
	  if (numOfBytesRead < 1024) {
		  fileEnded = true;
	  }
      	  pthread_mutex_lock(&writeToBuffer);
	  pthread_cond_wait(&conds[indexOfThread], &writeToBuffer);
	  for (i = 0; i<1024; i++) {
		  chunk[i] = chunk[i]^buffer[i];
	  }
   	  pthread_cond_signal(&conds[indexOfThread], &writeToBuffer);
    }

    printf("inc_count(): thread %ld, count = %d, unlocking mutex\n",
           my_id, count);
    pthread_mutex_unlock(&count_mutex);

    //Do some "work" so threads can alternate on mutex lock
    sleep(1);
    if (pthread_cond_wait(&count_threshold_cv, &count_mutex) == 0 && pthread_cond_wait(&count_threshold_cv, &count_mutex)  == 0) {
		    printf("all condition are finished\n");
  }
  pthread_exit(NULL);
}

//----------------------------------------------------------------------------
int main (int argc, char *argv[])
{
  int i;
  pthread_attr_t attr;


  //cheking the input of the user
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
  numOfThreads = argc-2;
//  thread_ids = (pid_t*)calloc(numOfThreads+1, sizeof(pid_t));
//  threads = (pthread_t*)calloc(numOfThreads+1, sizeof(pthread_t));
  conds = (pthread_cond_t*)calloc(numOfThreads+1, sizeof(pthread_cond_t));

  //Initialize mutex and condition variable objects
  pthread_mutex_init(&writeToBuffer, NULL);
  for (i = 0; i<numOfThreads; i++) {
	  pthread_cond_init(&cond[i], NULL);
  }

  //For portability, explicitly create threads in a joinable state
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (i = 0; i<numOfThreads; i++) {
	  pthread_create(threads[i], &attr, xor, (void *)argv[i+2]);
  }
  
  //Wait for all threads to complete
  for (i=0; i<NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  printf ("Main(): Waited on %d  threads. Done.\n", NUM_THREADS);

  //Clean up and exit
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&writeToBuffer);
  pthread_cond_destroy(&awake);
  pthread_cond_destroy(&eof);
  free(thread_ids);
  free(threads);
  pthread_exit(NULL);
}
//============================== END OF FILE =================================
