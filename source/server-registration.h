int login_registration(int acceptfd, struct user_info* user_info);
int registration(int acceptfd, struct user_info *user_info);
int login(int acceptfd, struct user_info *user_info);

int login_registration(int acceptfd, struct user_info* user_info){
	int uid;
	char op;

	if(receive_message_from(acceptfd, &uid, &op, &(user_info->username)) < 0)
		return -1;

	if(uid == 0 && op == OP_REG_USERNAME)
		return registration();
	else if(uid == 0 && op == OP_LOG_USERNAME)
		return login();
	else{
		send_message_to(acceptfd, 1, OP_NOT_ACCEPTED, "Incorrect first op");
		return -1;
	}

}

int registration(int acceptfd, struct user_info *user_info){
	int uid;
	char op;

	if(receive_message_from(acceptfd, &uid, &op, &(user_info->passwd)) < 0)
		return -1;

	if(!(uid == 0 && op == OP_REG_PASSWD))
		return -1;

	// CONTINUE FROM 3
}