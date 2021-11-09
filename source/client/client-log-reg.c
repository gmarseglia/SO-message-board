#include "client.h"
#include "../common/caesar-cipher.h"

/*
	DESCRIPTION:
		Attempt to login
	RETURNS:
		0 in case of success
		-1 in case of error
*/
int login();

/*
	DESCRIPTION:
		Attempt to register
	RETURNS:
		0 in case of success
		-1 in case of error
*/
int registration();

/*
	DESCRIPTION:
		Fill with username and password client_ui
*/
void user_info_fill(user_info* client_ui);

// ---------------------------------------------------------------------------------------------

int login_registration(){
	char op;
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
	user_info_fill(&client_ui);

	// Create the socket
	//	done here because on registration or login error, the socket gets closed
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) perror_and_failure("socket()", __func__);

	// Connect to server
	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) perror_and_failure("connect()", __func__);

	printf("Connected.\n");

	// Call the function to register or to login
	if(op == 'R'){
		return registration();
	} else if (op == 'L'){
		return login();
	}

	return -1;
}

int registration(){
	operation op;

	if(send_message_to(sockfd, 0, OP_REG_USERNAME, client_ui.username) < 0)
		return -1;

	if(send_message_to(sockfd, 0, OP_REG_PASSWD, client_ui.passwd) < 0)
		return -1;
	
	if(receive_operation_from(sockfd, &op) < 0)
		return -1;

	if(op.uid == UID_SERVER && op.code == OP_REG_UID){
		client_ui.uid = strtol(op.text, NULL, 10);
		free(op.text);
		printf("Registration successful.\n");
		return 0;
	}
	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Registration unsuccessful: %s\n", op.text);
	} else {
		fprintf(stderr, "Unexpected error during registration.\nUID=%d, code=%c\n", op.uid, op.code);
	}
		if(close(sockfd) < 0) perror("Error in registration on close");
		return -1;
}

int login(){
	operation op;

	if(send_message_to(sockfd, UID_ANON, OP_LOG_USERNAME, client_ui.username) < 0)
		return -1;

	if(send_message_to(sockfd, UID_ANON, OP_LOG_PASSWD, client_ui.passwd) < 0)
		return -1;
	
	if(receive_operation_from(sockfd, &op) < 0)
		return -1;

	if(op.uid == UID_SERVER && op.code == OP_LOG_UID){
		client_ui.uid = strtol(op.text, NULL, 10);
		free(op.text);
		printf("Login successful.\n");
		return 0;
	}
	
	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Login unsuccessful: %s\n", op.text);
	} else {
		fprintf(stderr, "Unexpected error during login.\nUID=%d, code=%c\n", op.uid, op.code);
	}
	if(close(sockfd) < 0) perror("Error in login on close");
	return -1;
}

void user_info_fill(user_info* client_ui){
	const int amount = 3;
	const int increment = 14;
	char *clear_passwd;

	printf("Type username:\n");
	while(1){
		scanf("%ms", &(client_ui->username));
		if(client_ui->username == NULL)
			continue;
		if(strlen(client_ui->username) > MAXSIZE_USERNAME){
			printf("Username too long: max size is %d\n", MAXSIZE_USERNAME);
			continue;
		}
		fflush(stdin);
		break;
	}

	printf("Type password:\n");
	while(1){
		scanf("%ms", &clear_passwd);
		if(clear_passwd == NULL)
			continue;
		if(strlen(clear_passwd) > MAXSIZE_PASSWD){
			printf("Passord too long: max size is %d\n", MAXSIZE_PASSWD);
			continue;
		}
		fflush(stdin);
		client_ui->passwd = caesar_cipher_2(clear_passwd, amount, increment);
		break;
	}
	
	return;
}