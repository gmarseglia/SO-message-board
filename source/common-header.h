#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
// Implicit declaration solved: https://stackoverflow.com/questions/11049687/something-about-inet-ntoa
#include <arpa/inet.h>

#define fflush(stdin) while(getchar() != '\n');

#define SERV_PORT 6990
#define BUFFER_SIZE 1024

#define SIZEOF_UNS_INT 4
#define SIZEOF_CHAR 1

#define OP_MSG "m"

int send_message_to(int sockfd, char *op, char *message){
	unsigned int byteWrite;
	char *buffer;
	int buffer_size;

	buffer_size = SIZEOF_CHAR + SIZEOF_UNS_INT;

	if((buffer = malloc(buffer_size)) == NULL){
		perror("Error in send_message_to on malloc.\n");
		return -1;
	}

	byteWrite = strlen(message);

	buffer[0] = *op;
	memcpy(&buffer[1], &byteWrite, SIZEOF_UNS_INT);

	#ifdef PRINT_DEBUG_FINE
	printf("Sending %d bytes=%c,%d\n", buffer_size, buffer[0], buffer[1]);
	#endif

	if(send(sockfd, &buffer, buffer_size, 0) < 0){
		perror("Error in send_message_to on first send.\n");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("Sending %d bytes=%s\n", byteWrite, message);
	#endif

	if(send(sockfd, message, byteWrite, 0) < 0){
		perror("Error in send_message_to on second send.\n");
		return -1;
	}

	return 0;
}