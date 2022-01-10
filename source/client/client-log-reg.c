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

	// Allow SIGINT
	pthread_sigmask(SIG_SETMASK, &sigset_sigint_allowed, NULL);

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

	/* Once the user has typed username and password, signals are blocked */
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	/* Put username and password into a single string*/
	op.text = calloc(sizeof(char), snprintf(op.text, 0, "%s %s", client_ui.username, client_ui.passwd));
	if(op.text == NULL) perror_and_failure("on op.text calloc()", __func__);
	snprintf(op.text, MAXSIZE_USERNAME + MAXSIZE_PASSWD, "%s %s", client_ui.username, client_ui.passwd);

	op.uid = UID_ANON;
	op.code = (action_code == ACTION_LOGIN) ? OP_LOG : OP_REG;

	/* Send the operation for login or registration to server */
	if(send_operation_to_2(sockfd, op) < 0)
		return -1;
	free(op.text);

	// Allow SIGINT
	pthread_sigmask(SIG_SETMASK, &sigset_sigint_allowed, NULL);

	printf("Waiting for server response.\n");

	/* Awaits response from server
		Signals are allowed, so user can close the client */
	int receive_return = receive_operation_from_2(sockfd, &op);

	/* Once the response is arrived, the signals are blocked again
		to ensure the correct completion of login or registration */
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	/* receive_return < 0 means no response is arrived 
		matbe the server went down */
	if(receive_return < 0)
		return -1;

	/* If operation code is OP_OK, then login or registration are successful */
	if(op.uid == UID_SERVER && op.code == OP_OK){
		client_ui.uid = strtol(op.text, NULL, 10);
		free(op.text);
		printf("%s successful.\n", action_name);
		return 0;
	}

	/* If operation code is OP_NOT_ACCEPTED, then login or registration are unsuccessful
		and the explenation is in the operation text */
	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("%s unsuccessful: %s\n", action_name, op.text);
		free(op.text);
	} else {	/* Unexpected UID or code */
		fprintf(stderr, "Unexpected error during %s.\nUID=%d, code=%c\n", action_name, op.uid, op.code);
		free(op.text);
	}

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