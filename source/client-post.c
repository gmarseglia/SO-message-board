#include "client-routines.h"

int post(int sockfd, user_info client_ui){
	char *subject, *body = NULL, *tmp_body;
	int body_len = 0;

	// #1: Ask to user the Subject
	printf("Insert the Subject of the message:\n");
	do {
		scanf("%m[^\n]", &subject);
		fflush(stdin);
	} while (subject == NULL);

	// #2: Ask to user the Body
	printf("Insert the Body of the message, double new line to end:\n");
	while(1){
		scanf("%m[^\n]", &tmp_body);
		fflush(stdin);
		if(tmp_body == NULL)
			break;
		body_len = body_len + strlen(tmp_body) + 1;
		body = reallocarray(body, sizeof(char), body_len + 1);
		body[body_len] = '\0';
		strcat(body, tmp_body);
		strcat(body, "\n");
		free(tmp_body);
	}

	#ifdef PRINT_DEBUG_FINE
	printf("-----------------\nSubject:\n%s\nBody:\n%s\n-----------------\n", subject, body);
	#endif

	// #3: Send (UID, OP_MSG_SUBJECT, Subject)
	if(send_message_to(sockfd, client_ui.uid, OP_MSG_SUBJECT, subject) < 0)
		return -1;

	// #4: Send (UID, OP_MSG_BODY, Body)
	if(send_message_to(sockfd, client_ui.uid, OP_MSG_BODY, body) < 0)
		return -1;

	// #5: Receive (UID_SERVER, OP_OK, ID of the message)
	#ifdef WAIT_SERVER_OK
	operation op;
	if(receive_operation_from(sockfd, &op) < 0 || op.uid != UID_SERVER){
		fprintf(stderr, "Error in Post on receive_message_from\n");
		exit(EXIT_FAILURE);
	}
	if(op.code != OP_OK){
		fprintf(stderr, "Post refused: %s\n", op.text);
	}
	#endif

	return 0;
}