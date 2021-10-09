#ifndef SERVER_ROUTINES_H_INCLUDED
#define SERVER_ROUTINES_H_INCLUDED

#include "common.h"

struct thread_arg{
	int id;
	int acceptfd;
	struct sockaddr_in *client_addr;
};

// Files
#define USERS_FILENAME "users.list"
#define MESSAGES_FILENAME "messages.list"
#define INDEX_FILENAME "index.list"
#define FREE_AREAS_FILENAME "free-areas.list"

#define FDOPEN_USERS() fdopen(open(USERS_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
#define FDOPEN_MESSAGES() fdopen(open(MESSAGES_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
#define FDOPEN_INDEX() fdopen(open(INDEX_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
#define FDOPEN_FREE_AREAS() fdopen(open(FREE_AREAS_FILENAME, O_CREAT | O_RDWR, 0660), "r+");

// Operational flags
#define SKIP_FREE_AREAS

// Constant
#define INITIAL_SERV_PORT 6990
#define INITIAL_UID 1000
#define MAX_BACKLOG 1024
#define MAX_THREAD 1024
#define INDEX_LINE_LEN 16 // 8 long offset + 4 int message_len + 4 int UID + 1 char '\n'
#define FREE_AREAS_LINE_LEN 12 // 8 uint64_t message_offset + 4 uint32_t message_len

// Semaphores
int UW;	//Users Write 
int UR;	//Users Read
// ---------------------
int MW;	//Messages Write
int MR; //Messages Read

/*
	DESCRIPTION:
		Ask user R or L, for Registration or Login
		Register or Log user
	RETURNS:
		0 in case of success
		-1 in case of unsuccess
		In case of error: exit_failure()
*/
int login_registration();

/*
	DESCRIPTION:
		Dispatch the different request incoming
	RETURNS:
		0 in case of success
		-1 in case of closed connection
*/ 
int dispatcher();

/*
	DESCRIPTION:
		Save the message received
	RETURNS:
		0 in case of success
		-1 in case of unsuccess
		In case of error: exit_failure()
*/
int post();

/*
	DESCRIPTION:
		Send to Client all saved messages
	RETURNS:
		0 in case of success
		-1 in case of unsuccess
		In case of error: exit_failure()
*/
int read_all();

/*
	DESCRIPTION:
		Try to delete MID post
		Successul if user asking to delete is the same user that posted
	RETURNS:
		0 in case of no error
		-1 in case of error
*/
int delete_post();

/*
	DESCRIPTION:
		Look for user by username and if found returs userinfo
		If not found returns NULL
*/
user_info *find_user_by_username(char *username);

/*
	DESCRIPTION:
		Look for user by uid and if found returs userinfo
		If not found returns NULL
*/
user_info *find_user_by_uid(int uid);

void *thread_communication_routine(void *arg);

#endif