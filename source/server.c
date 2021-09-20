#include "common-header.h"

// #define PRINT_DEBUG
#define MAX_BACKLOG 1024

void sigint_handler(int signum);
void *thread_communication_routine(void *arg);

struct thread_arg{
	int id;
	int acceptfd;
	struct sockaddr_in *client_addr;
};

int sockfd, acceptfd;
int serv_port = SERV_PORT;

int main(int argc, char const *argv[])
{
	pthread_t tids[MAX_BACKLOG];
	int tCount = 0;

	printf("Server active.\n");
	system("hostname -I | awk \'{print $1}\'");

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){							// Creo la socket che usa TCP
		perror("Error in server on socket creation attempt.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serv_port);												// Uso come porta SERV_PORT in common-header.h = 6990
	addr.sin_addr.s_addr = INADDR_ANY;												// Accetto connessioni da tutti 

	#ifdef PRINT_DEBUG
	printf("Server trying to bound on %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	#endif

	while(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		if(errno != EADDRINUSE){
			perror("Error in server on bind attempt.\n");
			exit(EXIT_FAILURE);
		} else {
			serv_port++;
			addr.sin_port = htons(serv_port);
		}
	}

	printf("Server bound on %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	if(listen(sockfd, MAX_BACKLOG) < 0){											// Mi metto in listen
		perror("Error in server on listen attempt.\n");
		exit(EXIT_FAILURE);
	}

	#ifdef PRINT_DEBUG
	printf("Server is on LISTEN\n");
	#endif

	signal(SIGINT, sigint_handler);

	socklen_t client_addr_len;
	while(1){
		struct sockaddr_in *client_addr = malloc(sizeof(struct sockaddr_in));
		memset(client_addr, '\0', sizeof(struct sockaddr_in));
		if(client_addr == NULL){
			perror("Error in server on main while malloc\n");
			exit(EXIT_FAILURE);
		}
		client_addr_len = sizeof(struct sockaddr_in);

		acceptfd = accept(sockfd, (struct sockaddr *)client_addr, &client_addr_len);
		if(acceptfd < 0){
			perror("Error in server on accept attempt.\n");
			exit(EXIT_FAILURE);
		}

		struct thread_arg *arg = malloc(sizeof(struct thread_arg));
		arg->id = tCount;
		arg->acceptfd = acceptfd;
		arg->client_addr = client_addr;
		pthread_create(&tids[tCount], NULL, thread_communication_routine, (void *)arg);
		tCount = (tCount + 1) % MAX_BACKLOG;

	}

	return 0;
}

void sigint_handler(int signum){
	if(close(sockfd) < 0) perror("Error in sigint_handler on close\n");
	exit(1);
}

void *thread_communication_routine(void *arg){
	struct thread_arg *t_arg = (struct thread_arg *)arg;
	int id = t_arg->id;
	int acceptfd = t_arg->acceptfd;
	struct sockaddr_in *client_addr = (struct sockaddr_in *)t_arg->client_addr;

	char *str_client_addr = inet_ntoa(client_addr->sin_addr);
	int i_client_port = ntohs(client_addr->sin_port);

	free(client_addr);
	free(arg);

	char *buffer;
	int byteRead, byteWrite;

	printf("Thread[%d] accepted connection from %s:%d\n", id, str_client_addr, i_client_port);

	while(1){

		byteWrite = 0;
		byteRead = recv(acceptfd, &byteWrite, sizeof(int), 0);

		if(byteRead == 0) break;

		#ifdef PRINT_DEBUG
		printf("byteWrite=%d\n", byteWrite);
		#endif

		buffer = malloc(sizeof(char) * (byteWrite + 1));
		memset(buffer, '\0', byteWrite + 1);

		recv(acceptfd, buffer, byteWrite, 0);

		if(byteRead == 0) break;

		printf("Thread[%d] from %s:%d:\n%s\n", id, str_client_addr, i_client_port, buffer);

		free(buffer);

	}

	printf("Thread[%d] has found closed connection.\n", id);
	if(close(acceptfd) < 0 && errno != EBADF) perror("Error in server on close accepted socket\n");
	return NULL;

}