#include "common-header.h"
#include "client-registration.h"

void sigint_handler(int signum);

int sockfd;
struct user_info user_info;

int main(int argc, const char *argv[]){
	char *buffer;
	const char *ip_address;
	int port;

	printf("Client active.\n");

	struct sockaddr_in addr;

	// DEBUG_SERVER 127.0.0.1:6990
	#ifdef DEBUG_SERVER
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

	// Client needs to register or login before being able to send message
	login_registration(&sockfd, &addr, &user_info);

	// Main cycle
	//	gets interrupted by SIGINT or errors
	while(1){
			// Scanf an entire line with memory modifier m, continue only if characters were typed
			printf("Insert message:\n");
			do {
				scanf("%m[^\n]", &buffer);
				fflush(stdin);
			} while (buffer == NULL);

			// Send message to server
			if(send_message_to(sockfd, user_info.uid, OP_MESSAGE, buffer) < 0){
				exit(EXIT_FAILURE);
			}

			// Free the buffer allocated by scanf
			free(buffer);
		}

	return 0;
}

/*
*	Close the socket
*/
void sigint_handler(int signum){
	if(close(sockfd) < 0 && errno != EBADF) perror("Error in client on close.\n");
	printf("Client has interrupted connection.\n");
}