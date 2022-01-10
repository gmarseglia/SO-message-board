/*
**	Contains the implementation of client actions
*/

#include "client.h"

int post_message(){
	char *post_buffer, *body = NULL;
	size_t len_buffer = 0;

	/* #1: Ask to user the Subject */
	printf("Insert the Subject of the message:\n");
	do {
		scanf("%m[^\n]", &post_buffer);	/* The subject is stored in post_buffer */
		fflush(stdin);
	} while (post_buffer == NULL);

	/* #2: Ask to user the Body
		The body is a multiline string */
	printf("Insert the Body of the message, double new line to end:\n");
	while(1){
		scanf("%m[^\n]", &body);	/* The newest line is stored in body */
		fflush(stdin);

		if(body == NULL){
			if(len_buffer == 0) continue;
			break;
		}

		// Allocate the space for the buffer plus the new line of the body
		len_buffer = strlen(post_buffer) + 1 + strlen(body);	// +1 because of '\n'
		/* The array pointed by post_buffer is increased in size every new line */
		post_buffer = reallocarray(post_buffer, sizeof(char), len_buffer + 1);

		post_buffer[len_buffer] = '\0';

		/* The newest line is appended to post_buffer */
		strcat(post_buffer, "\n");
		strcat(post_buffer, body);
		free(body);
	}

	/* #3: Block signals
		Once the subject and the body are given, the signals are blocked
		to ensure the correct completion of the post action
		signals will be re-allowed by dispatcher() */
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	#ifdef PRINT_DEBUG_FINE
	printf("BEGIN%s\n%s(%d) is sending:\n%s\n%s\n%s\n%s\n%sEND\n", SEP, client_ui.username, client_ui.uid, SEP,
		subject, SEP, body, SEP);
	#endif

	// #4: Send (UID, OP_MSG, Subject and Body)
	if(send_operation_to(sockfd, client_ui.uid, OP_MSG, post_buffer) < 0){
		free(post_buffer);
		return -1;
	}

	// #5: Awaits server response
	operation_t op;
	if(receive_operation_from_2(sockfd, &op) < 0)
		return -1;

	/* If operation code is OP_OK, then post was successful, do */
	if(op.uid == UID_SERVER && op.code == OP_OK){
		/* #6: Returns 0 */
		printf("Post #%s sent.\n\n", op.text);
		free(op.text);
		return 0;
	}

	/*	If operation code is OP_NOT_ACCEPTED,
		then post was refused but further operations are possible, do */
	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		/* #6: Returns 0 */
		printf("Post refused: %s\n", op.text);
		free(op.text);
		return 0;
	}
	/* If operation code is unexpected, then no further are possible, do */
	else {
		/* #6a: returns -1 */
		fprintf(stderr, "Unknown error. Exiting.\n");
		print_operation(&op);
		free(op.text);
		return -1;
	}
}

int read_all_messages(){
	int read_mid;
	char *read_username, *subject, *body;
	size_t before_body_len;
	operation_t op;

	/* #1: Block signals */
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	/* #2: Send server the read all request */
	if(send_operation_to(sockfd, client_ui.uid, OP_READ_REQUEST, NULL) < 0)
		return -1;

	printf("\nPosts:\n\n");

	/* Server will send an operation for each message, do
		#3: read message response until OP_OK */
	while(receive_operation_from_2(sockfd, &op) == 0){
		switch(op.code){
			case OP_READ_RESPONSE:
				/* sscanf to obtain MID, username and subject */
				sscanf(op.text, "%d\n%ms\n%m[^\n]\n", &read_mid, &read_username, &subject);
				/* The body of the message is obtained by displacing by the # of bytes of MID + Username + subject */
				before_body_len = snprintf(NULL, 0, "%d\n%s\n%s\n", read_mid, read_username, subject);
				body = &op.text[before_body_len];

				// Print post
				printf("BEGIN%s\n%s posted #%d: %s\n%s\n%sEND\n\n",
					SEP, read_username, read_mid, subject,
					body, SEP);

				free(op.text);
				break;
			case OP_OK:
				printf("Done.\n\n");
				return 0;
			case OP_NOT_ACCEPTED:
				printf("Read request refused: %s\n", op.text);
				free(op.text);
				return 0;
			default:
				printf("Unknown error.");
				print_operation(&op);
				return -1;
		}
	}

	/* If last operation's code is OP_READ_RESPONSE, then there is a fatal error */
	return -1;
}

int delete_message(){
	int target_mid;
	char target_mid_str[32];

	operation_t op;

	// #1: Ask user which post to delete
	printf("Which post do you want to delete?\n");
	while(scanf("%d", &target_mid) == 0)
		fflush(stdin);

	// #2: Block signals
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	// #3: Send delete request to server
	printf("Asking server to delete post #%d\n", target_mid);
	sprintf(target_mid_str, "%d", target_mid);

	if(send_operation_to(sockfd, client_ui.uid, OP_DELETE_REQUEST, target_mid_str) < 0)
		return -1;

	// #4: Wait server response
	if(receive_operation_from_2(sockfd, &op) < 0)
		return -1;

	// #5: Check operation_t result
	if(op.uid == UID_SERVER && op.code == OP_OK){
		printf("Post #%d deleted successfully.\n\n", target_mid);
		return 0;
	} else if (op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Post #%d not deleted: %s.\n\n", target_mid, op.text);
		free(op.text);
		return 0;
	} else {
		fprintf(stderr, "Unknown Error in Client on delete_post()\n");
		print_operation(&op);
		free(op.text);
		return -1;
	}
}