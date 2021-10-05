#include "server.h"

int close_and_return(int return_value, FILE ***files);

int read_response(int acceptfd, user_info client_ui, operation *op){

	printf("(%s, %d): read request.\n", client_ui.username, client_ui.uid);

	FILE *index_file, *messages_file;
	FILE **files[] = {&messages_file, &index_file, NULL};

	int mid, max_mid;
	int send_success;

	uint32_t message_len, uid;
	uint64_t message_offset;

	char *message_read, *text_to_send;
	size_t text_to_send_len;

	// #1: Wait (MW, 1)
	short_semop(MW, -1);

	// #2: Wait (MR, 1)
	short_semop(MR, -1);

	// #3: Signal (MW, 1)
	short_semop(MW, 1);

	// Open files
	messages_file = fdopen(open(MESSAGES_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	index_file = fdopen(open(INDEX_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	if(messages_file == NULL || index_file == NULL){
		perror("Error in server-save on fdopen");
		exit_failure();
	}

	// #4: Compute number of messages from "Index file" length
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
		// Cut the second '\n' in body
		message_read[message_len - 1] = '\0';

		// #5.4: Send (SERVER_UID, OP_READ_RESPONSE, MID + '\n' + Username + '\n' + Subject + '\n' + Body)
		text_to_send_len = snprintf(NULL, 0, "%d\n%d\n%s", mid, uid, message_read);
		text_to_send = calloc(sizeof(char), text_to_send_len + 1);
		if(text_to_send == NULL)
			return close_and_return(-1, files);

		sprintf(text_to_send, "%d\n%d\n%s", mid, uid, message_read);

		send_success = send_message_to(acceptfd, UID_SERVER, OP_READ_RESPONSE, text_to_send);
		
		free(message_read);
		free(text_to_send);
		if(send_success < 0)
			return close_and_return(-1, files);

	}	

	// #6: Send (UID_SERVER, OP_OK, NULL)
	if(send_message_to(acceptfd, UID_SERVER, OP_OK, NULL) < 0)
		return close_and_return(-1, files);

	return close_and_return(0, files);
}

int close_and_return(int return_value, FILE ***files){
	// #N-1: Signal (MR, 1)
	short_semop(MR, 1);

	// Close files
	for(int i = 0; files[i] != NULL; i++)
		fclose(*files[i]);
	
	return return_value;
}