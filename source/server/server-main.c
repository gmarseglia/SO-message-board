/*
**	Behavoiur of the main thread of the server
*/

#include "server.h"

// sockfd is for the socket that accept connection, acceptfd is for the socket that does the communication
int sockfd;
pthread_t tids[MAX_THREAD];

/*
	Close the connection socket
*/
void sigint_handler(int signum);

void display_help();

void main_cycle();

/*
	Server Main
*/
int main(int argc, char const *argv[]){
	int serv_port;
	struct ifaddrs *ifaddr, *ifa;
    struct sockaddr_in *addr_in;
    char *interface_name, *interface_address;

	/* xserver -> start server with default initial port */
	if(argc == 1){
		serv_port = INITIAL_SERV_PORT;
	} 
	else if (argc == 2){
		/* xserver initial_port -> start server with user defined initial port */
		if(sscanf(argv[1], "%d", &serv_port) > 0){

			/* If initial_port is outside range, then warn and exit */
			if(serv_port < 1024 || serv_port > 65535){
				printf("initial_port : <%d> is outside permitted range.\n"
					"Choose a port in range [1024, 65535]\n\n", serv_port);
				exit(0);
			}
		}

		/* xserver -h || xserver --help -> display help and exit */
		else if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
			display_help();
			exit(0);
		}

		/* xserver unknown_parameter -> warn incorrect usage, display help and exit */
		else {
			printf("Incorrect usage.\n");
			display_help();
			exit(0);
		}

	}

	printf("Server Parameters:\n"
		"\t      Initial port : <%d>\n"
		"\tMax active threads : <%d>\n"
		"\tConnection backlog : <%d>\n\n",
		serv_port,
		MAX_THREAD,
		MAX_BACKLOG);

	/* Print possible address */
    /* Call getifaddrs() and check errors */
    if (getifaddrs(&ifaddr) == -1) 
        perror_and_failure("getifaddrs()", __func__);

    printf("Possible Addresses:\n");

    /* Run through results */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

    	/* Continue if NULL address */
        if (ifa->ifa_addr == NULL)
            continue;  

        /* Read only interface with AF_INET family */
        if(ifa->ifa_addr->sa_family == AF_INET){
        	
        	/* Read interface name and address from struct */
        	addr_in = (struct sockaddr_in *)ifa->ifa_addr;
        	interface_name = ifa->ifa_name;
        	interface_address = inet_ntoa(addr_in->sin_addr);

        	printf("\tInterface : <%s>\n"
        		   "\t  Address : <%s>\n\n",
        		   interface_name, interface_address);
        }
        
    }

    freeifaddrs(ifaddr);

	/* Initialize Users file semaphores */
	int *SEMAPHORES[] = {&UR, &UW, &MR, &MW, &sem_free_threads, NULL};
	int SEMAPHORES_INIT_VALUE[] = {MAX_THREAD, 1, MAX_THREAD, 1, MAX_THREAD};

	for(int i = 0; SEMAPHORES[i] != NULL; i++){
		*SEMAPHORES[i] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0660);
		if(*SEMAPHORES[i] == -1) perror_and_failure("on semget", __func__);
		if(semctl(*SEMAPHORES[i], 0, SETVAL, SEMAPHORES_INIT_VALUE[i]) < 0) perror_and_failure("on semctl init", __func__);
	}

	/* Initialize bitmask */
	if(bitmask_init(&bm_free_threads, MAX_THREAD) < 0)	perror_and_failure("on bitmask_init()", __func__);
	bitmask_fill(&bm_free_threads);	/* Set all bit to 1, all threads are free */

	/* Initialize signal sets */
	sigemptyset(&set_sigint);
	sigaddset(&set_sigint, SIGINT);
	
	sigemptyset(&set_sigusr1);
	sigaddset(&set_sigusr1, SIGUSR1);

	sigemptyset(&set_both);
	sigaddset(&set_both, SIGINT);
	sigaddset(&set_both, SIGUSR1);

	/* Set signal handler */
	struct sigaction actual_sigaction = {.sa_handler = signal_handler, .sa_mask = set_both};
	sigaction(SIGINT, &actual_sigaction, NULL);
	sigaction(SIGUSR1, &actual_sigaction, NULL);

	/* Block signals */
	pthread_sigmask(SIG_BLOCK, &set_both, NULL);

	/* Create the socket for connection, use TCP	*/
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_and_failure("on socket creation attempt", __func__);

	/* Setup the struct that describes from which address accept connections */
	struct sockaddr_in addr;
	sockaddr_in_setup_inaddr(&addr, INADDR_ANY, serv_port);	// INADDR_ANY = 0.0.0.0, all addresses

	#ifdef PRINT_DEBUG
		printf("Server trying to bound on port : <%d>\n", ntohs(addr.sin_port));
	#endif

	/* Try to bind starting from INITIAL_SERV_PORT = 6990, and on error "address already in use" try next port */
	while(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		if(errno != EADDRINUSE)
			perror_and_failure("on bind attempt", __func__);
		else {
			serv_port++;
			addr.sin_port = htons(serv_port);
		}
	}

	/* Set the TCP connection socket on LISTEN */
	if(listen(sockfd, MAX_BACKLOG) < 0)
		perror_and_failure("listen()", __func__);

	char *cwd = getcwd(NULL, 0);

	printf("SERVER ACTIVE:\n"
		"\t            Bound on port : <%d>\n"
		"\tCurrent working directory : \n\t\t<%s>\n\n", 
		ntohs(addr.sin_port),
		cwd);

	free(cwd);

	printf("Server Log:\n");

	main_cycle();

	return 0;
}

void main_cycle(){
	
	socklen_t client_addr_len;
	int free_thread;	/* It's safe to use int, MAX_THREAD upper bound is 32767 */

	while(1){
		/* Initialize the struct to contain the address of the client once connected */
		struct sockaddr_in *client_addr = malloc(sizeof(struct sockaddr_in));
		if(client_addr == NULL) perror_and_failure("client_addr malloc()", __func__);
		memset(client_addr, '\0', sizeof(struct sockaddr_in));
		client_addr_len = sizeof(struct sockaddr_in);

		struct thread_arg *arg = malloc(sizeof(struct thread_arg));
		if(arg == NULL) perror_and_failure("thread_arg malloc()", __func__);

		/* Allow SIGINT */
		pthread_sigmask(SIG_UNBLOCK, &set_sigint, NULL);

		/* Wait for a free thread, BLOCKING */
		short_semop(sem_free_threads, -1);

		/* Find first free thread */
		if((free_thread = bitmask_find_first_set(&bm_free_threads)) < 0)
			perror_and_failure("No free thread found in bitmask_find...()", __func__);

		/* Accept a pending connection, blocking call */
		arg->acceptfd = accept(sockfd, (struct sockaddr *)client_addr, &client_addr_len);
		if(arg->acceptfd < 0) perror_and_failure("accept", __func__);

		/* Block SIGINT */
		pthread_sigmask(SIG_BLOCK, &set_sigint, NULL);

		/* Set thread as busy */
		bitmask_del(&bm_free_threads, free_thread);

		/* Create a thread for handling the communication with client */
		arg->id = free_thread;
		arg->client_addr = client_addr;
		pthread_create(&tids[free_thread], NULL, thread_communication_routine, (void *)arg);
	}

	return;
}

void signal_handler(int signum){
	if(signum == SIGINT){

		/* Close connection socket */
		if(close(sockfd) < 0) perror("Error in sigint_handler on close\n");

		printf("\nSIGINT catched.\nSending SIGUSR1 to threads.\n");

		/* Send SIGUSR1 to all threads */
		for(int i = 0; i < MAX_THREAD; i++){
			if(tids[i] != 0)
				pthread_kill(tids[i], SIGUSR1);
		}

		printf("Joining threads.\n");

		/* Join threads */
		for(int i = 0; i < MAX_THREAD; i++){
			if(tids[i] != 0)
				pthread_join(tids[i], NULL);
		}

		/* Close all semaphores */
		semctl(UR, 0, IPC_RMID);
		semctl(UW, 0, IPC_RMID);
		semctl(MR, 0, IPC_RMID);
		semctl(MW, 0, IPC_RMID);
		semctl(sem_free_threads, 0, IPC_RMID);

		printf("Server terminated.\n\n");

		/* Exit */
		exit(0);

	} else if(signum == SIGUSR1){
		/* Thread close connection */
		thread_close_connection();

		/* Thread exit */
		pthread_exit(NULL);
	}
	
	fprintf(stderr, "Unexpected signal %d catched. Quitting.\n", signum);
	perror_and_failure(__func__, NULL);
	return;
}

void display_help(){
	printf("Usage: xserver initial_port\n"
		"Start the server in this directory.\n\n"
		"Options:\n"
		"\t-h and --help: display this help and exit.\n\n"
		"initial_port must be in range [1024, 65535]\n");	
}