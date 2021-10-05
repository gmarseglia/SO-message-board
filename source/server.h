#ifndef SERVER_ROUTINES_H_INCLUDED
#define SERVER_ROUTINES_H_INCLUDED

#include "common.h"

// Files
#define USERS_FILENAME "users.list"
#define MESSAGES_FILENAME "messages.list"
#define INDEX_FILENAME "index.list"
#define FREE_AREAS_FILENAME "free-areas.list"

// Operational flags
#define SKIP_FREE_AREAS

// Constant
#define INITIAL_SERV_PORT 6990
#define INITIAL_UID 1000
#define MAX_BACKLOG 1024
#define MAX_THREAD 1024
#define INDEX_LINE_LEN 16 // 8 long offset + 4 int message_len + 4 int UID + 1 char '\n'

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
int login_registration(int acceptfd, user_info *client_ui);

/*
	DESCRIPTION:
		Save the message received
	RETURNS:
		0 in case of success
		-1 in case of unsuccess
		In case of error: exit_failure()
*/
int save(int acceptfd, user_info client_ui, operation *op);


/*
	DESCRIPTION:
		Send to Client all saved messages
	RETURNS:
		0 in case of success
		-1 in case of unsuccess
		In case of error: exit_failure()
*/
int read_response(int acceptfd, user_info client_ui, operation *op);

#endif