#include "server-routines.h"

int registration(FILE *users_file, int acceptfd, user_info *client_ui);
int login(FILE *users_file, int acceptfd, user_info *client_ui);
user_info *find_user(FILE *users_file, char *username);


int login_registration(int acceptfd, user_info* client_ui){
	int read_uid;
	char read_op;

	// Open Users file
	FILE *users_file = fdopen(open(USERS_FILENAME, O_CREAT|O_RDWR, 0660), "r+");
	if(users_file == NULL){
		perror("Error in thread_communication_routine on fread_open");
		exit(EXIT_FAILURE);
	}

	if(receive_message_from(acceptfd, &read_uid, &read_op, &(client_ui->username)) < 0)
		return -1;

	if(read_uid == UID_ANON && read_op == OP_REG_USERNAME)
		return registration(users_file, acceptfd, client_ui);
	else if(read_uid == UID_ANON && read_op == OP_LOG_USERNAME)
		return login(users_file, acceptfd, client_ui);
	else{
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect first operation. Expected OP_LOG_USERNAME or OP_REG_USERNAME");
		return -1;
	}
}

/*
	DESCRIPTION:
		documentation/Server-Registration.jpeg
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int registration(FILE *users_file, int acceptfd, user_info *client_ui){
	// Local variables declaration
	int read_uid;
	char read_op;
	struct sembuf sem_op = {.sem_num = 0, .sem_flg = 0};
	user_info *read_ui;

	// #2 Receive passwd
	if(receive_message_from(acceptfd, &read_uid, &read_op, &(client_ui->passwd)) < 0)
		return -1;

	// If not correct read_op OP_REG_PASSWD send read_op OP_NOT_ACCEPTED
	if(!(read_uid == UID_ANON && read_op == OP_REG_PASSWD)){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect second operation. Expected OP_REG_PASSWD");
		return -1;
	}

	// #3 Wait UW
	sem_op.sem_op = -1;
	semop(UW, &sem_op, 1);

	// #4 Wait UR N
	sem_op.sem_op = -MAX_THREAD;
	semop(UR, &sem_op, 1);

	// #5 Find username
	read_ui = find_user(users_file, client_ui->username);

	if(read_ui != NULL){	
		// #6a Signal UR N
		sem_op.sem_op = MAX_THREAD;
		semop(UR, &sem_op, 1);

		// #7a Signal UW
		sem_op.sem_op = 1;
		semop(UW, &sem_op, 1);

		// #8a Send OP_NOT_ACCEPTED to client
		send_message_to(acceptfd, 1, OP_NOT_ACCEPTED, "Username already existing.");

		// #9a
		return -1;
	}
	
	// #6b Compute new UID
	//	At the moment writes and reads are exclusive to this thread
	int last_uid;
	int file_len;
	fseek(users_file, 0, SEEK_END);
	file_len = ftell(users_file);

	// Find last UID
	if(file_len == 0){
		last_uid = INITIAL_UID;
	} else {
		int max_line_len = MAXSIZE_USERNAME + MAXSIZE_PASSWD + 5 + 3 + 1;
		char line_buffer[max_line_len + 1];
		memset(line_buffer, '\0', max_line_len + 1);

		fseek(users_file, file_len > max_line_len ? -max_line_len : -file_len, SEEK_END);
		while(fgets(line_buffer, max_line_len, users_file) != NULL);
		sscanf(line_buffer, "%d", &last_uid);
	}

	client_ui->uid = last_uid + 1;

	#ifdef PRINT_DEBUG_FINE
	printf("last_uid=%d, client_ui->uid=%d\n", last_uid, client_ui->uid);
	#endif

	// 7b Append on file
	fseek(users_file, 0, SEEK_END);
	fprintf(users_file, "%d %s %s\n",
		client_ui->uid, client_ui->username, client_ui->passwd);
	fclose(users_file);

	// #8b Signal UR, MAX_THREAD
	sem_op.sem_op = MAX_THREAD;
	semop(UR, &sem_op, 1);

	// #9b Signal UW
	sem_op.sem_op = 1;
	semop(UW, &sem_op, 1);

	// #10b Send client OP_REG_UID with uid
	char uid_str[6];
	sprintf(uid_str, "%d", client_ui->uid);
	if(send_message_to(acceptfd, UID_SERVER, OP_REG_UID, uid_str) < 0)
		return -1;

	// 11b go to main cycle
	return 0;
}

/*
	DESCRIPTION:
		documentation/Client-Registration.png
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int login(FILE * users_file, int acceptfd, user_info *client_ui){
	int read_uid;
	char read_op;
	struct sembuf sem_op = {.sem_num = 0, .sem_flg = 0};
	user_info *read_ui;

	// #1: Receive passwd
	if(receive_message_from(acceptfd, &read_uid, &read_op, &(client_ui->passwd)) < 0)
		return -1;

	// #2a: op incorrect -> send OP_NOT_ACCEPTED
	if(!(read_uid == UID_ANON && read_op == OP_LOG_PASSWD)){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect OP, expected OP_LOG_PASSWD");
		return -1;
	}

	// #2: wait(UW, 1)
	sem_op.sem_op = -1;
	semop(UW, &sem_op, 1);

	// #3: wait(UR, 1);
	sem_op.sem_op = -1;
	semop(UR, &sem_op, 1);

	// #4: signal(UW, 1)
	sem_op.sem_op = 1;
	semop(UW, &sem_op, 1);

	// #5: search for username and fill user info
	read_ui = find_user(users_file, client_ui->username);

	// #6: Signal (UR, 1)
	sem_op.sem_op = 1;
	semop(UR, &sem_op, 1);

	// #7: Close users_file
	fclose(users_file);

	// #8a: If username ! found -> send OP_NOT_ACCEPTED
	if(read_ui == NULL){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Username not found");
		return -1;
	}

	// #8: Compare password
	if(strcmp(client_ui->passwd, read_ui->passwd) != 0){
		//	9a: If passwords don't match -> Send OP_NOT_ACCEPTED
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Passwords don't match");
		return -1;
	}

	// #9: Send OP_LOG_UID with stored UID
	char uid_str[6];
	sprintf(uid_str, "%d", read_ui->uid);
	if(send_message_to(acceptfd, UID_SERVER, OP_LOG_UID, uid_str) < 0)
		return -1;
	// Save UID in client_ui
	client_ui->uid = read_ui->uid;

	// #10: return to main cycle
	return 0;
}

/*
	DESCRIPTION:
		Search for entry that matches username
	RETURNS:
		In case of match found: new allocated struct user_info, filled with info read from users file
		In case of match NOT found: NULL
*/
user_info *find_user(FILE *users_file, char *username){
	user_info *read_ui = malloc(sizeof(user_info));
	if(read_ui == NULL){
		fprintf(stderr, "Error in find_user on malloc\n");
		return NULL;
	}

	while(fscanf(users_file, "%d %ms %ms",
		&(read_ui->uid), &(read_ui->username), &(read_ui->passwd)) != EOF){
		if(strcmp(username ,read_ui->username) == 0)
			return read_ui;
	}

	free(read_ui);
	return NULL;
}