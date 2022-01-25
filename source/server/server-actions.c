#include "server.h"

const uint64_t DELETED_OFFSET = 0xffffffffffffffff;

extern __thread int acceptfd;
extern __thread user_info_t client_ui;
extern __thread operation_t op;
extern __thread int id;

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
void close_files_and_buffers();
void close_post();
void close_read_all();
void close_delete();

void read_mid_from_index(int mid);
// ---------------------------------------------------------------

int post_message(){
	/* Subject and body of the message */
	char *subject, *body;

	/* Contains all the files which are going to be closed at the end of the func */
	FILE **actual_files[] = {&messages_file, &index_file, &free_areas_file, NULL};
	files = actual_files;

	/* Contains the buffer which are going to be freed at the end of the function */
	char **actual_buffers[] = {&subject, &op.text, NULL};
	buffers = actual_buffers;

	int mid;
	long index_file_len;

	printf("Thread[%d]: received post request from (%s, %d).\n", id, client_ui.username, client_ui.uid);

	/* #1: Extract Subject and Body from OP text */
	sscanf(op.text, "%m[^\n]", &subject);
	body = &op.text[strlen(subject) + 1];

	message_len = strlen(subject) + 1 + strlen(body);

	/* #2: wait(MW, 1) */
	short_semop(MW, -1);

	/* #3: wait(MR, MAX_THREAD) */
	short_semop(MR, -MAX_THREAD);

	/* #4: Open "Messages file", "Index file", "Free areas file" */
	messages_file = FDOPEN_MESSAGES();
	index_file = FDOPEN_INDEX();
	free_areas_file = FDOPEN_FREE_AREAS();

	/* Check if files are open */
	if(messages_file == NULL || index_file == NULL || free_areas_file == NULL)
		perror_and_failure("FDOPEN", __func__);

	/* #5: Compute MID, Message ID */
	fseek(index_file, 0, SEEK_END);		/* Go to the end of the file */
	index_file_len = ftell(index_file);	/* Check the number of bytes in the file */
	/* Compute the number of lines, which is equal to the MID */
	mid = index_file_len / INDEX_LINE_LEN;

	#ifdef PRINT_DEBUG_FINE
		printf("index_file_len = %ld, mid = %d\n", index_file_len, mid);
	#endif

	/* #6: Search for free memory area in "Free file" using first fit */
	uint64_t read_offset,write_offset;
	uint32_t read_len, write_len;
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
	for(read_bytes = 0; read_bytes < free_areas_file_len; read_bytes += FREE_AREAS_LINE_LEN){
		/* Read the offset of the free area */
		fread(&read_offset, 1, sizeof(uint64_t), free_areas_file);
		/* Read the length in bytes of the free area */
		fread(&read_len, 1, sizeof(uint32_t), free_areas_file);

		/* Check if free area is valid and big enough to contain the message received*/
		if(read_offset != DELETED_OFFSET && read_len >= message_len){
			message_offset = read_offset;	/* Update the offset */

			/* #7a: Update "free areas file" */
			fseek(free_areas_file, read_bytes, SEEK_SET);

			/* If the free area is empty, then mark as deleted */ 
			if(read_len == message_len){
				write_offset = DELETED_OFFSET;
				write_len = 0;
			}
			/* If the free are has still space, then update */
			else {
				/* New offset is the end of the message */
				write_offset = read_offset + message_len;
				/* New offset is the end of the message */
				write_len = read_len - message_len;
			}

			/* Write on file */
			fwrite(&write_offset, 1, sizeof(uint64_t), free_areas_file);
			fwrite(&write_len, 1, sizeof(uint32_t), free_areas_file);

			break;
		}
	}

	/* #7: Write on "Index file" at MID line:
	** 	(Message offset, Message len, Sender UID) */
	fwrite(&message_offset, 1, sizeof(uint64_t), index_file);
	fwrite(&message_len, 1, sizeof(uint32_t), index_file);
	message_uid = (uint32_t) op.uid;
	fwrite(&message_uid, 1, sizeof(uint32_t), index_file);

	/* #8: Write on "Message file" at Message offset: (Subject , '\n', Body) */
	fseek(messages_file, message_offset, SEEK_SET);
	fwrite(subject, 1, strlen(subject), messages_file);
	fwrite("\n", 1, sizeof(char), messages_file);
	fwrite(body, 1, strlen(body), messages_file);

	/* #9: Flushes changes and close files
		This is important */
	for(int i = 0; files[i] != NULL; i++){
		fsync(fileno(*files[i]));
	}

	printf("Thread[%d]: saved a new message.\n", id);
	
	close_post();

	char mid_str[32];
	sprintf(mid_str, "%d", mid);

	/* #11: Send operation OK
		(SERVER_UID, OP_OK, MID) */
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, mid_str) < 0){
		printf("Thread[%d]: client unreachable.\n", id);
		return -1;
	}

	return 0;
}

int read_all_messages(){
	/* Contains files which are going to be closed */
	FILE **actual_files[] = {&messages_file, &index_file, NULL};
	files = actual_files;

	int mid, max_mid;
	int send_success;

	char *username;
	char *message_read, *text_to_send;
	size_t text_to_send_len;

	printf("Thread[%d]: received read request from (%s, %d).\n", id, client_ui.username, client_ui.uid);
	
	/* #1: wait(MW, 1)	*/
	short_semop(MW, -1);

	/* #2: wait(MR, 1) */
	short_semop(MR, -1);

	/* #3: signal(MW, 1) */
	short_semop(MW, 1);

	/* Open files */
	messages_file = FDOPEN_MESSAGES();
	index_file = FDOPEN_INDEX();

	if(messages_file == NULL || index_file == NULL)
		perror_and_failure("FDOPEN", __func__);

	/* #4: Compute number of messages from "Index file" length */
	fseek(index_file, 0, SEEK_END);
	max_mid = ftell(index_file) / INDEX_LINE_LEN;

	/* #5: Cycle through messages */
	for(mid = 0; mid < max_mid; mid++){

		/* #5.1: Read from "Index file": (message_offset, message_len, message_uid) */
		read_mid_from_index(mid);

		/* Skip deleted messages */
		if(message_offset == DELETED_OFFSET)
			continue;

		/* #5.2: Read from "Messages file" at message_offset: (subject, '\n', body) */
		message_read = calloc(sizeof(char), message_len); /* Allocate space */

		/* If calloc failed, then action ends not correctly */
		if(message_read == NULL){
			printf("Thread[%d]: read request failed: calloc() fail.\n", id);
			close_read_all();
			return -1;
		}

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

		/* If calloc() failed, then action ends not correctly */
		if(text_to_send == NULL){
			printf("Thread[%d]: read request failed: calloc() fail.\n", id);
			close_read_all();
			return -1;
		} 

		/* Write full operation text */
		sprintf(text_to_send, "%d\n%s\n%s", mid, username, message_read);

		/* Send operation to Client */
		send_success = send_operation_to(acceptfd, UID_SERVER, OP_READ_RESPONSE, text_to_send);

		free(read_ui);
		free(message_read);
		free(text_to_send);

		if(send_success < 0){
			printf("Thread[%d]: read request failed: send_operation_to() fail.\n", id);
			close_read_all();
			return -1;
		}
	}

	printf("Thread[%d]: read request done.\n", id);

	close_read_all();

	/* #6: Send (UID_SERVER, OP_OK, NULL) */
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, NULL) < 0){
		printf("Thread[%d]: client unreachable.\n", id);
		return -1;
	}

	return 0;
}

int delete_message(){
	/* Files that are going to be closed */
	FILE **actual_files[] = {&messages_file, &index_file, &free_areas_file, NULL};
	files = actual_files;

	int target_mid;

	/* #1: Read target MID */
	sscanf(op.text, "%d", &target_mid);
	free(op.text);

	printf("Thread[%d]: received delete request for post #%d from (%s, %d).\n",
		id, target_mid, client_ui.username, client_ui.uid);

	/* #2: wait(MW, 1) */
	short_semop(MW, -1);

	/* #3: wait(MR, MAX_THREAD) */
	short_semop(MR, -MAX_THREAD);
	
	/* Open "Messages file", "Index file", "Free areas file" */
	messages_file = FDOPEN_MESSAGES();
	index_file = FDOPEN_INDEX();
	free_areas_file = FDOPEN_FREE_AREAS();

	if(messages_file == NULL || index_file == NULL || free_areas_file == NULL)
		perror_and_failure("FDOPEN", __func__);

	/* Compute number of lines = max MID */
	fseek(index_file, 0, SEEK_END);
	if(target_mid >= ftell(index_file) / INDEX_LINE_LEN || target_mid < 0){

		/* If MID is not in bound, then
			#4a: Send opertation NOT ACCEPTED */
		close_delete();

		printf("Thread[%d]: delete request failed: target_mid not in buond.\n", id);

		if(send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Post doesn't exist.") < 0){
			printf("Thread[%d]: client unreachable.\n", id);
			return -1;
		}

		return 0;
	}

	/* #5: Read from "Index file": offset, len and sender UID of message #MID */
	read_mid_from_index(target_mid);

	/* Check if UID of sender and of Client match */
	if(op.uid != message_uid){

		/* If UIDs don't match, then
			#6a: send operation NOT ACCEPTED
			(UID_SERVER, OP_NOT_ACCETPED, "UID don't match") */
		close_delete();

		printf("Thread[%d]: delete request failed: UID don't match.\n", id);

		if(send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "UID don't match") < 0){
			printf("Thread[%d]: client unreachable.\n", id);
			return -1;
		}

		return 0;
	}

	/* Check if message was already deleted */
	if(message_offset == DELETED_OFFSET){

		/* If the message was already deleted, then
			#6b: send operation NOT ACCEPTED
			(UID_SERVER, OP_NOT_ACCEPTED, text) */
		close_delete();

		printf("Thread[%d]: delete request failed: message already deleted.\n", id);

		if(send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Message already deleted") < 0){
			printf("Thread[%d]: client unreachable.\n", id);
			return -1;
		}

		return 0;
	}

	/* #6: Write DELETED_OFFSET in "Index file" at message_offset */
	fseek(index_file, target_mid * INDEX_LINE_LEN, SEEK_SET);
	fwrite(&DELETED_OFFSET, 1, sizeof(uint64_t), index_file);

	/* #7: Look for free areas adjacent to the message */
	size_t free_areas_file_len, read_bytes = 0;	/* Used in the for cycle */
	/* write_* variables are the ones that will be written */
	uint64_t read_offset, write_offset;
	uint32_t read_len, write_len;

	char before_found = 0, after_found = 0, empty_found = 0;
	uint64_t before_position, after_position, empty_position;
	uint64_t before_offset;
	uint32_t before_len, after_len;

	/* Compute "Free areas file" length */
	fseek(free_areas_file, 0, SEEK_END);
	free_areas_file_len = ftell(free_areas_file);

	/* Cycle through the file which contains the free areas offsets */
	fseek(free_areas_file, 0, SEEK_SET);
	for(read_bytes = 0; read_bytes < free_areas_file_len; read_bytes += FREE_AREAS_LINE_LEN){

		/* If area before, area after and a already deleted are found, then break */
		if(before_found == 1 && after_found == 1 && empty_found == 1)
			break;

		/* Read the offset of the free area */
		fread(&read_offset, 1, sizeof(uint64_t), free_areas_file);
		/* Read the length in bytes of the free area */
		fread(&read_len, 1, sizeof(uint32_t), free_areas_file);

		/* Check if the free area ends when message starts */
		if(before_found == 0 && read_offset + read_len == message_offset){
			/* Save the offset, the length and the position of the free area found */
			before_offset = read_offset;
			before_len = read_len;
			/* At which offset the free area is written in "Free areas file" */
			before_position = read_bytes;	
			before_found = 1;
			continue;
		}

		/* Check if the free are starts when message ends */
		if(after_found == 0 && message_offset + message_len == read_offset){
			/* Save the length and the position of the free area found */
			after_len = read_len;
			/* At which offset the free area is written in "Free areas file" */
			after_position = read_bytes;
			after_found = 1;
			continue;
		}

		/* Check if the free area is already deleted */
		if(read_offset == DELETED_OFFSET){
			/* Save the position of the free area found */
			/* At which offset the free area is written in "Free areas file" */
			empty_position = read_bytes;
			empty_found = 1;
			continue;
		}
	}

	/* No adjacent free areas found */
	if(before_found == 0 && after_found == 0){
		write_offset = message_offset;
		write_len = message_len;
		if(empty_found == 0){
			fseek(free_areas_file, 0, SEEK_END);
		} else {
			fseek(free_areas_file, empty_position, SEEK_SET);
		}
	}

	/* Only free area before found */
	else if(before_found == 1 && after_found == 0) {
		write_offset = before_offset;
		write_len = before_len + message_len;
		fseek(free_areas_file, before_position, SEEK_SET);
	}

	/* Only free area after found */
	else if(before_found == 0 && after_found == 1) {
		write_offset = message_offset;
		write_len = message_len + after_len;
		fseek(free_areas_file, after_position, SEEK_SET);
	}

	/* Both areas before and after found */
	else if(before_found == 1 && after_found == 1){
		/* Delete the free area after */
		fseek(free_areas_file, after_position, SEEK_SET);
		fwrite(&DELETED_OFFSET, 1, sizeof(uint64_t), free_areas_file);
		int empty_free_area_len = 0;
		fwrite(&empty_free_area_len, 1, sizeof(uint32_t), free_areas_file);

		write_offset = before_offset;
		write_len = before_len + message_len + after_len;
		fseek(free_areas_file, before_position, SEEK_SET);
	}

	/* #8: Update "free areas file" */
	fwrite(&write_offset, 1, sizeof(uint64_t), free_areas_file);
	fwrite(&write_len, 1, sizeof(uint32_t), free_areas_file);

	/* #9: Flush changes and close files */
	for(int i = 0; files[i] != NULL; i++){
		fsync(fileno(*files[i]));
	}

	close_delete();

	printf("Thread[%d]: deleted post #%d.\n", id, target_mid);

	/* #10: send operation OP OK */
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, NULL) < 0){
		printf("Thread[%d]: client unreachable.\n", id);
		return -1;
	}

	return 0;
}

// -----------------------------------

void close_files_and_buffers(){
	if(files != NULL){
		for(int i = 0; files[i] != NULL; i++)
			fclose(*files[i]);
	}
	
	if(buffers != NULL){
		for(int i = 0; buffers[i] != NULL; i++)
			free(*buffers[i]);
	}
}

void close_post(){
	/* Close files and free buffers */
	close_files_and_buffers();

	/* #9: signal(MR, MAX_THREAD) */
	short_semop(MR, MAX_THREAD);

	/* #10: signal(MW, 1) */
	short_semop(MW, 1);
}

void close_read_all(){
	/* Close files */
	buffers = NULL;
	close_files_and_buffers();

	/* #N-1: signal(MR, 1) */
	short_semop(MR, 1);
}

void close_delete(){

	/* Close files */
	buffers = NULL;
	close_files_and_buffers();

	/* #N-2: signal(MR, MAX_THREAD) */
	short_semop(MR, MAX_THREAD);

	/* #N-1: signal(MW, 1) */
	short_semop(MW, 1);
	
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
