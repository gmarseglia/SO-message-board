#include "common-header.h"

// #define PRINT_DEBUG

void sigint_handler(int signum);

int sockfd;

int main(int argc, const char *argv[]){
	char *buffer;
	char *serv_addr;
	int serv_port;

	printf("Client active.\n");

	serv_addr = "127.0.0.1";
	serv_port = SERV_PORT; 
	if(argc > 1){
		serv_port = strtol(argv[1], NULL, 10);
	}
	if(argc > 2) {
		serv_addr = malloc(sizeof(argv[2]));
		strcpy(serv_addr, argv[2]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("Error in client on socket attempt.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serv_port);
	addr.sin_addr.s_addr = inet_addr(serv_addr);

	#ifdef PRINT_DEBUG
	printf("Client trying to connect to %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	#endif

	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("Error in client on connect");
		exit(EXIT_FAILURE);
	}

	printf("Connections to %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	while(1){
			printf("Insert string:\n");
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