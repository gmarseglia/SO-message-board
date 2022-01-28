#include "server.h"

__thread int acceptfd;
__thread user_info_t client_ui;
__thread operation_t op;
__thread int id;


void *thread_communication_routine(void *arg){
	// Block signals
	pthread_sigmask(SIG_BLOCK, &set_both, NULL);

	// Initialize local variables and free argument struct
	struct thread_arg *t_arg = (struct thread_arg *)arg;
	id = t_arg->id;
	acceptfd = t_arg->acceptfd;
	struct sockaddr_in *client_addr = (struct sockaddr_in *)t_arg->client_addr;

	char *str_client_addr = inet_ntoa(client_addr->sin_addr);
	int i_client_port = ntohs(client_addr->sin_port);

	free(client_addr);
	free(arg);

	printf("Thread[%d]: accepted connection from %s:%d\n", id, str_client_addr, i_client_port);

	// Start the login or registration phase, mandatory for every client
	if(login_registration(acceptfd, &client_ui) < 0)
		return thread_close_connection();

	printf("Thread[%d]: (%s, %d) authenticated from %s:%d\n",
		id, client_ui.username, client_ui.uid, str_client_addr, i_client_port);

	// Main cycle
	//	gets interrupted when receive_operation_from returns 0, meaning that connection was closed by client
	while(dispatcher() == 0);

	return thread_close_connection();
}

int dispatcher(){
	/* #1: Allow SIGUSR1 */
	pthread_sigmask(SIG_UNBLOCK, &set_sigusr1, NULL);

	/* #2: Receive operation */
	if(receive_operation_from_2(acceptfd, &op) < 0)
		return -1;

	/* #3: Block SIGUSR1 */
	pthread_sigmask(SIG_BLOCK, &set_sigusr1, NULL);

	#ifdef PRINT_DEBUG_FINE
	printf("BEGIN%s\n(%s, %d) sent \'%c\' op:\n%s\n%sEND\n\n",
	SEP, client_ui.username, client_ui.uid, op.code, op.text, SEP);
	#endif
	
	switch(op.code){
		/* #4a: Post */
		case OP_MSG:
			return post_message();

		/* #4b: Read all */
		case OP_READ_REQUEST:
			return read_all_messages();

		/* #4c: Delete */
		case OP_DELETE_REQUEST:
			return delete_message();

		/* If request with unkwown code is received, then send operation NOT ACCEPTED */
		default:
			printf("BEGIN%s\n(%s, %d) sent \'%c\' op:\n%s\n%sEND\n\n",
			SEP, client_ui.username, client_ui.uid, op.code, op.text, SEP);

			if(send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect OP") < 0)
				return -1;

			return 0;			
	}
}

void *thread_close_connection(){
	printf("Thread[%d]: closed connection.\n", id);

	/* #N-2: Mark thread as free */
	/* Mark slot as free on the bitmask */
	bitmask_add(&bm_free_threads, id);
	/* Signal on slots semaphore */
	short_semop(sem_free_threads, 1);

	/* #N-1: Close socket */
	if(close(acceptfd) < 0 && errno != EBADF) perror("Error in server on close accepted socket\n");
	
	/* Thread exit */
	return NULL;
}
