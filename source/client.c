#include "common.h"
#include "client-routines.h"

void close_connenction_and_exit(int signum);
int dispatcher();

int sockfd;
user_info client_ui;
struct sockaddr_in addr;

int main(int argc, const char *argv[]){
	const char *ip_address;
	int port;

	printf("Client active.\n");

	#ifdef DEBUG_SERVER
	// DEBUG_SERVER 127.0.0.1:6990
	ip_address = "127.0.0.1";
	port = argc == 1 ? INITIAL_SERV_PORT : strtol(argv[1], NULL, 10);
	
	// Intended use
	#else
	if(argc != 3){
		fprintf(stderr, "Incorrect number of arguments.\nCorrect usage is xclient ip_address port_number\n");
		exit(EXIT_FAILURE);
	}

	ip_address = argv[1];
	port = strtol(argv[2], NULL, 10);
	
	#endif

	// Setup the struct for server address
	sockaddr_in_setup(&addr, ip_address, port);

	printf("Server address is %s:%d\n",
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	signal(SIGINT, close_connenction_and_exit);

	// Client needs to register or login before being able to send message
	while(login_registration(&sockfd, &addr, &client_ui) < 0);

	// Main cycle
	//	gets interrupted by SIGINT or errors
	while(dispatcher() == 0);

	return 0;
}

int dispatcher(){
		char cli_op;
		// Ask users what cli_op they want to do
		printf("\nWhat do you want to do?\n(P)ost, (E)xit\n");
		scanf("%c", &cli_op);
		fflush(stdin);

		switch(cli_op){
			case CLI_OP_POST:
				return post(sockfd, client_ui);
			case CLI_OP_EXIT:
				close_connenction_and_exit(0);
			default:
				return 0;
		}
	}

/*
*	Close the socket
*/
void close_connenction_and_exit(int signum){
	if(close(sockfd) < 0 && errno != EBADF) perror("Error in client on close.\n");
	printf("Client has interrupted connection.\n");
	exit(0);
}