#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <memory.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#define _XOPEN_SOURCE
#define MAX_UNSIGNED_SHORT 65535
#define NUM_OF_PRINTABLE_CHARS 95
#define LOWEST_PRINTABLE_CHAR 32
#define HIGHEST_PRINTABLE_CHAR 126
#define BACKLOG_CONST 10

unsigned int* pcc_total;
pthread_mutex_t lock;
int numOfConnectionsOnline = 0;
pthread_t* threads;
int receivedSIGINT = 0;

void SIGINT_handler(int signum);

unsigned short initilize(int argc, char *argv[]);

int startListen(unsigned short port, struct sockaddr_in* serv_addr, socklen_t* addrsize);

void getNewConnection(unsigned short port,int listenfd);

void* handleConnection(void* connectionSocketfd);

unsigned int charsToUnsignedInt(char* buffer);

void unsignedIntToChars(unsigned int num, char buffer[]);

void updatePcc_total(char* buffer,unsigned int N, int connfd);

void finishMain();




int main(int argc, char *argv[])
{
    struct sigaction terminateHandler;
    terminateHandler.sa_handler = SIGINT_handler;
    terminateHandler.sa_flags = 0;
    if (0 > sigaction(SIGINT, &terminateHandler, NULL))
    {
        perror("ERROR14");
        exit(-1);
    }
    unsigned short port;
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    int listenfd  = -1;

    port = initilize(argc,argv);
    listenfd = startListen(port,&serv_addr,&addrsize);

    //start accepting connections
    while (!receivedSIGINT)//exits only on SIGINT
        getNewConnection(port,listenfd);
    finishMain();
}

void SIGINT_handler(int signum)
{
    if (signum == SIGINT)
        receivedSIGINT = 1;
}


unsigned short initilize(int argc, char *argv[])
{
    unsigned short port;
    if (argc < 2)
    {
        printf("Error15: Not enough parameters");
        exit(-1);
    }
    pthread_mutex_init(&lock,NULL);

    //receive parameter
    port = (unsigned short)strtoul(argv[1], NULL, 0);
    if ((int)port == 0 || (int)port > MAX_UNSIGNED_SHORT)
    {
        perror("ERROR16: bad port");
        pthread_mutex_destroy(&lock);
        exit(-1);
    }
    //Initialize a data structure pcc_total
    pcc_total = (unsigned int*)malloc(sizeof(unsigned int)*NUM_OF_PRINTABLE_CHARS);
    if (pcc_total == NULL)
    {
        perror("ERROR17");
        pthread_mutex_destroy(&lock);
        exit(-1);
    }
    memset(pcc_total,0,NUM_OF_PRINTABLE_CHARS);
    return port;
}

int startListen(unsigned short port, struct sockaddr_in* serv_addr, socklen_t* addrsize)
{
    int listenfd  = -1;

    listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    memset( serv_addr, 0, (size_t )*addrsize);

    serv_addr->sin_family = AF_INET;
    // INADDR_ANY = any local machine address
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr->sin_port = htons(port);

    if (0 != bind(listenfd,(struct sockaddr*)serv_addr,*addrsize) )
    {
        perror("Error18 : Bind Failed.");
        exit(-1);
    }

    if( 0 != listen(listenfd,BACKLOG_CONST) )
    {
        perror("Error19 : Listen Failed.");
        exit(-1);
    }
    return listenfd;
}

void getNewConnection(unsigned short port, int listenfd)
{
    int rv;
    int connectionSocketfd;
    //Accept a TCP connection on the specified server port.
    connectionSocketfd = accept(listenfd,NULL,NULL);
    if( connectionSocketfd < 0 )
    {
        if (errno == 4) //stop by SIGINT
            receivedSIGINT = 1;
        else // accept failed
        {
            perror("Error20 : Accept Failed.");
            exit(-1);
        }

    }
    else //connection established
    {
        //When a connection is accepted, start a Client Processor thread
        numOfConnectionsOnline++;
        threads = (pthread_t*)realloc(threads,sizeof(pthread_t)*numOfConnectionsOnline-1);
        if (threads == NULL)
        {
            perror("ERROR21");
            pthread_mutex_destroy(&lock);
            exit(-1);
        }
        rv = pthread_create(&threads[numOfConnectionsOnline-1], NULL, handleConnection, (void*)&connectionSocketfd );
        if (rv)
        {
            printf("ERROR22 in pthread_create(): %s\n", strerror(rv));
            free(threads);
            free(pcc_total);
            pthread_mutex_destroy(&lock);
            exit(-1);
        }
    }
}


void* handleConnection(void* connectionSocketfd)
{
    int connfd = *(int*)connectionSocketfd;
    char lengthBuffer[sizeof(unsigned int)];
    int bytes_read = 0;
    int lastRead;
    unsigned int N = 0;
    char* buffer;

    memset(lengthBuffer,0,sizeof(lengthBuffer));
    //read number of bytes N that client about to send
    bytes_read = (int) read(connfd, lengthBuffer, (int)sizeof(lengthBuffer));

    while ( bytes_read < (int)sizeof(lengthBuffer))
    {
        if (bytes_read < 0)
        {
            perror("ERROR24: failed to get N from client\n");
            exit(-1);
        }
        lastRead = bytes_read;
        bytes_read += read(connfd, lengthBuffer+lastRead, sizeof(lengthBuffer)-lastRead);
    }
    N = charsToUnsignedInt(lengthBuffer);

    //read stream of bytes from client
    buffer = (char*)calloc((size_t)N,sizeof(char));
    if (buffer == NULL)
    {
        perror("ERROR23");
        exit(-1);
    }
    bytes_read = (int) read(connfd, buffer, N);
    while ( bytes_read < N)
    {
        if (bytes_read < 0)
        {
            perror("ERROR10");
            exit(-1);
        }
        lastRead = bytes_read;
        bytes_read += read(connfd, buffer+lastRead, N-lastRead);
    }

    if( N == bytes_read)//count num of printable and update pcc
        updatePcc_total(buffer,N,connfd);
    else
        printf("ERROR26: Server got less than N=%d chars, number of bytes:%d\n",N,bytes_read);
    free(buffer);
    //close connection
    close(connfd);
}

unsigned int charsToUnsignedInt(char* buffer)
{
    unsigned int N;
    N = (unsigned int)(buffer[0]) << 24;
    N |= (unsigned int)(buffer[1]) << 16;
    N |= (unsigned int)(buffer[2]) << 8;
    N |= (unsigned int)(buffer[3]);
    return N;
}

void unsignedIntToChars(unsigned int num, char buffer[])
{
    buffer[0] = (char)((num >> 24) & 0xFF);
    buffer[1] = (char)((num >> 16) & 0xFF);
    buffer[2] = (char)((num >> 8) & 0xFF);
    buffer[3] = (char)(num & 0xFF);
}

void updatePcc_total(char* buffer,unsigned int N, int connfd)
{
    //count num of printable and update pcc
    char numOfPrintablesBuffer[sizeof(unsigned int)];
    unsigned int numOfPrintables = 0;
    int i,totalSent,notWritten,lastWrite;
    unsigned int local_pcc[NUM_OF_PRINTABLE_CHARS];

    memset(numOfPrintablesBuffer,0,sizeof(numOfPrintablesBuffer));
    memset(local_pcc,0,sizeof(local_pcc));
    for (i=0; i<N; i++)
    {
        if (buffer[i] >= LOWEST_PRINTABLE_CHAR && buffer[i] <= HIGHEST_PRINTABLE_CHAR)
        {
            numOfPrintables++;
            local_pcc[buffer[i]-LOWEST_PRINTABLE_CHAR]++;
        }
    }

    pthread_mutex_lock(&lock);
    for (i=0; i<NUM_OF_PRINTABLE_CHARS; i++)
            pcc_total[i] += local_pcc[i];
    pthread_mutex_unlock(&lock);

    //return number of printables chars to client (used code from stackoverflow)
    unsignedIntToChars(numOfPrintables,numOfPrintablesBuffer);
    notWritten = (int)sizeof(unsigned int); // how much we have left to write
    totalSent = 0; // how much we've written so far
    lastWrite = -1; // how much we've written in last write() call */

    // keep looping until nothing left to write
    while( notWritten > 0 )
    {
        lastWrite = (int) write(connfd, numOfPrintablesBuffer + totalSent, notWritten);
        if ( lastWrite < 0)
        {
            perror("24");
            exit(-1);
        }
        totalSent  += lastWrite;
        notWritten -= lastWrite;
    }
}


void finishMain()
{
    int i, status;
    int returnValue;
    int num = 0;

    while (num < numOfConnectionsOnline)
    {
        returnValue = pthread_join(threads[num], (void*)&status);
        if (returnValue)
        {
            printf("ERROR25 in pthread_join():%s\n", strerror(returnValue));
            free(pcc_total);
            free(threads);
            pthread_mutex_destroy(&lock);
            exit(-1);
        }
        num++;
    }
    //print num of app for EACH character
    printf("\n");
    for (i=0; i<NUM_OF_PRINTABLE_CHARS; i++)
        printf("char '%c' : %u times\n",(char)(i+LOWEST_PRINTABLE_CHAR),pcc_total[i]);
    free(pcc_total);
    free(threads);
    pthread_mutex_destroy(&lock);
    pthread_exit(0);
}
