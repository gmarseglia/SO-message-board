#include "server-routines.h"

int close_and_return(int return_value, FILE ***files);

int read_response(int acceptfd, user_info client_ui, operation *op){
	printf("(%s, %d): read request.\n", client_ui.username, client_ui.uid);
	print_operation(op);

	struct sembuf actual_sem_op = {.sem_flg = 0, .sem_num = 0};

	FILE *index_file, *messages_file;
	FILE **files[2] = {&messages_file, &index_file};

	int mid, max_mid;

	uint32_t message_len, uid;
	uint64_t message_offset;

	char *buffer;

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

	#ifdef PRINT_DEBUG_FINE
	printf("max_mid=%d\n", max_mid);
	#endif

	// #5: For cycle messages send
	for(mid = 0; mid < max_mid; mid++){
		// #5.1: Read from "Index file": (message_offset, message_len, uid)
		fseek(index_file, mid * INDEX_LINE_LEN, SEEK_SET);
		fread(&message_offset, 1, sizeof(uint64_t), index_file);
		fread(&message_len, 1, sizeof(uint32_t), index_file);
		fread(&uid, 1, sizeof(uint32_t), index_file);

		printf("MID=%d, message_offset=%ld, message_len=%d, uid=%d\n",
			mid, message_offset, message_len, uid);

		// #5.2: Read from "Messages file" from message_offset Subject and Title
		buffer = calloc(sizeof(char), message_len + 1);
		if(buffer == NULL)
			return close_and_return(-1, files);
		buffer[message_len] = '\0';

		fseek(messages_file, message_offset, SEEK_SET);
		fread(buffer, 1, message_len, messages_file);

		#ifdef PRINT_DEBUG_FINE
		subject = &buffer[0];
		body = &buffer[strlen(subject) + 1];
		printf("subject=%s\nbody=%s\n", subject, body);
		#endif

		// RESPONSE WITH UID OF THE POSTER
		if(send_message_to(acceptfd, uid, OP_READ_RESPONSE, buffer) < 0)
			return close_and_return(-1, files);

		free(buffer);
	}	

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