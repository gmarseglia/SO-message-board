#include "client.h"
#include "../common/caesar-cipher.h"

#define ACTION_LOGIN 'L'
#define ACTION_REGISTER 'R'


/*
	DESCRIPTION:
		Fill with username and password client_ui
*/
void user_info_fill(user_info_t* client_ui);

// ---------------------------------------------------------------------------------------------
operation_t op;
char action_code;
char *action_name;

int login_registration(){
	while(1){
		// 'R' for registration, 'L' for login
		printf("\n(L)ogin or (R)egistration?\n");
		scanf("%c", &action_code);
		fflush(stdin);
		if(action_code == ACTION_LOGIN || action_code == ACTION_REGISTER) break;
		printf("Please type \'%c\'' or \'%c\'\n", ACTION_LOGIN, ACTION_REGISTER);	
	}
	action_name = (action_code == ACTION_LOGIN) ? "Login" : "Registration";

	// Ask the user to type username and passwd
	user_info_fill(&client_ui);
	op.text = calloc(sizeof(char), snprintf(op.text, 0, "%s %s", client_ui.username, client_ui.passwd));
	if(op.text == NULL) perror_and_failure("on op.text calloc()", __func__);
	snprintf(op.text, MAXSIZE_USERNAME + MAXSIZE_PASSWD, "%s %s", client_ui.username, client_ui.passwd);

	// Create the socket
	//	done here because on registration or login error, the socket gets closed
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) perror_and_failure("socket()", __func__);

	// Connect to server
	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) perror_and_failure("connect()", __func__);

	printf("Connected.\n");

	op.uid = UID_ANON;
	op.code = (action_code == ACTION_LOGIN) ? OP_LOG : OP_REG;

	if(send_operation_to_2(sockfd, op) < 0)
		return -1;
	free(op.text);

	printf("Waiting for server response.\n");

	if(receive_operation_from_2(sockfd, &op) < 0)
		return -1;

	if(op.uid == UID_SERVER && op.code == OP_OK){
		client_ui.uid = strtol(op.text, NULL, 10);
		free(op.text);
		printf("%s successful.\n", action_name);
		return 0;
	}

	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("%s unsuccessful: %s\n", action_name, op.text);
		free(op.text);
	} else {
		fprintf(stderr, "Unexpected error during %s.\nUID=%d, code=%c\n", action_name, op.uid, op.code);
		free(op.text);
	}

	return -1;
}


void user_info_fill(user_info_t* client_ui){
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