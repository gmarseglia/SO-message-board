#include "common.h"
#include "server-routines.h"

void sigint_handler(int signum);
void *thread_communication_routine(void *arg);
void *thread_close_connection(int id, int sockfd);
int dispatcher();

// Struct for thread argument
struct thread_arg{
	int id;
	int acceptfd;
	struct sockaddr_in *client_addr;
};

// sockfd is for the socket that accept connection, acceptfd is for the socket that does the communication
int sockfd;

int main(int argc, char const *argv[])
{
	pthread_t tids[MAX_BACKLOG];
	int serv_port = INITIAL_SERV_PORT;
	int tCount = 0;

	printf("Server active.\n");

	// Print the ip address in current network
	system("hostname -I | awk \'{print $1}\'");

	// Create the Users file
	int usersfd = open(USERS_FILENAME, O_RDWR);
	if(usersfd < 0){
		perror("Error in server on open users file");
		exit(EXIT_FAILURE);
	}
	if(close(usersfd) < 0) perror("Error in server on close users file");

	// Initialize Users file semaphores
	UR = semget(IPC_PRIVATE, 1, IPC_CREAT | 0660);
	UW = semget(IPC_PRIVATE, 1, IPC_CREAT | 0660);
	if(UR == -1 || UW == -1){
		perror("Error in server on semget\n");
		exit(EXIT_FAILURE);
	}
	if(semctl(UR, 0, SETVAL, MAX_THREAD) < 0){
		perror("Error in server on semctl(UR)");
		exit(EXIT_FAILURE);
	}
	if(semctl(UW, 0, SETVAL, 1) < 0){
		perror("Error in server on semctl(UW)");
		exit(EXIT_FAILURE);
	}

	// Create the socket for connection, use TCP
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Error in server on socket creation attempt.\n");
		exit(EXIT_FAILURE);
	}

	// Setup the struct that describes from which address accept connections
	struct sockaddr_in addr;
	sockaddr_in_setup_inaddr(&addr, INADDR_ANY, serv_port);	// INADDR_ANY = 0.0.0.0, all addresses

	#ifdef PRINT_DEBUG
	printf("Server trying to bound on %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	#endif

	// Try to bind starting from INITIAL_SERV_PORT = 6990, and on error "address already in use" try next port
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

	// Set the TCP connection socket on LISTEN
	if(listen(sockfd, MAX_BACKLOG) < 0){
		perror("Error in server on listen attempt.\n");
		exit(EXIT_FAILURE);
	}

	#ifdef PRINT_DEBUG
	printf("Server is on LISTEN\n");
	#endif

	// Set the SIGINT handler
	signal(SIGINT, sigint_handler);

	socklen_t client_addr_len;
	// Main cycle
	while(1){
		// Initialize the struct to contain the address of the client once connected
		struct sockaddr_in *client_addr = malloc(sizeof(struct sockaddr_in));
		memset(client_addr, '\0', sizeof(struct sockaddr_in));
		if(client_addr == NULL){
			perror("Error in server on main while malloc\n");
			exit(EXIT_FAILURE);
		}
		client_addr_len = sizeof(struct sockaddr_in);

		struct thread_arg *arg = malloc(sizeof(struct thread_arg));
		if(arg == NULL){
			fprintf(stderr, "Error in server main on malloc\n");
			exit_failure();
		}

		// Accept a pending connection, blocking call
		arg->acceptfd = accept(sockfd, (struct sockaddr *)client_addr, &client_addr_len);
		if(arg->acceptfd < 0){
			perror("Error in server on accept attempt.\n");
			exit_failure();
		}

		// Create a thread for handling the communication with client
		arg->id = tCount;
		arg->client_addr = client_addr;
		pthread_create(&tids[tCount], NULL, thread_communication_routine, (void *)arg);

		// #TODO: limit thread based on MAX_BACKLOG
		tCount = (tCount + 1) % MAX_BACKLOG;
	}

	return 0;
}

/*
*	Close the connection socket
*/
void sigint_handler(int signum){
	if(close(sockfd) < 0) perror("Error in sigint_handler on close\n");
	exit(1);
}

/*
*	Handles the communication with client
*/
void *thread_communication_routine(void *arg){
	// Initialize local variables and free argument struct
	struct thread_arg *t_arg = (struct thread_arg *)arg;
	int id = t_arg->id;
	int acceptfd = t_arg->acceptfd;
	struct sockaddr_in *client_addr = (struct sockaddr_in *)t_arg->client_addr;

	char *str_client_addr = inet_ntoa(client_addr->sin_addr);
	int i_client_port = ntohs(client_addr->sin_port);

	free(client_addr);
	free(arg);

	user_info client_ui;

	printf("Thread[%d]: accepted connection from %s:%d\n", id, str_client_addr, i_client_port);

	// Start the login or registration phase, mandatory for every client
	if(login_registration(acceptfd, &client_ui) < 0)
		return thread_close_connection(id, acceptfd);

	printf("Thread[%d]: (%s, %d) authenticated from %s:%d\n",
		id, client_ui.username, client_ui.uid, str_client_addr, i_client_port);

	// Main cycle
	//	gets interrupted when receive_message_from returns 0, meaning that connection was closed by client
	while(dispatcher(acceptfd, client_ui) > 0);

	return thread_close_connection(id, acceptfd);
}

/*
	RETURNS:
		1 in case of success
		0 in case of closed connection
		exit_failure() in case of errors
*/ 
int dispatcher(int acceptfd, user_info client_ui){
	operation op;
	int byte_read;

	// Receive first operation
	byte_read = receive_operation_from(acceptfd, &op);
	if(byte_read == 0) return 0;
	if(byte_read < 0) exit_failure();

	#ifdef PRINT_DEBUG
	printf("BEGIN%s\n(%s, %d) sent \'%c\' op:\n%s\n%sEND\n\n",
	SEP, client_ui.username, client_ui.uid, op.code, op.text, SEP);
	#endif
	
	switch(op.code){
		case OP_MSG_BODY:
			printf("Simple ok response\n");	
			send_message_to(acceptfd, UID_SERVER, OP_OK, NULL);
			free(op.text);
			return 1;
		default:
			return 1;			
	}
}


void *thread_close_connection(int id, int sockfd){
	printf("Thread[%d]: found closed connection.\n", id);
	if(close(sockfd) < 0 && errno != EBADF) perror("Error in server on close accepted socket\n");
	return NULL;
}