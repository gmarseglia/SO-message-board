extern int UR;
extern int UW;
extern const int MAX_THREADS;
extern const char *users_filename;

int login_registration(int acceptfd, struct user_info* user_info);
int registration(FILE *users_file, int acceptfd, struct user_info *user_info);
int login(FILE *users_file, int acceptfd, struct user_info *user_info);

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

int registration(FILE *users_file, int acceptfd, struct user_info *user_info){
	// Local variables declaration
	int uid;
	char op;
	struct sembuf sem_op;
	sem_op.sem_num = 1;
	sem_op.sem_flg = 0;
	struct user_info *read_user_info;
	if((read_user_info = malloc(sizeof(struct user_info))) == NULL){
		perror("Error in registration on malloc");
		return -1;
	}

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

	// #4 Wait UR
	sem_op.sem_op = -1;
	semop(UR, &sem_op, 1);

	// #5 Signal UW
	sem_op.sem_op = 1;
	semop(UW, &sem_op, 1);

	// #6 Find username
	char found = 0;
	while(fscanf(users_file, "%d %ms %ms",
		&(read_user_info->uid), &(read_user_info->username), &(read_user_info->passwd)) != EOF){
		if(strcmp(user_info->username ,read_user_info->username) == 0){
			#ifdef PRINT_DEBUG
			printf("%s already found.\n", user_info->username);
			#endif
			found = 1;
			break;
		}
	}

	if(found == 1){	
		// #7a Signal UR
		sem_op.sem_op = 1;
		semop(UR, &sem_op, 1);

		// #8a Send OP_NOT_ACCEPTED to client
		send_message_to(acceptfd, 1, OP_NOT_ACCEPTED, "Username already existing.\n");

		// #9a
		return -1;
	}

	// #7b Wait UW
	sem_op.sem_op = -1;
	semop(UW, &sem_op, 1);

	// #8b Wait UR, MAX_THREADS - 1
	sem_op.sem_op = -(MAX_THREADS - 1);
	semop(UR, &sem_op, 1);

	sleep(5);

	// #9b Compute new UID
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

	// 10b Append on file
	fseek(users_file, 0, SEEK_END);
	fprintf(users_file, "%d %s %s\n",
		user_info->uid, user_info->username, user_info->passwd);
	// fsync(fileno(users_file));
	fclose(users_file);

	#ifdef PRINT_DEBUG
	printf("fync|fclose completed\n");
	#endif

	// #11b Signal UR, MAX_THREADS - 1
	sem_op.sem_op = (MAX_THREADS - 1);
	semop(UR, &sem_op, 1);

	// #12b Signal UW
	sem_op.sem_op = 1;
	semop(UW, &sem_op, 1);

	// #13b Send client OP_REG_UID with uid
	char uid_str[6];
	sprintf(uid_str, "%d", user_info->uid);
	send_message_to(acceptfd, UID_SERVER, OP_REG_UID, uid_str);

	// 14b go to main cycle
	return 0;
}

int login(FILE * users_file, int acceptfd, struct user_info *user_info){
	return 0;
}