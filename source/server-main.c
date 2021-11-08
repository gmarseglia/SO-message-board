#include "server.h"

// sockfd is for the socket that accept connection, acceptfd is for the socket that does the communication
int sockfd;

/*
	Close the connection socket
*/
void sigint_handler(int signum);

/*
	Server Main
*/
int main(int argc, char const *argv[]){
	pthread_t tids[MAX_BACKLOG];
	int serv_port = INITIAL_SERV_PORT;

	printf("Server active.\n");

	// Print the ip address in current network
	system("hostname -I | awk \'{print $1}\'");

	// Initialize Users file semaphores
	int *SEMAPHORES[] = {&UR, &UW, &MR, &MW, &sem_free_threads, NULL};
	int SEMAPHORES_INIT_VALUE[] = {MAX_THREAD, 1, MAX_THREAD, 1, MAX_THREAD};

	for(int i = 0; SEMAPHORES[i] != NULL; i++){
		*SEMAPHORES[i] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0660);
		if(*SEMAPHORES[i] == -1) perror_and_failure("on semget", __func__);
		if(semctl(*SEMAPHORES[i], 0, SETVAL, SEMAPHORES_INIT_VALUE[i]) < 0) perror_and_failure("on semctl init", __func__);
	}

	// Initialize bitmask
	if(bitmask_init(&bm_free_threads, MAX_THREAD) < 0)	perror_and_failure("on bitmask_init()", __func__);
	bitmask_fill(&bm_free_threads);	/* Set all bit to 1, all threads are free */

	// Create the socket for connection, use TCP
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_and_failure("on socket creation attempt", __func__);

	// Setup the struct that describes from which address accept connections
	struct sockaddr_in addr;
	sockaddr_in_setup_inaddr(&addr, INADDR_ANY, serv_port);	// INADDR_ANY = 0.0.0.0, all addresses

	#ifdef PRINT_DEBUG
	printf("Server trying to bound on %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	#endif

	// Try to bind starting from INITIAL_SERV_PORT = 6990, and on error "address already in use" try next port
	while(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		if(errno != EADDRINUSE) perror_and_failure("on bind attempt", __func__);
		else {
			serv_port++;
			addr.sin_port = htons(serv_port);
		}
	}

	printf("Server bound on %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	// Set the TCP connection socket on LISTEN
	if(listen(sockfd, MAX_BACKLOG) < 0) perror_and_failure("listen()", __func__);

	#ifdef PRINT_DEBUG
	printf("Server is on LISTEN\n");
	#endif

	// Set the SIGINT handler
	signal(SIGINT, sigint_handler);

	// Main cycle
	socklen_t client_addr_len;
	int free_thread;	/* It's safe to use int, MAX_THREAD upper bound is 32767 */

	while(1){
		// Initialize the struct to contain the address of the client once connected
		struct sockaddr_in *client_addr = malloc(sizeof(struct sockaddr_in));
		if(client_addr == NULL) perror_and_failure("client_addr malloc()", __func__);
		memset(client_addr, '\0', sizeof(struct sockaddr_in));
		client_addr_len = sizeof(struct sockaddr_in);

		struct thread_arg *arg = malloc(sizeof(struct thread_arg));
		if(arg == NULL) perror_and_failure("thread_arg malloc()", __func__);

		// Wait for a free thread, BLOCKING
		short_semop(sem_free_threads, -1);

		// Find first free thread
		if((free_thread = bitmask_find_first_set(&bm_free_threads)) < 0)
			perror_and_failure("No free thread found in bitmask_find...()", __func__);

		// Accept a pending connection, blocking call
		arg->acceptfd = accept(sockfd, (struct sockaddr *)client_addr, &client_addr_len);
		if(arg->acceptfd < 0) perror_and_failure("accept", __func__);

		// Set thread as busy
		bitmask_del(&bm_free_threads, free_thread);

		// Create a thread for handling the communication with client
		arg->id = free_thread;
		arg->client_addr = client_addr;
		pthread_create(&tids[free_thread], NULL, thread_communication_routine, (void *)arg);
	}

	return 0;
}

void sigint_handler(int signum){
	// Close all semaphore
	semctl(UR, 0, IPC_RMID);
	semctl(UW, 0, IPC_RMID);
	semctl(MR, 0, IPC_RMID);
	semctl(MW, 0, IPC_RMID);
	semctl(sem_free_threads, 0, IPC_RMID);
	if(close(sockfd) < 0) perror("Error in sigint_handler on close\n");
	exit(1);
}

// -------------------------------------------------
// server.h operations
user_info *find_user_by_username(char *username){
	// Open Users file
	FILE *users_file = FDOPEN_USERS();
	if(users_file == NULL) perror_and_failure("FDOPEN()", __func__);

	user_info *read_ui = malloc(sizeof(user_info));
	if(read_ui == NULL) perror_and_failure ("read_ui malloc()", __func__);

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
	FILE *users_file = FDOPEN_USERS();
	if(users_file == NULL) perror_and_failure("FDOPEN_USERS()", __func__);

	user_info *read_ui = malloc(sizeof(user_info));
	if(read_ui == NULL) perror_and_failure("read_ui malloc()", __func__);

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