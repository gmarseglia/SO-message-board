#include "common-header.h"
#include "server-registration.h"

#define SIMPLE_OK_RESPONSE

// Define the length of pending connection request
#define MAX_BACKLOG 1024

void sigint_handler(int signum);
void *thread_communication_routine(void *arg);
void *thread_close_connection(int id, int sockfd);

// Struct for thread argument
struct thread_arg{
	int id;
	int acceptfd;
	struct sockaddr_in *client_addr;
};

// Has to be less than SEMVMX 
//	Maximum allowable value for semval: implementation dependent (32767).
const int MAX_THREADS = 1024;

// Users file
const char *users_filename = "users.list";

// sockfd is for the socket that accept connection, acceptfd is for the socket that does the communication
int sockfd, acceptfd;

// Semaphores
int UW;	//Users Write 
int UR;	//Users Read

int main(int argc, char const *argv[])
{
	pthread_t tids[MAX_BACKLOG];
	int serv_port = INITIAL_SERV_PORT;
	int tCount = 0;

	printf("Server active.\n");

	// Print the ip address in current network
	system("hostname -I | awk \'{print $1}\'");

	// Create the Users file
	int usersfd = open(users_filename, O_RDWR);
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
	if(semctl(UR, 0, SETVAL, MAX_THREADS) < 0){
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

		// Accept a pending connection, blocking call
		acceptfd = accept(sockfd, (struct sockaddr *)client_addr, &client_addr_len);
		if(acceptfd < 0){
			perror("Error in server on accept attempt.\n");
			exit(EXIT_FAILURE);
		}

		// Create a thread for handling the communication with client
		struct thread_arg *arg = malloc(sizeof(struct thread_arg));
		arg->id = tCount;
		arg->acceptfd = acceptfd;
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

	struct user_info *user_info = malloc(sizeof(struct user_info));
	if(user_info == NULL){
		perror("Error in thread_communication_routine on malloc");
		exit(EXIT_FAILURE);
	}

	char *recipient, op;
	int uid;

	printf("Thread[%d] accepted connection from %s:%d\n", id, str_client_addr, i_client_port);

	// Start the login or registration phase, mandatory for every client
	if(login_registration(acceptfd, user_info) < 0)
		return thread_close_connection(id, acceptfd);

	// Main cycle
	//	gets interrupted when receive_message_from returns 0, meaning that connection was closed by client
	while(receive_message_from(acceptfd, &uid, &op, &recipient) > 0){

		printf("Thread[%d] received %c op from %d@%s:%d:\n%s\n",
			id, op, uid, str_client_addr, i_client_port, recipient);

		free(recipient);

		#ifdef SIMPLE_OK_RESPONSE
		printf("Simple ok response\n");
		if(op == OP_MSG_BODY)
			send_message_to(acceptfd, UID_SERVER, OP_OK, NULL);
		#endif

	}

	return thread_close_connection(id, acceptfd);
}

void *thread_close_connection(int id, int sockfd){
	printf("Thread[%d] has found closed connection.\n", id);
	if(close(acceptfd) < 0 && errno != EBADF) perror("Error in server on close accepted socket\n");
	return NULL;
}