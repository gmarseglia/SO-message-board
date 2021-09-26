int login_registration(int *sockfd, struct sockaddr_in *server_address, struct user_info *user_info);
int login(int sockfd, struct user_info *user_info);
int registration(int sockfd, struct user_info *user_info);
void user_info_fill(struct user_info* user_info);

char op;

/*
	DESCRIPTION:
		Ask if user wants to login or register
		Fill user's info
		Create the socket
		Connect to server
		Call function for login or register
*/
int login_registration(int *sockfd, struct sockaddr_in *server_address, struct user_info *user_info){
	while(1){
		// 'R' or 'r' for registration, 'L' or 'l' for login
		printf("\n(L)ogin or (R)egistration?\n");
		scanf("%c", &op);
		fflush(stdin);
		if(op == 'R' || op == 'L')
			break;
		printf("Please type \'R\' or \'L\'\n");	
	}

	// Ask the user to type username and passwd
	user_info_fill(user_info);

	// Create the socket
	//	done here because on registration or login error, the socket gets closed
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(*sockfd < 0){
		perror("Error in client on socket attempt.\n");
		exit(EXIT_FAILURE);
	}

	// Connect to server
	if(connect(*sockfd, (struct sockaddr *)server_address, sizeof(struct sockaddr)) < 0){
		perror("Error in client on connect");
		exit(EXIT_FAILURE);
	}

	printf("Connected.\n");

	// Call the function to register or to login
	if(op == 'R'){
		return registration(*sockfd, user_info);
	} else if (op == 'L'){
		return login(*sockfd, user_info);
	}

	return -1;
}

/*
	DESCRIPTION:
		Send to server 0,"u",username
		Send to server 0,"p",passwd
		Receive from server "i",uid in case of success, or "n",message in case of error
	RETURNS:
		In case of success: 0
		In case of error: -1
*/
int registration(int sockfd, struct user_info *user_info){
	char *container;
	int read_uid;

	if(send_message_to(sockfd, 0, OP_REG_USERNAME, user_info->username) < 0)
		exit(EXIT_FAILURE);

	if(send_message_to(sockfd, 0, OP_REG_PASSWD, user_info->passwd) < 0)
		exit(EXIT_FAILURE);
	
	if(receive_message_from(sockfd, &read_uid, &op, &container) < 0)
		exit(EXIT_FAILURE);

	if(read_uid == UID_SERVER && op == OP_REG_UID){
		user_info->uid = strtol(container, NULL, 10);
		printf("Registration successful.\n");
		return 0;
	} else if(read_uid == UID_SERVER && op == OP_NOT_ACCEPTED){
		printf("Registration unsuccessful: %s\n", container);
		if(close(sockfd) < 0) perror("Error in registration on close");
		return -1;
	} else {
		fprintf(stderr, "Unexpected error during registration.\nUID=%d, op=%c\n", read_uid, op);
		exit(EXIT_FAILURE);
	}
}

/*
	DESCRIPTION:
		Send to server (UID_ANON, OP_LOG_USERNAME, username)
		Send to server (UID_ANON, OP_LOG_PASSWD, passwd)
		Receive from server (UID_SERVER, OP, UID in case of success | Messagge in case of insuccess)
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int login(int sockfd, struct user_info *user_info){
	char *container;
	int read_uid;

	if(send_message_to(sockfd, 0, OP_LOG_USERNAME, user_info->username) < 0)
		exit(EXIT_FAILURE);

	if(send_message_to(sockfd, 0, OP_LOG_PASSWD, user_info->passwd) < 0)
		exit(EXIT_FAILURE);
	
	if(receive_message_from(sockfd, &read_uid, &op, &container) < 0)
		exit(EXIT_FAILURE);

	if(read_uid == UID_SERVER && op == OP_LOG_UID){
		user_info->uid = strtol(container, NULL, 10);
		printf("Login successful.\n");
		return 0;
	} else if(read_uid == UID_SERVER && op == OP_NOT_ACCEPTED){
		printf("Login unsuccessful: %s\n", container);
		if(close(sockfd) < 0) perror("Error in login on close");
		return -1;
	} else {
		fprintf(stderr, "Unexpected error during login.\nUID=%d, op=%c\n", read_uid, op);
		exit(EXIT_FAILURE);
	}
}

/*
	DESCRIPTION:
		Ask the user to type username and password
		Fill user_info
*/
void user_info_fill(struct user_info* user_info){
	printf("Type username:\n");
	do{
		scanf("%ms", &(user_info->username));
		if(user_info->username == NULL)
			continue;
		if(strlen(user_info->username) > MAXSIZE_USERNAME){
			printf("Username too long: max size is %d\n", MAXSIZE_USERNAME);
			continue;
		}
		fflush(stdin);
		break;
	} while(1);

	printf("Type password:\n");
	do{
		scanf("%ms", &(user_info->passwd));
		if(user_info->passwd == NULL)
			continue;
		if(strlen(user_info->passwd) > MAXSIZE_PASSWD){
			printf("Passord too long: max size is %d\n", MAXSIZE_PASSWD);
			continue;
		}
		fflush(stdin);
		break;
	} while(1);
	
	return;
}