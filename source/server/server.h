/*
**	Common header for server .c files
*/

#ifndef SERVER_ROUTINES_H_INCLUDED /* Double-inclusion guard */
	#define SERVER_ROUTINES_H_INCLUDED

	/* Include external headers */
	#include <ifaddrs.h>
	#include "../common/common.h"
	#include "../common/pthread-bitmask.h"

	/* Files */
	#define USERS_FILENAME "users.list"
	#define MESSAGES_FILENAME "messages.list"
	#define INDEX_FILENAME "index.list"
	#define FREE_AREAS_FILENAME "free-areas.list"

	/* Macros */
	#define FDOPEN_USERS() fdopen(open(USERS_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	#define FDOPEN_MESSAGES() fdopen(open(MESSAGES_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	#define FDOPEN_INDEX() fdopen(open(INDEX_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	#define FDOPEN_FREE_AREAS() fdopen(open(FREE_AREAS_FILENAME, O_CREAT | O_RDWR, 0660), "r+");

	/* Operational flags */
	#define SKIP_FREE_AREAS

	/* Constant */
	#define INITIAL_SERV_PORT (6990)
	#define INITIAL_UID (1000)
	#define MAX_BACKLOG (1024)
	#define MAX_THREAD (1024)		/* HAS TO BE LESS THAN 32767, "man semctl" */
	#define INDEX_LINE_LEN (16) // 8 long offset + 4 int message_len + 4 int UID + 1 char '\n'
	#define FREE_AREAS_LINE_LEN (12) // 8 uint64_t message_offset + 4 uint32_t message_len

	/* Thread argument */
	struct thread_arg{
		int id;
		int acceptfd;	/* Socket connected to client */
		struct sockaddr_in *client_addr;	/* Address of the client */
	};

	/* Semaphores */
	int UW;	/* Users file write permission */ 
	int UR;	/* Users file read permission */
 	int MW;	/* Messages file write permission */
	int MR; /* Messages file read permission */
	int sem_free_threads;	/* Free threads counter */

	/* Bitmasks */
	bitmask_t bm_free_threads;	/* 1 -> thread is free, 0 -> thread is busy */

	/* Signal set */
	sigset_t set_sigint, set_sigusr1, set_both;

	/*
	**	DESCRIPTION
	**		Ask user R or L, for Registration or Login
	**		Register or Log user
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of unsuccess
	**		In case of error: exit_failure()
	*/
	int login_registration();

	/*
	**	DESCRIPTION
	**		Dispatch the different request incoming
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of closed connection
	*/ 
	int dispatcher();

	/*
	**	DESCRIPTION
	**		Save the message received
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of unsuccess
	**		In case of error: exit_failure()
	*/
	int post_message();

	/*
	**	DESCRIPTION
	**		Send to Client all saved messages
	**
	**	RETURN VALUE
	**		0 in case of success
	**		-1 in case of unsuccess
	**		In case of error: exit_failure()
	*/
	int read_all_messages();

	/*
	**	DESCRIPTION
	**		Try to delete MID post
	**		Successul if user asking to delete is the same user that posted
	**
	**	RETURN VALUE
	**		0 in case of no error
	**		-1 in case of error
	*/
	int delete_message();

	/*
	**	DESCRIPTION
	**		Look for user by username and if found returs userinfo
	**		If not found returns NULL
	*/
	user_info_t *find_user_by_username(char *username);

	/*
	**	DESCRIPTION
	**		Look for user by uid and if found returs userinfo
	**		If not found returns NULL
	*/
	user_info_t *find_user_by_uid(int uid);

	/*
	**	DESCRIPTION
	**		Routine of the threads assigned to communication with clieny
	*/
	void *thread_communication_routine(void *arg);

	/*
	**	DESCRIPTION
	**		Handler of signals, used both by main thread and communication thread
	*/
	void signal_handler(int signum);

	/*
	**	DESCRIPTION
	**		Function used by communication thread that closes the socket
	*/
	void *thread_close_connection();

#endif
