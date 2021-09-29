#ifndef SERVER_ROUTINES_H_INCLUDED
#define SERVER_ROUTINES_H_INCLUDED
// -------------------------------
#include "common.h"

// Simple response on OP_MSG_BODY
#define SIMPLE_OK_RESPONSE

#define MAX_THREAD 1024

// Initial Server Port number
#define INITIAL_SERV_PORT 6990
#define INITIAL_UID 1000

// Define the length of pending connection request
#define MAX_BACKLOG 1024

// Users file
#define USERS_FILENAME "users.list"

#define MESSAGES_FILENAME "messages.list"
#define INDEX_FILENAME "index.list"
#define FREE_AREAS_FILENAME "free-areas.list"

// Semaphores
int UW;	//Users Write 
int UR;	//Users Read

int MW;	//Messages Write
int MR; //Messages Read


/*
	DESCRIPTION:
		Ask user R or L, for Registration or Login
		Register or Log user
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int login_registration(int acceptfd, user_info* client_ui);
// --------------------------------------------------------
#endif