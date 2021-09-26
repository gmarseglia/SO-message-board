extern int UR;
extern int UW;
extern const int MAX_THREADS;
extern const char *users_filename;

int login_registration(int acceptfd, struct user_info* user_info);
int registration(FILE *users_file, int acceptfd, struct user_info *user_info);
int login(FILE *users_file, int acceptfd, struct user_info *user_info);
struct user_info *find_user(FILE *users_file, char *username);

/*
	DESCRIPTION:
		Ask user R or L, for Registration or Login
		Call correct function
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int login_registration(int acceptfd, struct user_info* user_info){
	int uid;
	char op;

	// Open Users file
	FILE *users_file = fopen(users_filename, "r+");
	if(users_file == NULL){
		perror("Error in thread_communication_routine on fopen");
		exit(EXIT_FAILURE);
	}

	if(receive_message_from(acceptfd, &uid, &op, &(user_info->username)) < 0)
		return -1;

	if(uid == 0 && op == OP_REG_USERNAME)
		return registration(users_file, acceptfd, user_info);
	else if(uid == 0 && op == OP_LOG_USERNAME)
		return login(users_file, acceptfd, user_info);
	else{
		send_message_to(acceptfd, 1, OP_NOT_ACCEPTED, "Incorrect first op");
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
int registration(FILE *users_file, int acceptfd, struct user_info *user_info){
	// Local variables declaration
	int uid;
	char op;
	struct sembuf sem_op = {.sem_num = 0, .sem_flg = 0};
	struct user_info *read_user_info;

	// #2 Receive passwd
	if(receive_message_from(acceptfd, &uid, &op, &(user_info->passwd)) < 0)
		return -1;

	// If not correct op OP_REG_PASSWD send op OP_NOT_ACCEPTED
	if(!(uid == UID_ANON && op == OP_REG_PASSWD)){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect second op");
		return -1;
	}

	// #3 Wait UW
	sem_op.sem_op = -1;
	semop(UW, &sem_op, 1);

	// #4 Wait UR N
	sem_op.sem_op = -MAX_THREADS;
	semop(UR, &sem_op, 1);

	// #5 Find username
	read_user_info = find_user(users_file, user_info->username);

	if(read_user_info != NULL){	
		// #6a Signal UR N
		sem_op.sem_op = MAX_THREADS;
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
	int max_line_len = MAXSIZE_USERNAME + MAXSIZE_PASSWD + 5 + 3 + 1;
	int last_uid;
	char line_buffer[max_line_len + 1];
	memset(line_buffer, '\0', max_line_len + 1);

	fseek(users_file, -max_line_len, SEEK_END);
	while(fgets(line_buffer, max_line_len, users_file) != NULL);
	sscanf(line_buffer, "%d", &last_uid);
	user_info->uid = last_uid + 1;

	#ifdef PRINT_DEBUG_FINE
	printf("last_uid=%d, user_info->uid=%d\n", last_uid, user_info->uid);
	#endif

	// 7b Append on file
	fseek(users_file, 0, SEEK_END);
	fprintf(users_file, "%d %s %s\n",
		user_info->uid, user_info->username, user_info->passwd);
	fclose(users_file);

	// #8b Signal UR, MAX_THREADS
	sem_op.sem_op = MAX_THREADS;
	semop(UR, &sem_op, 1);

	// #9b Signal UW
	sem_op.sem_op = 1;
	semop(UW, &sem_op, 1);

	// #10b Send client OP_REG_UID with uid
	char uid_str[6];
	sprintf(uid_str, "%d", user_info->uid);
	send_message_to(acceptfd, UID_SERVER, OP_REG_UID, uid_str);

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
int login(FILE * users_file, int acceptfd, struct user_info *user_info){
	int read_uid;
	char read_op;
	struct sembuf sem_op = {.sem_num = 0, .sem_flg = 0};
	struct user_info *read_user_info;

	// #1: Receive passwd
	if(receive_message_from(acceptfd, &read_uid, &read_op, &(user_info->passwd)) < 0)
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
	read_user_info = find_user(users_file, user_info->username);

	// #6: Signal (UR, 1)
	sem_op.sem_op = 1;
	semop(UR, &sem_op, 1);

	// #7: Close users_file
	fclose(users_file);

	// #8a: If username ! found -> send OP_NOT_ACCEPTED
	if(read_user_info == NULL){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Username not found");
		return -1;
	}

	// #8: Compare password
	if(strcmp(user_info->passwd, read_user_info->passwd) != 0){
		//	9a: If passwords don't match -> Send OP_NOT_ACCEPTED
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Passwords don't match");
		return -1;
	}

	// #9: Send OP_LOG_UID with stored UID
	char uid_str[6];
	sprintf(uid_str, "%d", read_user_info->uid);
	send_message_to(acceptfd, UID_SERVER, OP_LOG_UID, uid_str);

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
struct user_info *find_user(FILE *users_file, char *username){
	struct user_info *read_user_info = malloc(sizeof(struct user_info));
	if(read_user_info == NULL){
		fprintf(stderr, "Error in find_user on malloc\n");
		return NULL;
	}

	while(fscanf(users_file, "%d %ms %ms",
		&(read_user_info->uid), &(read_user_info->username), &(read_user_info->passwd)) != EOF){
		if(strcmp(username ,read_user_info->username) == 0)
			return read_user_info;
	}

	free(read_user_info);
	return NULL;
}