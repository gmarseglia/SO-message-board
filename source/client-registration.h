// Struct containing the user info
struct user_info {
	char *username;
	char *passwd;
	int uid;
};

void login_registration(int *sockfd, struct sockaddr_in *server_address, struct user_info *user_info);
void login(int sockfd, struct user_info *user_info);
int registration(int sockfd, struct user_info *user_info);
void user_info_fill(struct user_info* user_info);

char op;

/*
	DESCRIPTION:
		Ask if user wants to login or register
		Create the socket
		Connect to server
		Call function for login or register
*/
void login_registration(int *sockfd, struct sockaddr_in *server_address, struct user_info *user_info){
	while(1){
		// 'R' or 'r' for registration, 'L' or 'l' for login
		printf("(L)ogin or (R)egistration?\n");
		scanf("%c", &op);
		fflush(stdin);
		if(!(op == 'R' || op == 'r' || op == 'L' || op == 'l')){
			printf("Please type \'R\' or \'L\' without \'\n");
			continue;
		}

		// Create the socket
		//	done here because on registration or login error, the socket gets closed
		*sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(*sockfd < 0){
			perror("Error in client on socket attempt.\n");
			exit(EXIT_FAILURE);
		}

		// Ask the user to type username and passwd
		user_info_fill(user_info);

		// Connect to server
		if(connect(*sockfd, (struct sockaddr *)server_address, sizeof(struct sockaddr)) < 0){
			perror("Error in client on connect");
			exit(EXIT_FAILURE);
		}

		printf("Connected.\n");

		// Call the function to register or to login
		if(op == 'R' || op == 'r'){
			op = 'R';
			if(registration(*sockfd, user_info) < 0)
				continue;
			break;
		} else if (op == 'L' || op == 'l'){
			op = 'L';
			login(*sockfd, user_info);
			break;
		}
	}
	return;
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
		if(close(sockfd) < 0) perror("Error in registration on close");
		return -1;
	} else {
		fprintf(stderr, "Unexpected error during registration.\n");
		exit(EXIT_FAILURE);
	}
}

void login(int sockfd, struct user_info *user_info){
	return;
}

/*
	DESCRIPTION:
		Ask the user to type username and password
*/
void user_info_fill(struct user_info* user_info){
	printf("Type username:\n");
	scanf("%ms", &(user_info->username));
	fflush(stdin);
	printf("Type passwd:\n");
	scanf("%ms", &(user_info->passwd));
	fflush(stdin);
	return;
}