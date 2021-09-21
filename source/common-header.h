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
#include <sys/uio.h>

#define fflush(stdin) while(getchar() != '\n');

#define SERV_PORT 6990
#define SIZEOF_CHAR 1
#define SIZEOF_INT 4

int send_message_to(int sockfd, char *op, char *message){
	int message_len, byte_written;
	struct iovec iov[2];

	message_len = strlen(message);

	iov[0].iov_base = op;
	iov[0].iov_len = SIZEOF_CHAR;

	iov[1].iov_base = &message_len;
	iov[1].iov_len = SIZEOF_INT;

	if((byte_written = writev(sockfd, iov, 2)) < 0){
		perror("Error in send_message_to on writev");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("writev has written %d bytes\n", byte_written);
	#endif

	if((byte_written = write(sockfd, message, message_len)) 
		!= message_len){
		perror("Error in send_message_to on write");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("write has written %d bytes=%s\n", byte_written, message);
	#endif

	return byte_written;
}

int receive_message_from(int sockfd, char *op, char **container){
	int message_len;
	int byte_read = 0;

	struct iovec iov[2];

	iov[0].iov_base = op;
	iov[0].iov_len = SIZEOF_CHAR;

	iov[1].iov_base = &message_len;
	iov[1].iov_len = SIZEOF_INT;

	if((byte_read = readv(sockfd, iov, 2)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("readv has read %d bytes, message_len=%d\n",
		byte_read, message_len);
	#endif

	*container = calloc(sizeof(char), message_len + 1);
	memset(*container, '\0', message_len + 1);

	if((byte_read = read(sockfd, *container, message_len)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("read has read %d bytes, message=%s\n",
		byte_read, *container);
	#endif

	return byte_read;
}