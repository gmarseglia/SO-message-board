#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
// ------------------
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
// --------------------
#include <netinet/in.h>
#include <netinet/ip.h>
// --------------------
#include <arpa/inet.h>

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#define fflush(stdin) while(getchar() != '\n');
#define exit_failure() exit(EXIT_FAILURE);

#define SIZEOF_CHAR 1
#define SIZEOF_INT 4

// Max sizes
#define MAXSIZE_USERNAME 32
#define MAXSIZE_PASSWD 32
// OP codes
#define OP_MSG_SUBJECT 'm'
#define OP_MSG_BODY 'M'
#define OP_MSG 'm'
// -----------------------
#define OP_READ_REQUEST 'r'
#define OP_READ_RESPONSE 'R'
// -----------------------
#define OP_DELETE_REQUEST 'd'
// -----------------------
#define OP_REG_USERNAME 'u'
#define OP_REG_PASSWD 'p'
#define OP_REG_UID 'i'
// -------------------------
#define OP_LOG_USERNAME 'U'
#define OP_LOG_PASSWD 'P'
#define OP_LOG_UID 'I'
// -------------------------
#define OP_OK 'K'
#define OP_NOT_ACCEPTED 'n'

// Fixed UID
#define UID_ANON 0
#define UID_SERVER 1

// Struct containing the user info
typedef struct user_info {
	char *username;
	char *passwd;
	int uid;
} user_info;

// Struct to encapslute operations
typedef struct operation {
	int uid;
	char code;
	char *text;
} operation;

#define SEP "--------------------------------"

/*
	DESCRIPTION:
		Send a text message in two steps:
		1. Pre-message:
			4 byte of int uid, User ID of the sender
			1 byte of char code, code of the operation
			4 byte of int text_len, Length of the text to send

		2. Actual text

		Text can be NULL

	RETURNS:
		0 in case of success
		-1 in case of error or connection closed
*/
int send_message_to(int sockfd, int uid, char code, char *text);
int send_operation_to(int sockfd, operation op);

/*
	DESCRIPTION:
		Receive from sockfd, both pre-message and text
		Allocates space for text in pointer *text

	RETURNS:
		0 in case of success
		-1 in case of error or connection closed
*/
int receive_message_from(int sockfd, int *uid, char *code, char **text);
int receive_operation_from(int sockfd, operation *op);


/*
	DESCRIPTION:
		Initialize struct sockaddr_in
*/
void sockaddr_in_setup_inaddr(struct sockaddr_in *addr, unsigned int ip_address, int port);
void sockaddr_in_setup(struct sockaddr_in *addr, const char *ip_address, int port);

/*
	DESCRIPTION:
		Print all the data in operation
*/
void print_operation(operation *op);

/*
	DESCRIPTION
		Short version of semop

	RETURN VALUE
		If successful and operation completed returns 0.
		If interrupted by signal returns -1.
		Otherwise exits.
*/
int short_semop(int semaphore, int op);

/*
	DESCRIPTION:
		Print on stderr s and func
		Call exit_failure();
*/
void perror_and_failure(const char *s, const char *func);

#endif