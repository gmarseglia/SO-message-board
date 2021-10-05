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
	body = malloc(sizeof(char));
	body[0] = '\0';
	printf("Insert the Body of the message, double new line to end:\n");
	while(1){
		scanf("%m[^\n]", &tmp_body);
		fflush(stdin);
		if(tmp_body == NULL){
			if(body_len == 0)
				continue;
			else
				break;
		}
		body_len = body_len + strlen(tmp_body) + 1;	// +1 because of '\n'
		body = reallocarray(body, sizeof(char), body_len + 1);
		body[body_len] = '\0';
		strcat(body, tmp_body);
		strcat(body, "\n");
		free(tmp_body);
	}

	printf("BEGIN%s\n%s(%d) is sending:\n%s\n%s\n%s\n%s\n%sEND\n", SEP, client_ui.username, client_ui.uid, SEP,
		subject, SEP, body, SEP);

	// #3: Send (UID, OP_MSG_SUBJECT, Subject)
	if(send_message_to(sockfd, client_ui.uid, OP_MSG_SUBJECT, subject) < 0)
		return -1;
	free(subject);

	// #4: Send (UID, OP_MSG_BODY, Body)
	if(send_message_to(sockfd, client_ui.uid, OP_MSG_BODY, body) < 0)
		return -1;
	free(body);

	// #5: Receive (UID_SERVER, OP_OK, ID of the message)
	#ifdef WAIT_SERVER_OK
	operation op;

	if(receive_operation_from(sockfd, &op) < 0)
		return -1;

	if(op.uid == UID_SERVER && op.code == OP_OK)
		return 0;

	free(op.text);
	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Post refused: %s\n", op.text);
	} else {
		fprintf(stderr, "Unknown error. Exiting.\n");
	}
	return -1;
	#endif

	return 0;
}