/*
**	Contains the main cycle of the client
*/

#include "client.h"

/*	
**	DESCRIPTION
**		Close the socket and exit with code 0
*/
void close_connenction_and_exit(int signum);

void display_help();

/* ----------------------------------------------------------------- */

int main(int argc, const char *argv[]){
	int port;

	/* Setup server address and port */

	/* Flag used during debugging
		DEBUG_SERVER 127.0.0.1:6990 */
	#ifdef DEBUG_SERVER

		if(argc == 1)
			port = 6990;
		else
		{
			/* Read port number from argv[1] */
			if(sscanf(argv[1], "%d", &port) < 0 || port < 0 || port > 65535)
			{
				printf("Incorrect port number.\n");
				exit(EXIT_SUCCESS);
			}
		}

		sockaddr_in_setup(&addr, "127.0.0.1", port);

	/* Intended use */
	#else
		unsigned long ip_address;

		/* xclient -h || xclient --help -> display help and exit */
		if(argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
		{
			display_help();
			exit(EXIT_SUCCESS);
		}

		/* xclient server_address server_port -> start server with user defined server address and port */
		else if(argc == 3 && inet_pton(AF_INET, argv[1], (void *) &ip_address) > 0 &&  sscanf(argv[2], "%d", &port) > 0)
		{
			/* If server_port is outside range, then warn and exit */
			if(port < 0 || port > 65535)
			{
				printf("Server_port : <%d> is outside permitted range.\n"
					"Choose a port in range [0, 65535]\n\n", port);
				exit(EXIT_SUCCESS);
			}

		}

		/* xclient unknown_parameters -> warn, display help and exit */
		else {
			printf("Incorrect usage.\n");
			display_help();
			exit(EXIT_SUCCESS);
		}
	
		/* Setup the struct for server address */
		sockaddr_in_setup_inaddr(&addr, ip_address, port);

	#endif

	printf("Server address : <%s:%d>\n\n",
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	/* Signal handling */
	sigemptyset(&set_sigint);
	sigaddset(&set_sigint, SIGINT);

	/* The handler is very simple, needs only to close socket and exit */
	struct sigaction actual_sigaction = {.sa_handler = close_connenction_and_exit, .sa_mask = set_sigint};
	sigaction(SIGINT, &actual_sigaction, NULL);

	/* Client needs to register or login before being able to send message */
	while(1){
		/* Create the socket */
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0) perror_and_failure("socket()", __func__);

		printf("Waiting for server connection...\n");

		/* #1: Connect to server */
		if(connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0){
			if(errno != ECONNREFUSED && errno != ETIMEDOUT)
				perror_and_failure("connect()", __func__);
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


void display_help(){
	printf("Usage: xclient server_address server_port\n"
		"Start client.\n\n"
		"Options:\n"
		"\t-h and --help: -h and --help: display this help and exit.\n\n");
}