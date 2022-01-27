/*
**	Client-side header
*/

#ifndef CLIENT_ROUTINES_H_INCLUDED
	#define CLIENT_ROUTINES_H_INCLUDED

	#include "../common/common.h"

	/* Client OP codes for the dispatcher */
	#define ACTION_POST 'P'
	#define ACTION_EXIT 'E'
	#define ACTION_READ 'R'
	#define ACTION_DELETE 'D'

	/* Global variables */
	struct sockaddr_in addr;
	int sockfd;
	user_info_t client_ui;
	sigset_t set_sigint;

	/*
	**	DESCRIPTION
	**		Call function for login or register
	**
	**	RETURN
	**		0 in case of success
	**		-1 in case of unsuccess
	*/
	int login_registration();

	/*
	**	DESCRIPTION
	**		Dispatch the users' requests
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of error
	*/ 
	int dispatcher();

	/*
	**	DESCRIPTION
	**		Post on server
	**		A post is composed by Subject and Body
	**		Send two messages
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of error (no more operation are possible)
	*/
	int post_message();

	/*
	**	DESCRIPTION
	**		Read all the post on server
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of error (no more operation are possible)
	*/
	int read_all_messages();

	/*
	**	DESCRIPTION
	**		Ask the server to delete a post
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of error (no more operation are possible)
	*/
	int delete_message();

#endif