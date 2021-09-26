#include "common-header.h"
#include "client-registration.h"

#define CLI_OP_POST 'P'
#define CLI_OP_EXIT 'E'

#define WAIT_SERVER_OK

void close_connenction_and_exit(int signum);
int main_cycle();
int post();

int sockfd;
struct user_info user_info;
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
	while(login_registration(&sockfd, &addr, &user_info) < 0);

	// Main cycle
	//	gets interrupted by SIGINT or errors
	while(main_cycle() == 0);

	return 0;
}

int main_cycle(){
		char cli_op;
		// Ask users what cli_op they want to do
		printf("What do you want to do?\n(P)ost, (E)xit\n");
		scanf("%c", &cli_op);
		fflush(stdin);

		switch(cli_op){
			case CLI_OP_POST:
				return post();
			case CLI_OP_EXIT:
				close_connenction_and_exit(0);
			default:
				return 0;
		}
	}

int post(){
	char *subject, *body = NULL, *tmp_body;
	int body_len = 0;

	// #1: Ask to user the Subject
	printf("Insert the Subject of the message:\n");
	do {
		scanf("%m[^\n]", &subject);
		fflush(stdin);
	} while (subject == NULL);

	// #2: Ask to user the Body
	printf("Insert the Body of the message, double new line to end:\n");
	while(1){
		scanf("%m[^\n]", &tmp_body);
		fflush(stdin);
		if(tmp_body == NULL)
			break;
		body_len = body_len + strlen(tmp_body) + 1;
		body = reallocarray(body, sizeof(char), body_len + 1);
		body[body_len] = '\0';
		strcat(body, tmp_body);
		strcat(body, "\n");
		free(tmp_body);
	}

	#ifdef PRINT_DEBUG_FINE
	printf("-----------------\nSubject:\n%s\nBody:\n%s\n-----------------\n", subject, body);
	#endif

	// #3: Send (UID, OP_MSG_SUBJECT, Subject)
	if(send_message_to(sockfd, user_info.uid, OP_MSG_SUBJECT, subject) < 0)
		return -1;

	// #4: Send (UID, OP_MSG_BODY, Body)
	if(send_message_to(sockfd, user_info.uid, OP_MSG_BODY, body) < 0)
		return -1;

	// #5: Receive (UID_SERVER, OP_OK, ID of the message)
	#ifdef WAIT_SERVER_OK
	int read_uid;
	char read_op, *recipient;
	if(receive_message_from(sockfd, &read_uid, &read_op, &recipient) < 0 || read_uid != UID_SERVER){
		fprintf(stderr, "Error in Post on receive_message_from\n");
		exit(EXIT_FAILURE);
	}
	if(read_op != OP_OK){
		fprintf(stderr, "Post refused: %s\n", recipient);
	}
	#endif

	return 0;
}

/*
*	Close the socket
*/
void close_connenction_and_exit(int signum){
	if(close(sockfd) < 0 && errno != EBADF) perror("Error in client on close.\n");
	printf("Client has interrupted connection.\n");
	exit(0);
}