#include "server.h"

void sigint_handler(int signum);
void *thread_communication_routine(void *arg);
void *thread_close_connection(int id, int sockfd);

// Struct for thread argument
struct thread_arg{
	int id;
	int acceptfd;
	struct sockaddr_in *client_addr;
};

// sockfd is for the socket that accept connection, acceptfd is for the socket that does the communication
int sockfd;

/*
	Server Main
*/
int main(int argc, char const *argv[]){
	pthread_t tids[MAX_BACKLOG];
	int serv_port = INITIAL_SERV_PORT;
	int tCount = 0;

	printf("Server active.\n");

	// Print the ip address in current network
	system("hostname -I | awk \'{print $1}\'");

	// Initialize Users file semaphores
	int *SEMAPHORES[4] = {&UR, &UW, &MR, &MW};
	int SEMAPHORES_INIT_VALUE[4] = {MAX_THREAD, 1, MAX_THREAD, 1};

	for(int i = 0; i < 4; i++){
		*SEMAPHORES[i] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0660);
		if(*SEMAPHORES[i] == -1){
			perror("Error in server on semget\n");
			exit_failure();
		}
		if(semctl(*SEMAPHORES[i], 0, SETVAL, SEMAPHORES_INIT_VALUE[i]) < 0){
			fprintf(stderr, "Semaphore %d\n", i);
			perror("Error in server on semctl");
			exit_failure();
		}
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
		if(client_addr == NULL){
			perror("Error in server on main while malloc\n");
			exit(EXIT_FAILURE);
		}
		memset(client_addr, '\0', sizeof(struct sockaddr_in));
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
	Close the connection socket
*/
void sigint_handler(int signum){
	semctl(UR, 0, IPC_RMID);
	semctl(UW, 0, IPC_RMID);
	semctl(MR, 0, IPC_RMID);
	semctl(MW, 0, IPC_RMID);
	if(close(sockfd) < 0) perror("Error in sigint_handler on close\n");
	exit(1);
}

/*
	Handles the communication with client
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
	while(dispatcher(acceptfd, client_ui) == 0);

	return thread_close_connection(id, acceptfd);
}

/*
	Close communication with client
*/
void *thread_close_connection(int id, int sockfd){
	printf("Thread[%d]: closed connection.\n", id);
	if(close(sockfd) < 0 && errno != EBADF) perror("Error in server on close accepted socket\n");
	return NULL;
}

// -------------------------------------------------
// server.h operations
int dispatcher(int acceptfd, user_info client_ui){
	operation op;

	// Receive first operation
	if(receive_operation_from(acceptfd, &op) < 0)
		return -1;

	#ifdef PRINT_DEBUG_FINE
	printf("BEGIN%s\n(%s, %d) sent \'%c\' op:\n%s\n%sEND\n\n",
	SEP, client_ui.username, client_ui.uid, op.code, op.text, SEP);
	#endif
	
	switch(op.code){
		case OP_MSG_SUBJECT:
			return post(acceptfd, client_ui, &op);
		case OP_READ_REQUEST:
			return read_all(acceptfd, client_ui, &op);
		case OP_DELETE_REQUEST:
			send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Delete not possible.");
			return 0;
		default:
			printf("BEGIN%s\n(%s, %d) sent \'%c\' op:\n%s\n%sEND\n\n",
			SEP, client_ui.username, client_ui.uid, op.code, op.text, SEP);
			return send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect OP");			
	}
}

user_info *find_user_by_username(char *username){
	// Open Users file
	FILE *users_file = fdopen(open(USERS_FILENAME, O_CREAT|O_RDWR, 0660), "r+");
	if(users_file == NULL){
		perror("Error in thread_communication_routine on fdopen");
		exit(EXIT_FAILURE);
	}

	user_info *read_ui = malloc(sizeof(user_info));
	if(read_ui == NULL){
		fprintf(stderr, "Error in find_user on malloc\n");
		exit_failure();
	}

	while(fscanf(users_file, "%d %ms %ms", &(read_ui->uid), &(read_ui->username), &(read_ui->passwd)) != EOF){
		if(strcmp(username ,read_ui->username) == 0){
			fclose(users_file);
			return read_ui;
		}
	}

	free(read_ui);
	fclose(users_file);
	return NULL;
}

user_info *find_user_by_uid(int uid){
	// Open Users file
	FILE *users_file = fdopen(open(USERS_FILENAME, O_CREAT|O_RDWR, 0660), "r+");
	if(users_file == NULL){
		perror("Error in thread_communication_routine on fdopen");
		exit_failure();
	}

	user_info *read_ui = malloc(sizeof(user_info));
	if(read_ui == NULL){
		fprintf(stderr, "Error in find_user on malloc\n");
		exit_failure();
	}

	while(fscanf(users_file, "%d %ms %ms", &(read_ui->uid), &(read_ui->username), &(read_ui->passwd)) != EOF){
		if(read_ui->uid == uid){
			fclose(users_file);
			return read_ui;
		}
	}

	free(read_ui);
	fclose(users_file);
	return NULL;
}
// -------------------------------------------------