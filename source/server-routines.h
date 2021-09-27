#ifndef SERVER_ROUTINES_H_INCLUDED
#define SERVER_ROUTINES_H_INCLUDED
// -------------------------------
#include "common.h"

// Simple response on OP_MSG_BODY
#define SIMPLE_OK_RESPONSE

extern int UR;
extern int UW;
extern const int MAX_THREADS;
extern const char *users_filename;

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