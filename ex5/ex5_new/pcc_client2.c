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
#include <sys/types.h>
#include <string.h>


in_addr_t hostNameToIp(char* serverHost);
unsigned int sendToConnection(char* bufferToSend,int sockfd,unsigned int length);
void convertToChars(unsigned int unsignedIntInput, char buffer[]);
void sendRandomCharsToServer(char* bufferToSend,int sockfd,unsigned int length);
unsigned int convertToUnsignedInt(char* buffer);
void sendLengthToServer(int sockfd,unsigned int length);

int main(int argc, char *argv[]) {
	
	int rv;
	char* serverHost;
	unsigned short serverPort;
	unsigned int length;
	in_addr_t ip;
	struct sockaddr_in serverAddr;
	char* buffer;
	int randomFile;
	ssize_t dataRead;
	ssize_t currentRead;
	ssize_t anotherRead;
	unsigned int numOfPrintableCharacters;
	int socketNum;

	// initial checkups
	if (argc < 4) {
	    	printf("Invalid usage of this function please use:\n./prog <server_host> <server_port> <length>");
		exit(EXIT_FAILURE);
    	}
    	serverHost = argv[1];
	serverPort = (unsigned short)strtoul(argv[2], NULL, 0);
	if (serverPort == 0) {
		perror("Not a valid port!");    
		exit(EXIT_FAILURE);
	}    
	length = (unsigned int)strtoul(argv[3], NULL, 0);    
	if (length == 0) {
		perror("Not a valid length!");
		exit(EXIT_FAILURE);    
	}
    	printf("server: %s, port: %s, length: %s\n",argv[1], argv[2], argv[3]);
	// initilizing all host parameters
	ip = hostNameToIp(serverHost);
	socketNum = socket(AF_INET, SOCK_STREAM, 0);
	if(socketNum < 0) {
		perror("Error in creating socket");
		exit(EXIT_FAILURE);
	}
	serverAddr.sin_addr.s_addr = ip;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_family = AF_INET;
	memset(&serverAddr, 0, sizeof(serverAddr));
	printf("Before connecting to server\n");	
	// conecting to server
	rv = connect(socketNum, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	if(rv < 0) {
		perror("Error in Connecting to server");
		exit(EXIT_FAILURE);
	}
	
	// getting the tandom characters to buffer
	buffer = (char*)malloc(sizeof(char)*length);
	if (buffer == NULL) {
		perror("Error in mallocing buffer");
		exit(EXIT_FAILURE);
	}
	memset(buffer,0,length);
	randomFile = open("/dev/urandom", O_RDONLY);
	if (randomFile < 0) {
		perror("Could not open urandom!");
		exit(EXIT_FAILURE); 
	}
	dataRead = read(randomFile, buffer, length);
	if (dataRead < 0) {
		perror("Could not read from urandom");
		exit(EXIT_FAILURE);
	}
	while (dataRead < length) {
		currentRead = dataRead;
		anotherRead = read(randomFile, currentRead+buffer, length-currentRead);
		if (anotherRead < 0) {
			perror("Could not read from urandom");
			exit(EXIT_FAILURE);
		}
		dataRead = dataRead + anotherRead;
	}
	printf("After buffer read\n");
	//sending data to the server
    	numOfPrintableCharacters = sendToConnection(buffer, socketNum, length);
	printf("After sending data to server\n");
    	printf("# of printable characters: %u\n", numOfPrintableCharacters);
    	close(socketNum);
    	free(buffer);
    	exit(EXIT_SUCCESS);
}

in_addr_t hostNameToIp(char* serverHost) {
	int rv;
    	struct in_addr serverInAddr;
    	struct addrinfo hints;
	struct addrinfo *serverInfo;
	in_addr_t ip;
     	
	memset(&serverInAddr, 0, sizeof(struct in_addr));
    	memset(&hints, 0, sizeof(struct addrinfo));
	rv = inet_aton(serverHost, &serverInAddr);
    	if (rv == 0) {
        	hints.ai_family = AF_INET;
	        hints.ai_socktype = SOCK_STREAM;
		rv = getaddrinfo(serverHost , NULL, &hints , &serverInfo);
        	if (rv != 0) {
	    		perror("Error in getaddrinfo()");
	    		exit(EXIT_FAILURE);
		}
		struct sockaddr *socketAddr = serverInfo->ai_addr;
		struct sockaddr_in *socketAddrIn = (struct sockaddr_in *)socketAddr;
	        serverHost = inet_ntoa(socketAddrIn->sin_addr);
		rv = inet_aton(serverHost, &serverInAddr);
	        if (rv == 0) {
	    		perror("Error not valid IP address");
			exit(EXIT_FAILURE);
		}
    	}
    	ip = serverInAddr.s_addr;
    	return ip;
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

    numOfPrintables = convertToUnsignedInt(numOfPrintablesBuffer);
    return numOfPrintables;
}

void sendLengthToServer(int sockfd,unsigned int length)
{
    //############# send length N number of bytes to server ###########
    char lengthBuffer[sizeof(unsigned int)];
    int totalSent;
    int notWritten;
    int lastWrite;

    convertToChars(length,lengthBuffer);
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
/*
unsigned int sendingDataToServer(char* buffer,int socketNum,unsigned int length) {
	char numOfPrintablesInChars[4];
    	unsigned int numOfPrintableCharacters;
	ssize_t numOfBytesReadWrote;
	ssize_t currentReadWrite;
	ssize_t anotherReadWrite;	
	char lengthInChars[sizeof(unsigned int)];
	int lengthToReadWrite;
	printf("Sending length\n");
	// sending length to server
	lengthToReadWrite = (int)sizeof(unsigned int);
	convertToChars(length,lengthInChars);
	printf("before write %c%c%c%c\n",lengthInChars[0],lengthInChars[1],lengthInChars[2],lengthInChars[3]);
	printf("writing socketNum %d, lengthToReadWrite: %d\n",socketNum,lengthToReadWrite);
	numOfBytesReadWrote = (int) write(socketNum, lengthInChars+0, (size_t)lengthToReadWrite);
	if (numOfBytesReadWrote < 0) {
		perror("Could not write length to server");
		exit(EXIT_FAILURE);                         
	}
	printf("after written the first time\n");
   	while(numOfBytesReadWrote < lengthToReadWrite) {
		currentReadWrite = numOfBytesReadWrote;
		anotherReadWrite = (int)write(socketNum, lengthInChars+currentReadWrite, (size_t)lengthToReadWrite-currentReadWrite);
		if (anotherReadWrite < 0) {
			perror("Could not write length to server");
			exit(EXIT_FAILURE);
		}
		numOfBytesReadWrote = numOfBytesReadWrote + anotherReadWrite;
	}
	printf("Sending data\n");
	// sending data to server
	numOfBytesReadWrote = (int)write(socketNum, buffer, (size_t)length);
	if (numOfBytesReadWrote < 0) {
		perror("Could not write data to server");
		exit(EXIT_FAILURE);
	}
	while( numOfBytesReadWrote < length ) {
		currentReadWrite = numOfBytesReadWrote;
		anotherReadWrite = (int)write(socketNum, buffer+currentReadWrite, (size_t)length-currentReadWrite);
		if (anotherReadWrite < 0) {
			perror("Could not write length to server");
			exit(EXIT_FAILURE);
		}
		numOfBytesReadWrote = numOfBytesReadWrote + anotherReadWrite;
	}	
	printf("recieving response\n");
	//recieving response from server
    	memset(numOfPrintablesInChars, 0, sizeof(numOfPrintablesInChars));
    	numOfBytesReadWrote = read(socketNum, numOfPrintablesInChars, (size_t)lengthToReadWrite);
	if (numOfBytesReadWrote < 0) {
		perror("Could not read from server");
		exit(EXIT_FAILURE);
	}
    	while (numOfBytesReadWrote < lengthToReadWrite) { 
		currentReadWrite = numOfBytesReadWrote;
		anotherReadWrite = read(socketNum, numOfPrintablesInChars+currentReadWrite, (size_t)lengthToReadWrite-currentReadWrite);
		if (anotherReadWrite < 0) {
			perror("Could not read from server");
			exit(EXIT_FAILURE);
		}
		numOfBytesReadWrote = numOfBytesReadWrote + anotherReadWrite;
	}
    	numOfPrintableCharacters = convertToUnsignedInt(numOfPrintablesInChars);
	printf("returning protible letters\n");
    	return numOfPrintableCharacters;
}
*/
unsigned int convertToUnsignedInt(char* buffer)
{
    unsigned int result = (unsigned int)(buffer[0]) << 24;
    result |= (unsigned int)(buffer[1]) << 16;
    result |= (unsigned int)(buffer[2]) << 8;
    result |= (unsigned int)(buffer[3]);
    return result;
}


void convertToChars(unsigned int unsignedIntInput, char buffer[]) {
//	printf("converting to chars unsigned int: %d\n",(int)unsignedIntInput);
    	buffer[0] = (unsigned char)((unsignedIntInput >> 24 & 255));
    	buffer[1] = (unsigned char)((unsignedIntInput >> 16 & 255));
    	buffer[2] = (unsigned char)((unsignedIntInput >> 8 & 255));
    	buffer[3] = (unsigned char)(unsignedIntInput & 255);
//	printf("buffer is %c%c%c%c\n",buffer[0],buffer[1],buffer[2],buffer[3]);
}
