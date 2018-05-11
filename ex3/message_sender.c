#include "message_slot.h" 

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

bool checkForInt(char *str2Check);

int main( int argc, char* argv[]) {
  int fileDescriptor;
  int returnValue;
  char *filePath;
  int channelID;
  char *message;

  // check num of arguments
  if (argc != 4) {
	  printf("\nWrong use in function! please insert\n./message_sender <filePath> <channelID> <message>\n");
	  exit(EXIT_FAILURE);
  }
  filePath = argv[1];
  // chech if the channelID is integer
  if (checkForInt(argv[2])) {
	  channelID = atoi(argv[2]);
  } else {
	  printf("\nNot a valid channel ID must be an integer\n");
	  exit(EXIT_FAILURE);
  }
  //checking message
  if (strlen(argv[3]) <= 128) {
	  message = argv[3];
  } else {
	  printf("\nMesssage must be in MAX length of 128\n");
	  exit(EXIT_FAILURE);
  }
  // Open slot device
  fileDescriptor = open(argv[1], O_RDWR);
  if (fileDescriptor < 0) {
	  printf("\nCannot open file\n");
      	  exit(EXIT_FAILURE);
  }
  returnValue = ioctl(fileDescriptor, IOCTL_SET_ENC, 0);
  printf("\nreturnValue of icotl is: %d\n",returnValue);
  returnValue = write(fileDescriptor, message, strlen(message));
  printf("\nreturnValue of write is: %d\n",returnValue);
//  returnValue = read(fileDescriptor, NULL, 100 );
  returnValue = close(fileDescriptor);
  printf("\nreturnValue of release is: %d\n",returnValue);
  exit(EXIT_SUCCESS);
}


bool checkForInt(char *str2Check) {
	int len = strlen(str2Check);
	int i = 0;
	for (i = 0 ; i<len ; i++) {
		if (str2Check[i] < 48 || str2Check[i] > 57)
			return false;
	}
	return true;
}
