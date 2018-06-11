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

unsigned int sendingDataToServer(char* buffer,int socketNum,unsigned int length);

void convertToChars(unsigned int num, char buffer[]);

unsigned int convertToUnsignedInt(char* buffer);


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

	//sending data to the server
    	numOfPrintableCharacters = sendingDataToServer(buffer, socketNum, length);
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

unsigned int sendingDataToServer(char* buffer,int socketNum,unsigned int length) {

	char numOfPrintablesInChars[4];
    	unsigned int numOfPrintableCharacters;
	ssize_t numOfBytesReadWrote;
	ssize_t currentReadWrite;
	ssize_t anotherReadWrite;	
	char lengthInChars[sizeof(unsigned int)];
	int lengthToReadWrite;

	// sending length to server
	lengthToReadWrite = (int)sizeof(unsigned int);
	convertToChars(length,lengthInChars);
	numOfBytesReadWrote = (int)write(socketNum, lengthInChars, (size_t)lengthToReadWrite);
	if (numOfBytesReadWrote < 0) {
		perror("Could not write length to server");
		exit(EXIT_FAILURE);                         
	}
   	while(numOfBytesReadWrote < lengthToReadWrite) {
		currentReadWrite = numOfBytesReadWrote;
		anotherReadWrite = (int)write(socketNum, lengthInChars+currentReadWrite, (size_t)lengthToReadWrite-currentReadWrite);
		if (anotherReadWrite < 0) {
			perror("Could not write length to server");
			exit(EXIT_FAILURE);
		}
		numOfBytesReadWrote = numOfBytesReadWrote + anotherReadWrite;
	}

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
    	return numOfPrintableCharacters;
}

unsigned int convertToUnsignedInt(char* buffer)
{
    unsigned int N;
    N = (unsigned int)(buffer[0]) << 24;
    N |= (unsigned int)(buffer[1]) << 16;
    N |= (unsigned int)(buffer[2]) << 8;
    N |= (unsigned int)(buffer[3]);
    return N;
}


void convertToChars(unsigned int num, char buffer[])
{
    buffer[0] = (char)((num >> 24) & 0xFF);
    buffer[1] = (char)((num >> 16) & 0xFF);
    buffer[2] = (char)((num >> 8) & 0xFF);
    buffer[3] = (char)(num & 0xFF);
}

