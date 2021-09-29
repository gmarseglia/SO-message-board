#ifndef CLIENT_ROUTINES_H_INCLUDED
#define CLIENT_ROUTINES_H_INCLUDED

#include "common.h"

#define CLI_OP_POST 'P'
#define CLI_OP_EXIT 'E'
#define CLI_OP_READ 'R'

#define WAIT_SERVER_OK


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
int login_registration(int *sockfd, struct sockaddr_in *server_address, user_info *client_ui);

int post(int sockfd, user_info client_ui);

int save(int sockfd, user_info client_ui);

#endif