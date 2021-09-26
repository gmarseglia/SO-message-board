#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include <arpa/inet.h>

#define fflush(stdin) while(getchar() != '\n');

#define INITIAL_SERV_PORT 6990

#define SIZEOF_CHAR 1
#define SIZEOF_INT 4
#define MAXSIZE_USERNAME 32
#define MAXSIZE_PASSWD 32

// OP codes
#define OP_MSG_SUBJECT 'm'
#define OP_MSG_BODY 'M'
#define OP_REG_USERNAME 'u'
#define OP_REG_PASSWD 'p'
#define OP_REG_UID 'i'
#define OP_LOG_USERNAME 'U'
#define OP_LOG_PASSWD 'P'
#define OP_LOG_UID 'I'
#define OP_NOT_ACCEPTED 'n'
#define OP_OK 'K'

// Fixed UID
#define UID_ANON 0
#define UID_SERVER 1

// Struct containing the user info
struct user_info {
	char *username;
	char *passwd;
	int uid;
};

int send_message_to(int sockfd, int uid, char op, char *message);
int receive_message_from(int sockfd, int *uid, char *op, char **recipient);

/*
	DESCRIPTION:
		Send two message:
		1. Pre-message:
			4 byte of int uid, User ID
			1 byte of char op, OPeration code
			4 byte of int message_len, LENgth of the MESSAGE

		2. Actual message:

	RETURNS:
		In case of success: the number of byte written
		In case of error: -1, and print errno
*/
int send_message_to(int sockfd, int uid, char op, char *message){
	int message_len, byte_written;
	message_len = message == NULL ? 0 : strlen(message);

	// iovec used by writev
	struct iovec iov[3];

	iov[0].iov_base = &uid;
	iov[0].iov_len = SIZEOF_INT;

	iov[1].iov_base = &op;
	iov[1].iov_len = SIZEOF_CHAR;

	iov[2].iov_base = &message_len;
	iov[2].iov_len = SIZEOF_INT;

	// Send pre-message:
	//	writev enables to send different arrays as one
	if((byte_written = writev(sockfd, iov, 3)) < 0){
		perror("Error in send_message_to on writev");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("writev has written %d bytes\n", byte_written);
	#endif

	// If messagge == NULL, don't send second part
	if(message == NULL)
		return 0;

	// Send message
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

/*
	DESCRIPTION:
		Receive from sockfd, both pre-message and message
		Allocates space for message in recipient

	RETURNS:
		In case of success: the number of byte read
		In case of closed socket: 0
		In case of error: -1, and print errno
*/
int receive_message_from(int sockfd, int *uid, char *op, char **recipient){
	int message_len;
	int null_uid;
	char null_op;
	int byte_read = 0;

	struct iovec iov[3];

	iov[0].iov_base = uid == NULL ? &null_uid : uid;
	iov[0].iov_len = SIZEOF_INT;

	iov[1].iov_base = op == NULL ? &null_op : op;
	iov[1].iov_len = SIZEOF_CHAR;

	iov[2].iov_base = &message_len;
	iov[2].iov_len = SIZEOF_INT;

	if((byte_read = readv(sockfd, iov, 3)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("readv has read %d bytes, message_len=%d\n",
		byte_read, message_len);
	#endif

	// If message_len == 0 -> sender has not included message
	// If recipient == NULL -> receiver is not interested
	if(message_len == 0 || recipient == NULL){
		*recipient = NULL;
		return 0;
	}

	*recipient = calloc(sizeof(char), message_len + 1);
	memset(*recipient, '\0', message_len + 1);

	if((byte_read = read(sockfd, *recipient, message_len)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("read has read %d bytes, message=%s\n",
		byte_read, *recipient);
	#endif

	return byte_read;
}

/*
	DESCRIPTION:
		Initialize struct sockaddr_in
*/
void sockaddr_in_setup_inaddr(struct sockaddr_in *addr, unsigned int ip_address, int port){
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = ip_address;
	return;
}

/*
	DESCRIPTION:
		Initialize struct sockaddr_in using string as ip address
*/
void sockaddr_in_setup(struct sockaddr_in *addr, const char *ip_address, int port){
	return sockaddr_in_setup_inaddr(addr, inet_addr(ip_address), port);
}
