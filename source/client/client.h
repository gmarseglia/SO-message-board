#ifndef CLIENT_ROUTINES_H_INCLUDED
#define CLIENT_ROUTINES_H_INCLUDED

#include "../common/common.h"

// Client OP codes for the dispatcher
#define ACTION_POST 'P'
#define ACTION_EXIT 'E'
#define ACTION_READ 'R'
#define ACTION_DELETE 'D'

// Global variables
struct sockaddr_in addr;
int sockfd;
user_info_t client_ui;
sigset_t sigset_all_blocked, sigset_sigint_allowed;

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
int post_message();

/*
	DESCRIPTION:
		Read all the post on server
	RETURNS:
		0 in case of success
		-1 in case of error
*/
int read_all_messages();

/*
	DESCRIPTION:
		Ask the server to delete a post
	RETURNS_
		0 in case of success
		-1 in case of error
*/
int delete_message();

#endif