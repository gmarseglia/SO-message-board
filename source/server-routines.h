#ifndef SERVER_ROUTINES_H_INCLUDED
#define SERVER_ROUTINES_H_INCLUDED

#include "common.h"

extern int UR;
extern int UW;
extern const int MAX_THREADS;
extern const char *users_filename;

/*
	DESCRIPTION:
		Ask user R or L, for Registration or Login
		Call correct function
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int login_registration(int acceptfd, user_info* client_ui);

#endif