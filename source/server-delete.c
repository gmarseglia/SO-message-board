#include "server.h"

int close_delete(int return_value, FILE ***files, char semaphores);

int delete_post(int acceptfd, user_info client_ui, operation *op){
	int target_mid;

	uint32_t message_len, read_uid;
	uint64_t message_offset;

	FILE *messages_file, *index_file, *free_areas_file;
	messages_file = index_file = free_areas_file = NULL;
	FILE **files[] = {&messages_file, &index_file, &free_areas_file, NULL};


	// #1: Read target MID
	sscanf(op->text, "%d", &target_mid);
	free(op->text);

	printf("(%s, %d): deletion of post #%d requested.\n", client_ui.username, client_ui.uid, target_mid);
	
	// #2: Open "Messages file", "Index file", "Free areas file"
	messages_file = fdopen(open(MESSAGES_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	index_file = fdopen(open(INDEX_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	free_areas_file = fdopen(open(FREE_AREAS_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	if(messages_file == NULL || index_file == NULL || free_areas_file == NULL){
		perror("Error in server-save on fdopen");
		exit_failure();
	}

	// #3: Check target MID in bound
	fseek(index_file, 0, SEEK_END);
	if(target_mid >= ftell(index_file) / INDEX_LINE_LEN){
		return close_delete(send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Post doesn't exist."),
			files, 0);
	}

	// #4: Wait MW(1)
	short_semop(MW, -1);

	// #5: Wait MR(MAX_THREAD)
	short_semop(MR, -MAX_THREAD);

	// #6: Read info from "index file"
	fseek(index_file, target_mid * INDEX_LINE_LEN, SEEK_SET);
	fread(&message_offset, 1, sizeof(uint64_t), index_file);
	fread(&message_len, 1, sizeof(uint32_t), index_file);
	fread(&read_uid, 1, sizeof(uint32_t), index_file);

	// #7: check UID match
	if(op->uid != read_uid){
		return close_delete(send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "UID don't match."), 
		files, 1);
	}

	printf("(%s, %d): deletion post #%d accepted.\n",
		client_ui.username, client_ui.uid, target_mid);

	return close_delete(send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Deletion not implemented."),
	 files, 1);
}

int close_delete(int return_value, FILE ***files, char semaphores){
	for(int i = 0; files[i] != NULL; i++)
		fclose(*files[i]);

	if(semaphores == 1){
		short_semop(MR, MAX_THREAD);
		short_semop(MW, 1);
	}

	return return_value;
}