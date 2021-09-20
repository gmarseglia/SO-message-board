#include "common-header.h"

#define PRINT_DEBUG

int main(int argc, const char *argv[]){
	char *buffer;
	char *serv_addr;
	printf("Client active.\n");

	if(argc == 1){
		serv_addr = "127.0.0.1";
	} else {
		serv_addr = malloc(sizeof(argv[1]));
		strcpy(serv_addr, argv[1]);
	}

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("Error in client on socket attempt.\n");
		exit(EXIT_FAILURE);
	}

	printf("Insert string:\n");
	scanf("%ms", &buffer);

	#ifdef PRINT_DEBUG
	printf("String Inserted %s.\n", buffer);
	#endif

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERV_PORT);
	addr.sin_addr.s_addr = inet_addr(serv_addr);

	#ifdef PRINT_DEBUG
	printf("Client trying to connect to %s:%d\n", inet_ntoa(addr.sin_addr), SERV_PORT);
	#endif

	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("Error in client on connect");
		exit(EXIT_FAILURE);
	}

	printf("Connection established.\n");

	write(sockfd, buffer, sizeof(buffer));

	free(buffer);

	if(close(sockfd) < 0 && errno != EBADF) perror("Error in client on close.\n");

	return 0;
}