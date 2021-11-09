#ifndef CLIENT_ROUTINES_H_INCLUDED
#define CLIENT_ROUTINES_H_INCLUDED

#include "../common/common.h"

// Client OP codes for the dispatcher
#define CLI_OP_POST 'P'
#define CLI_OP_EXIT 'E'
#define CLI_OP_READ 'R'
#define CLI_OP_DELETE 'D'

// Operational flags
#define WAIT_SERVER_OK

// Global variables
struct sockaddr_in addr;
int sockfd;
user_info client_ui;

/*
	DESCRIPTION:
		Ask if user wants to login or register
		Fill user's info
		Create the socket
		Connect to server
		Call function for login or register
	RETURNS:
		0 in case of success
		-1 in case of unsuccess
		exit(EXIT_FAILURE) in case of error
*/
int login_registration();

/*
	DESCRIPTION:
		Dispatch the different request incoming
	RETURNS:
		0 in case of success
		-1 in case of error
*/ 
int dispatcher();

/*
	DESCRIPTION:
		Post on server
		A post is composed by Subject and Body
		Send two messages
	RETURNS:
		0 in case of success
		-1 in case of error
*/
int post();

/*
	DESCRIPTION:
		Read all the post on server
	RETURNS:
		0 in case of success
		-1 in case of error
*/
int read_all();

/*
	DESCRIPTION:
		Ask the server to delete a post
	RETURNS_
		0 in case of success
		-1 in case of error
*/
int delete_post();

#endif