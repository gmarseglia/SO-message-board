#include "server-routines.h"

int return_save(int return_value, FILE ***files, char ***buffers);

int save(int acceptfd, user_info *client_ui, operation *op){
	char *subject = op->text, *body = NULL;
	char **buffers[2] = {&subject, &body};

	FILE *messages_file, *index_file,*free_areas_file;
	messages_file = index_file = free_areas_file = NULL;
	FILE **files[3] = {&messages_file, &index_file, &free_areas_file};

	struct sembuf actual_sem_op = {.sem_flg = 0, .sem_num = 0};

	int mid;
	long index_file_len;

	uint32_t message_len, uid = (uint32_t)client_ui->uid;
	uint64_t message_offset;

	// Check UID match
	if(client_ui->uid != op->uid){
		printf("client_ui->uid != op->uid, client_ui->uid=%d op->uid=%d\n", client_ui->uid, op->uid);
		return -1;
	}

	// #1: Receive (client_uid, OP_MSG_BODY, body)
	if(receive_operation_from(acceptfd, op) < 0)
		return -1;
	body = op->text;

	message_len = strlen(subject) + 1 + strlen(body) + 1;

	#ifdef PRINT_DEBUG
	printf("BEGIN%s\n(%s, %d) sent:\n%s\n%s\n%s\n%s\n%sEND\n", SEP,
		client_ui->username, client_ui->uid, SEP,
		subject, SEP,
		body, SEP);
	#endif

	// #2: Open "Messages file", "Index file", "Free areas file"
	messages_file = fdopen(open(MESSAGES_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	index_file = fdopen(open(INDEX_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	free_areas_file = fdopen(open(FREE_AREAS_FILENAME, O_CREAT | O_RDWR, 0660), "r+");
	if(messages_file == NULL || index_file == NULL || free_areas_file == NULL){
		perror("Error in server-save on fdopen");
		exit_failure();
	}

	// #3: Wait MW(1)
	actual_sem_op.sem_op = -1;
	semop(MW, &actual_sem_op, 1);

	// #4: Wait MR(MAX_THREAD)
	actual_sem_op.sem_op = -MAX_THREAD;
	semop(MR, &actual_sem_op, 1);

	// #5: Compute MID, Message ID
	fseek(index_file, 0, SEEK_END);
	index_file_len = ftell(index_file);

	mid = index_file_len / INDEX_LINE_LEN;

	#ifdef PRINT_DEBUG_FINE
	printf("index_file_len = %ld, mid = %d\n", index_file_len, mid);
	#endif

	#ifndef SKIP_FREE_AREAS
	// #6: Search for free memory area in "Free file"
	#endif

	// #7: Write on "Index file" at MID offset: (Message offset, Message len, Sender UID)
	fseek(messages_file, 0, SEEK_END);
	message_offset = ftell(messages_file);
	fwrite(&message_offset, 1, sizeof(uint64_t), index_file);
	fwrite(&message_len, 1, sizeof(uint32_t), index_file);
	fwrite(&uid, 1, sizeof(uint32_t), index_file);

	// #8: Write on "Message file" at Message offset: (Subject , '\0', Body, '\0')
	fwrite(subject, 1, strlen(subject), messages_file);
	fwrite("\n", 1, sizeof(char), messages_file);
	fwrite(body, 1, strlen(body), messages_file);
	fwrite("\n", 1, sizeof(char), messages_file);

	// #9: Signal MR(MAX_THREAD)
	actual_sem_op.sem_op = MAX_THREAD;
	semop(MR, &actual_sem_op, 1);

	// #10: Signal MW(1)
	actual_sem_op.sem_op = 1;
	semop(MW, &actual_sem_op, 1);

	// #11: Send (SERVER_UID, OP_OK, MID)
	char mid_str[32];
	sprintf(mid_str, "%d", mid);
	if(send_message_to(acceptfd, UID_SERVER, OP_OK, mid_str) < 0)
		return return_save(-1, files, buffers);

	return return_save(0, files, buffers);
}

int return_save(int return_value, FILE ***files, char ***buffers){
	for(int i = 0; i < 3; i++){
		fclose(*files[i]);
	}
	for(int i = 0; i < 2; i++){
		free(*buffers[i]);
	}
	return return_value;
}