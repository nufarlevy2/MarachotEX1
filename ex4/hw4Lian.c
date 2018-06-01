/*
 * hw4.c
 *
 *  Created on: May 19, 2018
 *      Author: lian
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>     /* exit */
#include <errno.h>
#include <sys/stat.h>

#define FILENAME_MAX_SIZE 1024
#define FAIL -1
#define BUFFER_SIZE 1048576 /* 2**20 */
#define FALSE 0
#define TRUE 1

char buffer[BUFFER_SIZE];
int *inFds, outFd, *excluded, bufferUsed = 0, inCount, shouldRun = 0;
pthread_t *threads;
pthread_cond_t *conds;
pthread_mutex_t lock;

/*
 * Get next thread that's supposed to run
 * If all are excluded return -1
 */
int nextThread(int threadNum) {
	int threadCount = 0, next = (threadNum + 1) % inCount;
	while (excluded[next] == TRUE && threadCount < inCount) {
		next = (next + 1) % inCount;
		threadCount++;
	}
	if (threadCount == inCount) {
		printf("nextThread: returning -1\n");
		return -1;
	}
	return next;
}

/*
 * Close input files & free resources
 */
void freeAll(int inCount) {
	close(outFd);
	for (int i = 0; i < inCount; i++) {
		close(inFds[i]);
		pthread_cond_destroy(&conds[i]);
	}
	pthread_mutex_destroy(&lock);
	free(inFds);
	free(threads);
	free(excluded);
	free(conds);
}

void *threadReader(void *threadNum) {
	int i, nextT = -1, written, rc;
	int fd = inFds[(int) threadNum];
	char bufFile[BUFFER_SIZE];
	int bytesRead = BUFFER_SIZE;

	printf("Thread %d started\n", (int) threadNum);

	while (bytesRead == BUFFER_SIZE) {
		if ((bytesRead = read(fd, &bufFile, BUFFER_SIZE)) == -1) {
			printf("ERROR: Failed to read from fd %d\n", fd);
			pthread_exit(NULL );
		}
		rc = pthread_mutex_lock(&lock);
		if (rc != 0) {
			printf("ERROR in pthread_mutex_lock(): %s\n", strerror(rc));
			exit(FAIL);
		}
		if ((int) threadNum != nextT) {
			// While it's not my turn - wake up next thread and go to sleep
			while (shouldRun != (int) threadNum) {
				nextT = nextThread((int) threadNum);
				pthread_cond_signal(&conds[nextT]);
				pthread_cond_wait(&conds[(int) threadNum], &lock);
			}
		}

		for (i = 0; i < bytesRead; i++) {
			buffer[i] = (char) (buffer[i] ^ bufFile[i]);
		}
		if (bufferUsed < bytesRead) {
			bufferUsed = bytesRead;
		}
		nextT = nextThread((int) threadNum);
		if (nextT <= (int) threadNum) {
			if ((written = write(outFd, buffer, bufferUsed)) < 0) {
				printf("ERROR: writing failed - errno %d\n", errno);
				pthread_exit(NULL );
			}
			bufferUsed = 0;
			memset(buffer, 0, BUFFER_SIZE);
		}

		shouldRun = nextT;

		rc = pthread_mutex_unlock(&lock);
		if (rc != 0) {
			printf("ERROR in pthread_mutex_unlock(): %s\n", strerror(rc));
			exit(FAIL);
		}

		if (bytesRead < BUFFER_SIZE) {
			excluded[(int) threadNum] = TRUE;
			pthread_cond_signal(&conds[nextT]);
			pthread_exit(NULL );
		} else {
			pthread_cond_signal(&conds[nextT]);
		}

	}
	// Shouldn't get here
	pthread_exit(NULL );
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("You should insert at least output file and one input file\n");
		exit(FAIL);
	}
	inCount = argc - 2;

	char outputPath[FILENAME_MAX_SIZE];
	int rc, i;
	void *status;
	strcpy(outputPath, argv[1]);

	memset(buffer, 0, BUFFER_SIZE);

	printf("Hello, creating %s from %d input files\n", outputPath, inCount);

	outFd = open(outputPath, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (outFd == -1) {
		printf("ERROR: Problem opening output file\n");
		close(outFd);
		exit(FAIL);
	}

	// memory allocation
	inFds = (int*) malloc(sizeof(int) * inCount);
	threads = (pthread_t*) malloc(sizeof(pthread_t) * inCount);
	excluded = (int*) calloc(inCount, sizeof(int) * inCount);
	conds = (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * inCount);

	// initialize lock
	pthread_mutex_init(&lock, NULL );

	// Open input files & initiate condition variables
	for (i = 0; i < inCount; i++) {
		inFds[i] = open(argv[i + 2], O_RDONLY);
		if (inFds[i] == -1) {
			printf("ERROR: Problem opening input file %d\n", (i));
			freeAll(i + 1); // close files created until current file
			exit(FAIL);
		}
		pthread_cond_init(&conds[i], NULL );

	}

	// Create reader threads
	i = 0;
	for (i = 0; i < inCount; i++) {
		rc = pthread_create(&threads[i], NULL, threadReader, (void*) i);
		if (rc) {
			printf("ERROR in pthread_create(): %s\n", strerror(rc));
			freeAll(i + 1); // close files created until current file
			exit(FAIL);
		}
	}
	sleep(1);
	pthread_cond_signal(&conds[0]);

	for (i = 0; i < inCount; i++) {
		rc = pthread_join(threads[i], &status);
		if (rc) {
			printf("ERROR in pthread_join(): %s\n", strerror(rc));
			freeAll(inCount);
			exit(FAIL);
		}
		printf("Main: completed join with thread %d having a status of %ld\n",
				i, (long) status);
	}

	// find file size
	struct stat st;
	stat(argv[1], &st);
	int outputSize = st.st_size;
	printf("Created %s with size %d bytes\n", outputPath, outputSize);

	// free all & exit
	freeAll(inCount);
	pthread_exit(NULL );
}
