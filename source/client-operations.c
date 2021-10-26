#include "client.h"

int post(){
	char *post_buffer, *body = NULL;
	size_t len_buffer = 0;

	// #1: Ask to user the Subject
	printf("Insert the Subject of the message:\n");
	do {
		scanf("%m[^\n]", &post_buffer);
		fflush(stdin);
	} while (post_buffer == NULL);

	// #2: Ask to user the Body
	printf("Insert the Body of the message, double new line to end:\n");
	while(1){
		scanf("%m[^\n]", &body);
		fflush(stdin);

		if(body == NULL){
			if(len_buffer == 0) continue;
			break;
		}

		// Allocate the space for the buffer plus the new line of the body
		len_buffer = strlen(post_buffer) + 1 + strlen(body);	// +1 because of '\n'
		post_buffer = reallocarray(post_buffer, sizeof(char), len_buffer + 1);

		post_buffer[len_buffer] = '\0';

		strcat(post_buffer, "\n");
		strcat(post_buffer, body);
		free(body);
	}

	#ifdef PRINT_DEBUG_FINE
	printf("BEGIN%s\n%s(%d) is sending:\n%s\n%s\n%s\n%s\n%sEND\n", SEP, client_ui.username, client_ui.uid, SEP,
		subject, SEP, body, SEP);
	#endif

	// #3: Send (UID, OP_MSG, Subject and Body)
	if(send_message_to(sockfd, client_ui.uid, OP_MSG, post_buffer) < 0){
		free(post_buffer);
		return -1;
	}

	// #4: Receive (UID_SERVER, OP_OK, ID of the message)
	operation op;

	if(receive_operation_from(sockfd, &op) < 0)
		return -1;

	if(op.uid == UID_SERVER && op.code == OP_OK){
		printf("Post #%s sent.\n\n", op.text);
		free(op.text);
		return 0;
	}

	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Post refused: %s\n", op.text);
	} else {
		fprintf(stderr, "Unknown error. Exiting.\n");
	}
	free(op.text);
	return -1;

	printf("Message sent.\n");

	return 0;
}

int read_all(){
	int read_mid;
	char *read_username, *subject, *body;
	size_t before_body_len;
	operation op;

	if(send_message_to(sockfd, client_ui.uid, OP_READ_REQUEST, NULL) < 0)
		return -1;

	printf("\nPosts:\n\n");

	while(receive_operation_from(sockfd, &op) == 0){
		switch(op.code){
			case OP_READ_RESPONSE:
				// Body ends with a single '\n'

				// sscanf in order to obtain MID, username and subject
				sscanf(op.text, "%d\n%ms\n%m[^\n]\n", &read_mid, &read_username, &subject);
				// body obtained by pointing at the begin of the body in text, # of bytes obtained with snprintf
				before_body_len = snprintf(NULL, 0, "%d\n%s\n%s\n", read_mid, read_username, subject);
				body = &op.text[before_body_len];

				// Print post
				printf("BEGIN%s\n%s posted #%d: %s\n%s\n%sEND\n\n",
					SEP, read_username, read_mid, subject,
					body, SEP);

				free(op.text);
				break;
			case OP_OK:
			case OP_NOT_ACCEPTED:
				printf("Done.\n\n");
				return 0;
		}
	}

	return 0;
}

int delete_post(){
	int target_mid;
	char target_mid_str[32];

	operation op;

	// #1: Ask user which post to delete
	printf("Which post do you want to delete?\n");
	while(scanf("%d", &target_mid) == 0)
		fflush(stdin);

	// #2: Send delete request to server
	printf("Asking server to delete post #%d\n", target_mid);
	sprintf(target_mid_str, "%d", target_mid);

	if(send_message_to(sockfd, client_ui.uid, OP_DELETE_REQUEST, target_mid_str) < 0)
		return -1;

	// #3: Wait server response
	if(receive_operation_from(sockfd, &op) < 0)
		return -1;

	// #4: Check operation result
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