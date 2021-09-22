extern int UR;
extern int UW;
extern FILE *users_file;

int login_registration(int acceptfd, struct user_info* user_info);
int registration(int acceptfd, struct user_info *user_info);
int login(int acceptfd, struct user_info *user_info);

int login_registration(int acceptfd, struct user_info* user_info){
	int uid;
	char op;

	if(receive_message_from(acceptfd, &uid, &op, &(user_info->username)) < 0)
		return -1;

	if(uid == 0 && op == OP_REG_USERNAME)
		return registration(acceptfd, user_info);
	else if(uid == 0 && op == OP_LOG_USERNAME)
		return login(acceptfd, user_info);
	else{
		send_message_to(acceptfd, 1, OP_NOT_ACCEPTED, "Incorrect first op");
		return -1;
	}

}

int registration(int acceptfd, struct user_info *user_info){
	int uid;
	char op;
	struct sembuf sem_op;
	sem_op.sem_num = 1;
	sem_op.sem_flg = 0;
	// int buffer_size = SIZEOF_INT + MAXSIZE_USERNAME + MAXSIZE_PASSWD + 1;
	// char buffer[buffer_size];
	// memset(buffer, '\0', buffer_size);
	struct user_info read_user_info;

	// #2 Receive passwd
	if(receive_message_from(acceptfd, &uid, &op, &(user_info->passwd)) < 0)
		return -1;

	// If not correct op OP_REG_PASSWD send op OP_NOT_ACCEPTED
	if(!(uid == 0 && op == OP_REG_PASSWD)){
		send_message_to(acceptfd, 1, OP_NOT_ACCEPTED, "Incorrect second op");
		return -1;
	}

	// #3 Wait UW
	sem_op.sem_op = -1;
	semop(UW, &sem_op, 1);

	// #4 Wait UR
	semop(UR, &sem_op, 1);

	// #5 Signal UW
	sem_op.sem_op = 1;
	semop(UW, &sem_op, 1);

	// #6 Find username
	while(fscanf(users_file, "%d %ms %ms",
		&(read_user_info.uid), &(read_user_info.username), &(read_user_info.passwd)) != EOF);

	return 0;
}

int login(int acceptfd, struct user_info *user_info){
	return 0;
}