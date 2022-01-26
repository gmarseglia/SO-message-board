/*
**	Common header for both server and client .c files.
*/

/* Headers */
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

#ifndef COMMON_H_INCLUDED	/* Double-inclusion guard*/
	#define COMMON_H_INCLUDED

	/* Macros */
	#define fflush(stdin) while(getchar() != '\n');
	#define exit_failure() exit(EXIT_FAILURE);

	/* Definitions */
	#define MAXSIZE_USERNAME (32)
	#define MAXSIZE_PASSWD (32)
	
	/* Operations code */
	#define OP_OK 'K'
	#define OP_NOT_ACCEPTED 'n'
	#define OP_MSG 'm'
	#define OP_READ_REQUEST 'r'
	#define OP_READ_RESPONSE 'R'
	#define OP_DELETE_REQUEST 'd'
	#define OP_REG 'u'
	#define OP_LOG 'U'

	/* Defined User IDs */
	#define UID_ANON (0)
	#define UID_SERVER (1)

	/* User credentials */
	typedef struct user_info_t {
		char *username;
		char *passwd;
		int uid;
	} user_info_t;

	/* Operation */
	typedef struct operation_t {
		int uid;
		char code;
		char *text;
	} operation_t;

	#define SEP "-----------------------------------"

	/* Functions prototypes */

	/*
	**	DESCRIPTION
	**		Send a text message in two steps:
	**		Step 1:
	**			4 byte of uint32_t uid, User ID of the sender.
	**			1 byte of char code, code of the operation_t.
	**			4 byte of uint32_t text_len, Length of the text to send.
	**
	**		2. Actual text, if NULL then text is not sent.
	**
	**	RETURN VALUE
	**		In case of success: 0
	**		In case of error or connection closed: -1
	*/
	int send_operation_to(int sockfd, int uid, char code, char *text);
	int send_operation_to_2(int sockfd, operation_t op);

	/*
	**	DESCRIPTION
	**		Receive message from sockfd.
	**		Allocates space for text in pointer *text.
	**			Space allocated should be freed upon use.
	**		uid, code and text can be NULL.
	**
	**	RETURN VALUE
	**		In case of success: 0
	**		In case of error or connection closed: -1
	*/
	int receive_operation_from(int sockfd, int *uid, char *code, char **text);
	int receive_operation_from_2(int sockfd, operation_t *op);


	/*
	**	DESCRIPTION
	**		Initialize struct sockaddr_in
	*/
	void sockaddr_in_setup_inaddr(struct sockaddr_in *addr, unsigned int ip_address, int port);
	void sockaddr_in_setup(struct sockaddr_in *addr, const char *ip_address, int port);

	/*
	**	DESCRIPTION
	**		Print all the data in operation_t
	*/
	void print_operation(operation_t *op);

	/*
	**	DESCRIPTION
	**		Short version of semop
	**
	**	RETURN VALUE
	**		If successful and operation_t completed: 0.
	**		If interrupted by signal: -1.
	**		Otherwise exits.
	*/
	int short_semop(int semaphore, int op);

	/*
	**	DESCRIPTION
	**		Print on stderr s and func
	**		Call exit_failure();
	*/
	void perror_and_failure(const char *s, const char *func);

#endif
