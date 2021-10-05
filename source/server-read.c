#include "server.h"

int close_and_return(int return_value, FILE ***files);

int read_response(int acceptfd, user_info client_ui, operation *op){

	#ifdef PRINT_DEBUG
	printf("(%s, %d): read request.\n", client_ui.username, client_ui.uid);
	#endif
	#ifdef PRINT_DEBUG_FINEI
	print_operation(op);
	#endif

	struct sembuf actual_sem_op = {.sem_flg = 0, .sem_num = 0};

	FILE *index_file, *messages_file;
	FILE **files[2] = {&messages_file, &index_file};

	int mid, max_mid;

	uint32_t message_len, uid;
	uint64_t message_offset;

	char *message_read, *text_to_send;
	size_t text_to_send_len;

	#ifdef PRINT_DEBUG_FINE
	char *subject, *body;
	#endif

	// #1: Wait (MW, 1)
	actual_sem_op.sem_op = -1;
	if(semop(MW, &actual_sem_op, 1) < 0)
		exit_failure();

	// #2: Wait (MR, 1)
	actual_sem_op.sem_op = -1;
	if(semop(MR, &actual_sem_op, 1) < 0)
		exit_failure();

	// #3: Signal (MW, 1)
	actual_sem_op.sem_op = 1;
	if(semop(MW, &actual_sem_op, 1) < 0)
		exit_failure();

	// #4: Compute number of messages from "Index file" length
	messages_file = fdopen(open(MESSAGES_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	index_file = fdopen(open(INDEX_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	if(messages_file == NULL || index_file == NULL){
		perror("Error in server-save on fdopen");
		exit_failure();
	}

	fseek(index_file, 0, SEEK_END);
	max_mid = ftell(index_file) / INDEX_LINE_LEN;

	// #5: For cycle messages send
	for(mid = 0; mid < max_mid; mid++){
		// #5.1: Read from "Index file": (message_offset, message_len, uid)
		fseek(index_file, mid * INDEX_LINE_LEN, SEEK_SET);
		fread(&message_offset, 1, sizeof(uint64_t), index_file);
		fread(&message_len, 1, sizeof(uint32_t), index_file);
		fread(&uid, 1, sizeof(uint32_t), index_file);

		#ifdef PRINT_DEBUG_FINE
		printf("MID=%d, message_offset=%ld, message_len=%d, uid=%d\n",
			mid, message_offset, message_len, uid);
		#endif

		// #5.2: Read from "Messages file" from message_offset Subject and Title
		message_read = calloc(sizeof(char), message_len);
		if(message_read == NULL)
			return close_and_return(-1, files);

		fseek(messages_file, message_offset, SEEK_SET);
		fread(message_read, 1, message_len, messages_file);
		message_read[message_len] = '\0';

		#ifdef PRINT_DEBUG_FINE
		subject = &message_read[0];
		body = &message_read[strlen(subject) + 1];
		printf("subject=%s\nbody=%s\n", subject, body);
		#endif

		text_to_send_len = snprintf(NULL, 0, "%d\n%d\n%s", mid, uid, message_read);
		text_to_send = calloc(sizeof(char), text_to_send_len + 1);
		if(text_to_send == NULL)
			return close_and_return(-1, files);

		sprintf(text_to_send, "%d\n%d\n%s", mid, uid, message_read);

		if(send_message_to(acceptfd, UID_SERVER, OP_READ_RESPONSE, text_to_send) < 0)
			return close_and_return(-1, files);

		free(message_read);
		free(text_to_send);
	}	

	// #6: Send (UID_SERVER, OP_OK, NULL)
	if(send_message_to(acceptfd, UID_SERVER, OP_OK, NULL) < 0)
		return close_and_return(-1, files);

	return close_and_return(0, files);
}

int close_and_return(int return_value, FILE ***files){
	// #N-1: Signal (MR, 1)
	struct sembuf actual_sem_op = {.sem_flg = 0, .sem_num = 0, .sem_op = 1};
	if(semop(MR, &actual_sem_op, 1) < 0)
		exit_failure();

	// Close files
	for(int i = 0; i < 2; i++){
		fclose(*files[i]);
	}

	return return_value;
}