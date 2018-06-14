#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

in_addr_t hostNameToIp(char* serverHost);

int getConnection(unsigned short port,in_addr_t serverIP, unsigned int length);

char* getRandomChars(unsigned int length);

unsigned int sendToConnection(char* bufferToSend,int sockfd,unsigned int length);

void sendLengthToServer(int sockfd,unsigned int length);

void sendRandomCharsToServer(char* bufferToSend,int sockfd,unsigned int length);

void unsignedIntToChars(unsigned int num, char buffer[]);

unsigned int charsToUnsignedInt(char* buffer);


int main(int argc, char *argv[]) {
    if (argc < 4)
    {
        printf("Error1: Not enough parameters");
        exit(-1);
    }

    int sockfd;
    char* serverHost;
    in_addr_t serverIP;
    unsigned short port;
    unsigned int length;
    unsigned int numOfPrintable;
    char* bufferToSend;

    serverHost = argv[1];
    //may need to convert the host name to an IP address first
    serverIP = hostNameToIp(serverHost);
    port = (unsigned short)strtoul(argv[2], NULL, 0);
    if ((int)port == 0)
    {
        perror("ERROR2: bad port");
        exit(-1);
    }
    length = (unsigned int)strtoul(argv[3], NULL, 0);
    if ((int)length == 0)
    {
        perror("ERROR3: bad char num");
        exit(-1);
    }

    //1. Create a TCP connection to the specified server port on the specified server host.
    sockfd = getConnection(port,serverIP,length);
    //2. Open /dev/urandom for reading.
    bufferToSend = getRandomChars(length);
    //3. Transfer the specified length bytes from /dev/urandom to the server over the TCP connection.
    numOfPrintable = sendToConnection(bufferToSend, sockfd, length);
    //4. Obtain the count of printable characters computed by the server, C (an unsigned int), and print it
    printf("# of printable characters: %u\n", numOfPrintable);
    close(sockfd);
    free(bufferToSend);
    exit(0);
}

in_addr_t hostNameToIp(char* serverHost) {
	in_addr_t ip;
    struct in_addr addr;
    struct addrinfo hints, *servinfo;

    memset(&addr, 0, sizeof(addr));
    //if regular ip, use inet_aton() and return
    if (0 == inet_aton(serverHost, &addr))
    {
        //if name, try to convert using getaddrinfo()
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(serverHost , NULL, &hints , &servinfo) != 0)
        {
            perror("ERROR4");
            exit(-1);
        }
        struct sockaddr *sa = servinfo->ai_addr;
        struct sockaddr_in *sin = (struct sockaddr_in *) sa;
        serverHost = inet_ntoa(sin->sin_addr);
        if (0 == inet_aton(serverHost, &addr))
        {
            perror("ERROR5: bad IP");
            exit(-1);
        }
    }
    ip = addr.s_addr;
    return ip;
}

int getConnection(unsigned short port, in_addr_t serverIP, unsigned int length)
{
    int sockfd = -1;
    struct sockaddr_in serv_addr;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error6 : Could not create socket.");
        exit(-1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = serverIP;

    if(0 > connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)))
    {
        perror("Error7 : Connect Failed.");
        exit(-1);
    }
    return sockfd;
}

char* getRandomChars(unsigned int length)
{
    char* bufferToSend;
    ssize_t result;
    ssize_t lastResult;
    bufferToSend = (char*)malloc(sizeof(char)*length);

    if (bufferToSend == NULL)
    {
        perror("ERROR8");
        exit(-1);
    }
    memset(bufferToSend,0,sizeof(bufferToSend));

    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData < 0)
    {
        perror("ERROR9");
        exit(-1);
    }

    result = read(randomData, bufferToSend, length);
    while ( result < length)
    {
        if (result < 0)
        {
            perror("ERROR10");
            exit(-1);
        }
        lastResult = result;
        result += read(randomData, bufferToSend+lastResult, sizeof(bufferToSend)-lastResult);
    }

    return bufferToSend;
}

unsigned int sendToConnection(char* bufferToSend,int sockfd,unsigned int length)
{
    unsigned int numOfPrintables;
    char numOfPrintablesBuffer[sizeof(unsigned int)];
    int bytes_read, lastRead;

    sendLengthToServer(sockfd,length);
    sendRandomCharsToServer(bufferToSend,sockfd,length);

    //######### get number of printables chars ######################
    memset(numOfPrintablesBuffer,0,sizeof(numOfPrintablesBuffer));
    bytes_read = (int) read(sockfd, numOfPrintablesBuffer, (int)sizeof(unsigned int));
    while ( bytes_read < (int)sizeof(unsigned int))
    {
        if (0 > bytes_read)
        {
            perror("ERROR11");
            exit(-1);
        }
        lastRead = bytes_read;
        bytes_read += read(sockfd, numOfPrintablesBuffer+lastRead, sizeof(unsigned int)-lastRead);
    }

    numOfPrintables = charsToUnsignedInt(numOfPrintablesBuffer);
    return numOfPrintables;
}

void sendLengthToServer(int sockfd,unsigned int length)
{
    //############# send length N number of bytes to server ###########
    char lengthBuffer[sizeof(unsigned int)];
    int totalSent;
    int notWritten;
    int lastWrite;

    unsignedIntToChars(length,lengthBuffer);
    notWritten = (int)sizeof(unsigned int); // how much we have left to write
    totalSent = 0; // how much we've written so far
    lastWrite = -1; // how much we've written in last write() call */

    // keep looping until nothing left to write
    while( notWritten > 0 )
    {
        lastWrite = (int) write(sockfd, lengthBuffer + totalSent, (size_t)notWritten);
        if ( lastWrite < 0)
        {
            perror("ERROR12");
            exit(-1);
        }
        totalSent  += lastWrite;
        notWritten -= lastWrite;
    }
}

void sendRandomCharsToServer(char* bufferToSend,int sockfd,unsigned int length)
{
    //######### send N random chars to server ######################
    int totalSent;
    int notWritten;
    int lastWrite;

    notWritten = length; // how much bytes we have left to write
    totalSent = 0; // how much we've written so far
    lastWrite = -1; // how much we've written in last write() call */

    // keep looping until nothing left to write
    while( notWritten > 0 )
    {
        lastWrite = (int) write(sockfd, bufferToSend + totalSent, (size_t)notWritten);
        if ( lastWrite < 0)
        {
            perror("ERROR13");
            exit(-1);
        }
        totalSent  += lastWrite;
        notWritten -= lastWrite;
    }
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

