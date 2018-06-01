#define _XOPEN_SOURCE
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdbool.h>

#define MSG_O "Created %s with size %ld bytes\n"
#define MSG_I "Hello, creating %s from %d input files\n"
#define CHUNK 1024*1024
char BUFFER[CHUNK];
int fd_o = -2;
bool *hasFinished;
bool *hasXored;
int *allSizes;
int inputs_amount = -2;
int *descriptors;
pthread_mutex_t mx;
pthread_cond_t cond;



int maxSize(){
    int maxWrite = - 1;
    for(int i = 0; i < inputs_amount; i++){
        if (allSizes[i] > maxWrite){
            maxWrite = allSizes[i];
        }
    }
    return maxWrite;
}

void xorWithBuffer(char fromFile[], int bytesAmount){
    for(int i = 0; i < bytesAmount; i++){
        BUFFER[i] = BUFFER[i] ^ fromFile[i];
    }
}

bool allDone(){
    for(int i = 0; i < inputs_amount; i++){
        if(hasXored[i] != true && hasFinished[i] != true){
            return false;
        }
    }
    return true;
}

void handelXorArr(){
    for(int i = 0; i < inputs_amount; i++){
        if(hasFinished[i]){
            hasXored[i] = true;
        }
        else{
            hasXored[i] = false;
        }
    }
}



void* func(void *arg){
    int ix = (int)arg;
    int fd = descriptors[ix];
    char buffer[CHUNK];
    int read_bytes;
    int written_bytes;
    read_bytes = read(fd, buffer, CHUNK);
    
    if(read_bytes < 0){
        printf("Problem with read. %s\n", strerror(errno));
        exit(-1);
    }
    
    while(read_bytes  > 0){
        allSizes[ix] = read_bytes;
        pthread_mutex_lock(&mx);
        
        xorWithBuffer(buffer, read_bytes);
        hasXored[ix] = true;
        if(allDone()){
            
            written_bytes = write(fd_o, BUFFER, maxSize());
            if(written_bytes < 0){
                printf("Problem with write. %s\n", strerror(errno));
                exit(-1);
            }
            memset(allSizes, 0, inputs_amount);
            memset(BUFFER, 0, CHUNK);
            handelXorArr();
            if(pthread_cond_broadcast(&cond) != 0){
                printf("Problem with broadcast. %s\n", strerror(errno));
                exit(-1);
            }
        }
        else{
            pthread_cond_wait(&cond, &mx);
        }
        pthread_mutex_unlock(&mx);
        read_bytes = read(fd, buffer, CHUNK);
        if(read_bytes < 0){
            printf("Problem with read. %s\n", strerror(errno));
            exit(-1);
        }
        if(read_bytes < CHUNK){
            
            hasFinished[ix] = true;
        }
        
    }
    close(fd);
    pthread_exit(NULL);
}
    


int main(int argc, char* argv[]){
    assert(argc >= 3);
    char* dest = argv[1];
    printf(MSG_I, dest, argc - 2);
    fd_o = open(dest, O_TRUNC|O_CREAT|O_WRONLY, 0777);
    if(fd_o < 0){
        printf("Output file opening failed. %s\n", strerror(errno));
        return -1;
    }
    if(pthread_mutex_init(&mx, NULL) != 0){
        printf("Initializing mutex failed. %s\n", strerror(errno));
        return -1;
    }
    if(pthread_cond_init (&cond, NULL) != 0){
        printf("Problem with condition variable failed. %s\n", strerror(errno));
        return -1;
    }
    inputs_amount = argc - 2;
    allSizes = (int*) malloc(sizeof(int*) * inputs_amount);
    if(allSizes == NULL){
        printf("problem with malloc");
    }
    memset(allSizes, 0, inputs_amount);
    hasFinished = (bool*) malloc(sizeof(bool*) * inputs_amount);
    if(hasFinished == NULL){
        printf("problem with malloc");
    }
    memset(hasFinished, false, inputs_amount);
    hasXored = (bool*) malloc(sizeof(bool*) * inputs_amount);
    if(hasXored == NULL){
        printf("problem with malloc");
    }
    memset(hasXored, false, inputs_amount);
    memset(BUFFER, 0, CHUNK);
    pthread_t *tid = (pthread_t*) malloc(sizeof(pthread_t*) * inputs_amount);
    if(tid == NULL){
        printf("problem with malloc");
    }
    descriptors = (int*) malloc(sizeof(int*) * inputs_amount);
    if(descriptors == NULL){
        printf("problem with malloc");
    }
    int fd_i = -2;
    int i;
    int rc;
    void *status;
    int ix = 0;
    for(i = 2; i < argc; i++){
        fd_i = open(argv[i], O_RDONLY);
        
        if(fd_i < 0){
            printf("Input file opening failed. %s\n", strerror(errno));
            return -1;
        }
        descriptors[i - 2] = fd_i;
        ix = i - 2;
        rc = pthread_create(&tid[i-2] , NULL, func ,(void*)ix);
        if(rc){
            printf("Thread failed. %s\n", strerror(errno));
            return -1;
        }
    }
  
    for(i = 0; i < inputs_amount; i++){
        rc = pthread_join(tid[i], &status);
        if(rc){
            printf("Join failed. %s\n", strerror(errno));
            return -1;
        }
    }
    struct stat st;
    fstat(fd_o, &st);
    printf(MSG_O, dest, st.st_size);
    close(fd_o);
    free(hasFinished);
    free(hasXored);
    free(allSizes);
    free(descriptors);
    pthread_mutex_destroy(&mx);
    pthread_cond_destroy(&cond);
    return 0;
}
