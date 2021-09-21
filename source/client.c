#include "common-header.h"

void sigint_handler(int signum);

int sockfd;

int main(int argc, const char *argv[]){
	char *buffer;
	const char *ip_address;
	int port;

	printf("Client active.\n");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("Error in client on socket attempt.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr;

	switch(argc){
		case 1:
			ip_address = "127.0.0.1";
			port = INITAL_SERV_PORT;
			break;
		case 2:
			ip_address = "127.0.0.1";
			port = strtol(argv[1], NULL, 10);
			break;
		case 3:
			// strcpy(ip_address, argv[1]);
			ip_address = argv[1];
			port = strtol(argv[2], NULL, 10);
			break;
		default:
			fprintf(stderr, "Incorrect number of arguments.\n");
			exit(EXIT_FAILURE);
			break;
	}

	sockaddr_in_setup(&addr, ip_address, port);

	printf("Server address is %s:%d\n",
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("Error in client on connect");
		exit(EXIT_FAILURE);
	}

	printf("Connected.\n");

	while(1){
			printf("Insert message:\n");
			do {
				scanf("%m[^\n]", &buffer);
				fflush(stdin);
			} while (buffer == NULL);

			if(send_message_to(sockfd, "m", buffer) < 0){
				exit(EXIT_FAILURE);
			}

			free(buffer);
		}

	return 0;
}

void sigint_handler(int signum){
	if(close(sockfd) < 0 && errno != EBADF) perror("Error in client on close.\n");
	printf("Client has interrupted connection.\n");
}