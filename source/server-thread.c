#include "server.h"

__thread int acceptfd;
__thread user_info client_ui;
__thread operation op;
__thread int id;

void *thread_close_connection();

void *thread_communication_routine(void *arg){
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
	//	gets interrupted when receive_message_from returns 0, meaning that connection was closed by client
	while(dispatcher() == 0);

	return thread_close_connection();
}

int dispatcher(){
	// Receive first operation
	if(receive_operation_from(acceptfd, &op) < 0)
		return -1;

	#ifdef PRINT_DEBUG_FINE
	printf("BEGIN%s\n(%s, %d) sent \'%c\' op:\n%s\n%sEND\n\n",
	SEP, client_ui.username, client_ui.uid, op.code, op.text, SEP);
	#endif
	
	switch(op.code){
		case OP_MSG:
			return post();
		case OP_READ_REQUEST:
			return read_all();
		case OP_DELETE_REQUEST:
			return delete_post();
		default:
			printf("BEGIN%s\n(%s, %d) sent \'%c\' op:\n%s\n%sEND\n\n",
			SEP, client_ui.username, client_ui.uid, op.code, op.text, SEP);
			return send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect OP");			
	}
}

void *thread_close_connection(){
	printf("Thread[%d]: closed connection.\n", id);

	// Set thread as free
	bitmask_add(&bm_free_threads, id);

	bitmask_print(&bm_free_threads);

	// Post on free threads semaphore
	short_semop(sem_free_threads, 1);

	if(close(acceptfd) < 0 && errno != EBADF) perror("Error in server on close accepted socket\n");
	return NULL;
}
