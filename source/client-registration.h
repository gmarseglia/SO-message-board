struct user_info {
	char *username;
	char *passwd;
	int uid;
};

void login_registration(int sockfd, struct sockaddr_in *server_address, struct user_info *user_info);
void login(int sockfd, struct user_info *user_info);
int registration(int sockfd, struct user_info *user_info);
void user_info_fill(struct user_info* user_info);

char op;

void login_registration(int sockfd, struct sockaddr_in *server_address, struct user_info *user_info){
	while(1){
		printf("(L)ogin or (R)egistration?\n");
		scanf("%c", &op);
		fflush(stdin);
		if(!(op == 'R' || op == 'r' || op == 'L' || op == 'l')){
			printf("Please type \'R\' or \'L\' without \'\n");
			continue;
		}

		user_info_fill(user_info);

		if(connect(sockfd, (struct sockaddr *)server_address, sizeof(struct sockaddr)) < 0){
			perror("Error in client on connect");
			exit(EXIT_FAILURE);
		}

		printf("Connected.\n");

		if(op == 'R' || op == 'r'){
			op = 'R';
			if(registration(sockfd, user_info) < 0)
				continue;
			break;
		} else if (op == 'L' || op == 'l'){
			op = 'L';
			login(sockfd, user_info);
			break;
		}
	}
	return;
}

int registration(int sockfd, struct user_info *user_info){
	char *container;

	if(send_message_to(sockfd, 0, OP_SEND_REG_USERNAME, user_info->username) < 0)
		exit(EXIT_FAILURE);

	if(send_message_to(sockfd, 0, OP_SEND_REG_PASSWD, user_info->passwd) < 0)
		exit(EXIT_FAILURE);
	
	if(receive_message_from(sockfd, NULL, &op, &container) < 0)
		exit(EXIT_FAILURE);

	if(op == 'i'){
		user_info->uid = strtol(container, NULL, 10);
		printf("Registration successful.\n");
		return 0;
	} else if(op == 'n'){
		printf("Registration unsuccessful: %s\n", container);
		return -1;
	}
	return -1;
}

void login(int sockfd, struct user_info *user_info){
	return;
}

void user_info_fill(struct user_info* user_info){
	printf("Type username:\n");
	scanf("%ms", &(user_info->username));
	fflush(stdin);
	printf("Type passwd:\n");
	scanf("%ms", &(user_info->passwd));
	fflush(stdin);
	return;
}