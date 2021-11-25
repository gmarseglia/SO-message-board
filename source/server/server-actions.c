#include "server.h"

const uint64_t DELETED_OFFSET = 0xffffffffffffffff;

extern __thread int acceptfd;
extern __thread user_info_t client_ui;
extern __thread operation_t op;

__thread FILE *messages_file, *index_file,*free_areas_file;

__thread FILE ***files;
__thread char ***buffers;

__thread uint32_t message_len;
__thread uint32_t message_uid;
__thread uint64_t message_offset;

/*
	DESCRIPTION:
		Closes all files, free all buffers, return
*/
int close_return(int return_value);
int close_read_all(int return_value);
int close_delete(int return_value, char semaphores);

void read_mid_from_index(int mid);
// ---------------------------------------------------------------

int post_message(){
	char *subject, *body;

	/* Contains the buffer which are going to be freed at the end of the function */
	char **actual_buffers[] = {&subject, &op.text, NULL};
	buffers = actual_buffers;

	int mid;
	long index_file_len;

	/* Check UID match */
	message_uid = (uint32_t) op.uid;
	if(client_ui.uid != message_uid){
		printf("client_ui.uid=%d != message_uid=%d\n", client_ui.uid, message_uid);
		return -1;
	}

	printf("(%s, %d): message sent.\n",	client_ui.username, client_ui.uid);

	/* #1: Extract Subject and Body from OP text */
	sscanf(op.text, "%m[^\n]", &subject);
	body = &op.text[strlen(subject) + 1];

	message_len = strlen(subject) + 1 + strlen(body);

	#ifdef PRINT_DEBUG_FINE
		printf("BEGIN%s\n(%s, %d) sent:\n%s\n%s\n%s\n%s\n%sEND\n", SEP,
			client_ui.username, client_ui.uid, SEP,
			subject, SEP,
			body, SEP);
	#endif

	/* Contains all the files which are going to be closed at the end of the func */
	FILE **actual_files[] = {&messages_file, &index_file, &free_areas_file, NULL};
	files = actual_files;

	/* #2: Open "Messages file", "Index file", "Free areas file" */
	messages_file = FDOPEN_MESSAGES();
	index_file = FDOPEN_INDEX();
	free_areas_file = FDOPEN_FREE_AREAS();

	if(messages_file == NULL || index_file == NULL || free_areas_file == NULL)
		perror_and_failure("FDOPEN", __func__);

	/* #3: wait(MW, 1) */
	short_semop(MW, -1);

	/* #4: wait(MR, MAX_THREAD) */
	short_semop(MR, -MAX_THREAD);

	/* #5: Compute MID, Message ID */
	fseek(index_file, 0, SEEK_END);		/* Go to the end of the file */
	index_file_len = ftell(index_file);	/* Check the number of bytes in the file */
	/* Compute the number of lines, which is equal to the MID */
	mid = index_file_len / INDEX_LINE_LEN;

	#ifdef PRINT_DEBUG_FINE
		printf("index_file_len = %ld, mid = %d\n", index_file_len, mid);
	#endif

	/* #6: Search for free memory area in "Free file" using first fit */
	uint64_t read_offset;
	uint32_t read_len;
	size_t free_areas_file_len, read_bytes = 0;

	/* The default offset at which the message will be saved 
	** is at the end of the file */
	fseek(messages_file, 0, SEEK_END);
	message_offset = ftell(messages_file);

	/* Find "free areas file" length */
	fseek(free_areas_file, 0, SEEK_END);	
	free_areas_file_len = ftell(free_areas_file);
	fseek(free_areas_file, 0, SEEK_SET);

	/* Cycle through the file which contains the free areas offsets */
	for(read_bytes = 0; read_bytes < free_areas_file_len; ){
		/* Read the offset of the free area */
		fread(&read_offset, 1, sizeof(uint64_t), free_areas_file);
		/* Read the length in bytes of the free area */
		fread(&read_len, 1, sizeof(uint32_t), free_areas_file);

		/* Check if the free area is big enough to contain the message received */
		if(read_len >= message_len){
			message_offset = read_offset;	/* Update the offset */

			/* #7a: Update "free areas file" */
			fseek(free_areas_file, read_bytes, SEEK_SET);

			/* New offset is the end of the message */
			read_offset = read_offset + message_len;
			/* New length is reduced by the bytes used by the message*/
			read_len = read_len - message_len;	
			fwrite(&read_offset, 1, sizeof(uint64_t), free_areas_file);
			fwrite(&read_len, 1, sizeof(uint32_t), free_areas_file);
			break;
		}

		read_bytes += FREE_AREAS_LINE_LEN;	/* For cycle increment */
	}

	/* #7: Write on "Index file" at MID line:
	** 	(Message offset, Message len, Sender UID) */
	fwrite(&message_offset, 1, sizeof(uint64_t), index_file);
	fwrite(&message_len, 1, sizeof(uint32_t), index_file);
	fwrite(&message_uid, 1, sizeof(uint32_t), index_file);

	/* #8: Write on "Message file" at Message offset: (Subject , '\n', Body) */
	fseek(messages_file, message_offset, SEEK_SET);
	fwrite(subject, 1, strlen(subject), messages_file);
	fwrite("\n", 1, sizeof(char), messages_file);
	fwrite(body, 1, strlen(body), messages_file);

	/* #9: signal(MR, MAX_THREAD) */
	short_semop(MR, MAX_THREAD);

	/* #10: signal(MW, 1) */
	short_semop(MW, 1);

	/* #11: Send (SERVER_UID, OP_OK, MID) */
	char mid_str[32];
	sprintf(mid_str, "%d", mid);
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, mid_str) < 0)
		return close_return(-1);

	return close_return(0);
}

int read_all_messages(){
	printf("(%s, %d): read request.\n", client_ui.username, client_ui.uid);

	int mid, max_mid;
	int send_success;

	char *username;
	char *message_read, *text_to_send;
	size_t text_to_send_len;

	/* #1: wait(MW, 1)	*/
	short_semop(MW, -1);

	/* #2: wait(MR, 1) */
	short_semop(MR, -1);

	/* #3: signal(MW, 1) */
	short_semop(MW, 1);

	/* Contains files which are going to be closed */
	FILE **actual_files[] = {&messages_file, &index_file, NULL};
	files = actual_files;

	/* Open files */
	messages_file = FDOPEN_MESSAGES();
	index_file = FDOPEN_INDEX();

	if(messages_file == NULL || index_file == NULL)
		perror_and_failure("FDOPEN", __func__);

	/* #4: Compute number of messages from "Index file" length */
	fseek(index_file, 0, SEEK_END);
	max_mid = ftell(index_file) / INDEX_LINE_LEN;

	/* #5: For cycle through messages to send */
	for(mid = 0; mid < max_mid; mid++){

		// #5.1: Read from "Index file": (message_offset, message_len, message_uid)
		read_mid_from_index(mid);

		#ifdef PRINT_DEBUG_FINE
			printf("MID=%d, message_offset=%ld, message_len=%d, message_uid=%d\n",
				mid, message_offset, message_len, message_uid);
		#endif

		if(message_offset == DELETED_OFFSET)
			/* #5.2a: Skip deleted */
			continue;

		/* #5.2: Read from "Messages file" at message_offset: (subject, '\n', body) */
		message_read = calloc(sizeof(char), message_len); /* Allocate space */
		if(message_read == NULL) return close_read_all(-1);

		fseek(messages_file, message_offset, SEEK_SET);
		fread(message_read, 1, message_len, messages_file);	/* Read from message file */
		message_read[message_len] = '\0';

		/* #5.3: Convert UID into Username */
		user_info_t *read_ui = find_user_by_uid(message_uid);
		username = (read_ui == NULL) ? "Not found" : read_ui->username;

		/* #5.4: Send (SERVER_UID, OP_READ_RESPONSE,
		**	MID + '\n' + Username + '\n' + Subject + '\n' + Body) */
		text_to_send_len = snprintf(NULL, 0, "%d\n%s\n%s", mid, username, message_read);
		text_to_send = calloc(sizeof(char), text_to_send_len + 1);	/* Allocate space */
		if(text_to_send == NULL) return close_read_all(-1);

		/* Write full operation text */
		sprintf(text_to_send, "%d\n%s\n%s", mid, username, message_read);

		/* Send operation to Client */
		send_success = send_operation_to(acceptfd, UID_SERVER, OP_READ_RESPONSE, text_to_send);

		free(read_ui);
		free(message_read);
		free(text_to_send);

		if(send_success < 0)
			return close_read_all(-1);

	}	

	/* #6: Send (UID_SERVER, OP_OK, NULL)	*/
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, NULL) < 0)
		return close_read_all(-1);

	return close_read_all(0);
}

int delete_message(){
	int target_mid;

	/* #1: Read target MID */
	sscanf(op.text, "%d", &target_mid);
	free(op.text);

	printf("(%s, %d): deletion of post #%d requested.\n",
		client_ui.username, client_ui.uid, target_mid);
	
	/* Files that are going to be closed */
	FILE **actual_files[] = {&messages_file, &index_file, &free_areas_file, NULL};
	files = actual_files;

	/* Open "Messages file", "Index file", "Free areas file" */
	messages_file = FDOPEN_MESSAGES();
	index_file = FDOPEN_INDEX();
	free_areas_file = FDOPEN_FREE_AREAS();

	if(messages_file == NULL || index_file == NULL || free_areas_file == NULL)
		perror_and_failure("FDOPEN", __func__);

	/* #2: Check target MID in bound */
	fseek(index_file, 0, SEEK_END);
	/* Compute number of lines = max MID */
	if(target_mid >= ftell(index_file) / INDEX_LINE_LEN){
		/* #2a: Send OP_NOT_ACCEPTED */
		return close_delete(
			send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Post doesn't exist."), 
			0);
	}

	/* #3: wait(MW, 1) */
	short_semop(MW, -1);

	/* #4: wait(MR, MAX_THREAD) */
	short_semop(MR, -MAX_THREAD);

	/* #5: Read from "index file" offset and len of MID message */
	read_mid_from_index(target_mid);

	/* #6: check UID match */
	if(op.uid != message_uid){
		/* #7a: send (UID_SERVER, OP_NOT_ACCETPED, "UID don't match") */
		return close_delete(
			send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "UID don't match"), 
			1);
	}

	/* #7: check if message already deleted */
	if(message_offset == DELETED_OFFSET){
		/* #8a: send (UID_SERVER, OP_NOT_ACCEPTED, "Message already deleted") */
		return close_delete(
			send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Message already deleted"),
			1);	
	}

	/* #8: write DELETED_OFFSET in message_offset in index */
	fseek(index_file, target_mid * INDEX_LINE_LEN, SEEK_SET);
	fwrite(&DELETED_OFFSET, 1, sizeof(uint64_t), index_file);

	/* #9: append in "free-areas file" (offset, len) */
	fseek(free_areas_file, 0, SEEK_END);
	fwrite(&message_offset, 1, sizeof(uint64_t), free_areas_file);
	fwrite(&message_len, 1, sizeof(uint32_t), free_areas_file);

	printf("(%s, %d): deletion of post #%d accepted.\n",
		client_ui.username, client_ui.uid, target_mid);

	return close_delete(send_operation_to(acceptfd, UID_SERVER, OP_OK, NULL), 1);
}

// -----------------------------------

int close_return(int return_value){
	if(files != NULL){
		for(int i = 0; files[i] != NULL; i++)
			fclose(*files[i]);
	}
	
	if(buffers != NULL){
		for(int i = 0; buffers[i] != NULL; i++)
			free(*buffers[i]);
	}
	
	return return_value;
}

int close_read_all(int return_value){
	// #N-1: Signal (MR, 1)
	short_semop(MR, 1);

	// Close files
	buffers = NULL;
	return close_return(return_value);
}

int close_delete(int return_value, char semaphores){
	buffers = NULL;

	if(semaphores == 1){
		// #N-2: signal(MR, MAX_THREAD)
		short_semop(MR, MAX_THREAD);

		// #N-1: signal(MW, 1)
		short_semop(MW, 1);
	}

	return close_return(return_value);
}

// ----------------------------------------------------

void read_mid_from_index(int mid){
	fseek(index_file, mid * INDEX_LINE_LEN, SEEK_SET);
	fread(&message_offset, 1, sizeof(uint64_t), index_file);
	fread(&message_len, 1, sizeof(uint32_t), index_file);
	fread(&message_uid, 1, sizeof(uint32_t), index_file);
}

user_info_t *find_user_by_username(char *username){
	// Open Users file
	FILE *users_file = FDOPEN_USERS();
	if(users_file == NULL) perror_and_failure("FDOPEN()", __func__);

	user_info_t *read_ui = malloc(sizeof(user_info_t));
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

user_info_t *find_user_by_uid(int uid){
	// Open Users file
	FILE *users_file = FDOPEN_USERS();
	if(users_file == NULL) perror_and_failure("FDOPEN_USERS()", __func__);

	user_info_t *read_ui = malloc(sizeof(user_info_t));
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
