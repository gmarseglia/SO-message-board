/*
**	Contains the main cycle of the client
*/

#include "client.h"

/*	
**	DESCRIPTION
**		Close the socket and exit with code 0
*/
void close_connenction_and_exit(int signum);

/* ----------------------------------------------------------------- */

int main(int argc, const char *argv[]){

	const char *ip_address;
	int port;

	printf("Client active.\n");

	#ifdef DEBUG_SERVER		/* Flag used during debugging */
		/* DEBUG_SERVER 127.0.0.1:6990 */
		ip_address = "127.0.0.1";
		port = argc == 1 ? 6990 : strtol(argv[1], NULL, 10);
	
	
	#else	/* Intended use */
		if(argc != 3){
			fprintf(stderr, "Incorrect number of arguments.\nCorrect usage is xclient ip_address port_number\n");
			exit_failure();
		}

		ip_address = argv[1];
		port = strtol(argv[2], NULL, 10);
	
	#endif

	/* Setup the struct for server address */
	sockaddr_in_setup(&addr, ip_address, port);

	printf("Server address is %s:%d\n\n",
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	/* Signal handling */
	sigfillset(&sigset_all_blocked);
	pthread_sigmask(SIG_SETMASK, NULL, &sigset_sigint_allowed);

	/* The handler is very simple, needs only to close socket and exit */
	struct sigaction actual_sigaction = {.sa_handler = close_connenction_and_exit, .sa_mask = sigset_all_blocked};
	sigaction(SIGINT, &actual_sigaction, NULL);

	/* Client needs to register or login before being able to send message */
	while(1){
		/* Create the socket */
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0) perror_and_failure("socket()", __func__);

		printf("Waiting for server connection...\n");

		/* #1: Connect to server */
		if(connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0){
			if(errno != ECONNREFUSED) perror_and_failure("connect()", __func__);
			printf("Could not connect to server. Quitting.\n");
			exit(0);
		} 

		printf("Connected.\n\n");

		/* #2: Login or Registration
			Break cycle if login or registration are successful */
		if(login_registration() == 0) break;

		/* #3a: Close connection
			If login or registration are unsuccessful, then the socket is closed, to be reopened later */
		close(sockfd);
	}

	/* #3: Start Client Main Cycle
		Main cycle,	gets interrupted by SIGINT or errors */
	while(dispatcher() == 0);

	/* Application ends either after receiving SIGINT or
	user select (E)xit or
	one action returns -1 */
	close_connenction_and_exit(0);
}

void close_connenction_and_exit(int signum){
	if(close(sockfd) < 0 && errno != EBADF) perror("Error in client on close.\n");
	printf("Connection closed.\n");
	exit(0);
}

int dispatcher(){
	char cli_op[2];

	/* #1: Ask users what action they want to do */
	printf("\nWhat do you want to do?\n(P)ost, (R)ead, (D)elete, (E)xit\n");
	while(scanf("%1s", cli_op) < 1);
	fflush(stdin);

	switch(cli_op[0]){
		/* #2a: Post a message */
		case ACTION_POST:
			return post_message();

		/* #2b: Read all messages */
		case ACTION_READ:
			return read_all_messages();

		/* #2c: Delete a message */
		case ACTION_DELETE:
			return delete_message();

		/* #2d: Exit*/ 
		case ACTION_EXIT:
			return -1;

		/* If key is not valid, ask again */	
		default:	
			return 0;
	}
}
