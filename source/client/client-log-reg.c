#include "client.h"
#include "../common/caesar-cipher.h"

#define ACTION_LOGIN 'L'
#define ACTION_REGISTER 'R'

/*
**	DESCRIPTION
**		Fill with username and password client_ui
**		checks for max length
*/
void user_info_fill(user_info_t* client_ui);

// ---------------------------------------------------------------------------------------------
operation_t op;
char action_code;
char *action_name;

int login_registration(){

	/* Block signals while sending operation */
	pthread_sigmask(SIG_BLOCK, &set_sigint, NULL);

	/* #1: Send operation to start handshake
		Send OP_OK to start handshake */
	if(send_operation_to(sockfd, UID_ANON, OP_OK, NULL) < 0)
		return -1;

	/* Unlock signals after send */
	pthread_sigmask(SIG_UNBLOCK, &set_sigint, NULL);

	printf("Waiting in queue for free server slot...\n");

	/* #2: Wait server response to end handshake
		(UID_SERVER, OP_OK, *) is required */
	if(receive_operation_from_2(sockfd, &op) < 0
		|| op.code != OP_OK || op.uid != UID_SERVER){
		printf("Could not complete handshake.\n\n");
		/* Login or Registration failed */
		return -1;
	}

	printf("Handshake complete.\n\n");

	/* #3: Ask user if Login or Registration */
	while(1){
		// 'R' for registration, 'L' for login
		printf("(L)ogin or (R)egistration?\n");
		scanf("%c", &action_code);
		fflush(stdin);
		if(action_code == ACTION_LOGIN || action_code == ACTION_REGISTER) break;
		printf("Please type \'%c\'' or \'%c\'\n", ACTION_LOGIN, ACTION_REGISTER);	
	}
	action_name = (action_code == ACTION_LOGIN) ? "Login" : "Registration";

	/* #4: Ask user info */
	user_info_fill(&client_ui);

	/* Put username and password into a single string*/
	op.text = calloc(sizeof(char), snprintf(op.text, 0, "%s %s", client_ui.username, client_ui.passwd));
	if(op.text == NULL) perror_and_failure("on op.text calloc()", __func__);
	snprintf(op.text, MAXSIZE_USERNAME + MAXSIZE_PASSWD, "%s %s", client_ui.username, client_ui.passwd);

	op.uid = UID_ANON;
	op.code = (action_code == ACTION_LOGIN) ? OP_LOG : OP_REG;

	/* Pre-send block */
	pthread_sigmask(SIG_BLOCK, &set_sigint, NULL);

	/* #5: Send operation with user info
		(UID_ANON, OP_LOG | OP_REG, "username passwd") */
	if(send_operation_to_2(sockfd, op) < 0)
		return -1;

	free(op.text);

	/* After-send unlock */
	pthread_sigmask(SIG_UNBLOCK, &set_sigint, NULL);
	
	printf("Waiting for server response...\n");

	/* #6: Receive response */
	if(receive_operation_from_2(sockfd, &op) < 0)
		return -1;

	/* If (UID_SERVER, OP_OK, *) then Login or Registration are successful */
	if(op.uid == UID_SERVER && op.code == OP_OK){
		client_ui.uid = strtol(op.text, NULL, 10);
		free(op.text);
		printf("%s successful.\n\n", action_name);
		return 0;
	}

	/* If (UID_SERVER, OP_NOT_ACCEPTED, *) then login or registration are unsuccessful
		and the explenation is in the operation text */
	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("%s unsuccessful: %s\n\n", action_name, op.text);
		free(op.text);
	} else {	/* Unexpected UID or code */
		fprintf(stderr, "Unexpected error during %s.\nUID=%d, code=%c\n\n", action_name, op.uid, op.code);
		free(op.text);
	}

	/* Login or Registration failed */
	return -1;
}


void user_info_fill(user_info_t* client_ui){
	const int amount = 3;
	const int increment = 14;
	char *clear_passwd;

	/* Asks user to type username */
	while(1){
		printf("Type username:\n");
		scanf("%ms", &(client_ui->username));
		if(client_ui->username == NULL)
			continue;
		if(strlen(client_ui->username) > MAXSIZE_USERNAME){	/* Max length check */
			printf("Username too long: max size is %d\n", MAXSIZE_USERNAME);
			free(client_ui->username);
			continue;
		}
		fflush(stdin);
		break;
	}

	/* Asks user to type passwd */
	while(1){
		printf("Type password:\n");
		scanf("%ms", &clear_passwd);
		if(clear_passwd == NULL)
			continue;
		if(strlen(clear_passwd) > MAXSIZE_PASSWD){
			printf("Passord too long: max size is %d\n", MAXSIZE_PASSWD);
			free(clear_passwd);
			continue;
		}
		fflush(stdin);
		/* Encrypt passwd */
		client_ui->passwd = caesar_cipher_2(clear_passwd, amount, increment);
		break;
	}
	
	return;
}