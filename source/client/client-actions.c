/*
**	Contains the implementation of client actions
*/

#include "client.h"

int post_message(){
	char *post_buffer, *body = NULL;
	size_t len_buffer = 0;
	operation_t op;

	/* #1: Ask user the Subject */
	printf("Insert the Subject of the message:\n");
	do {
		scanf("%m[^\n]", &post_buffer);	/* The subject is stored in post_buffer */
		fflush(stdin);
	} while (post_buffer == NULL);

	/* #2: Ask user the Body
		The body is a multiline string */
	printf("Insert the Body of the message, double new line to end:\n");
	while(1){
		scanf("%m[^\n]", &body);	/* The newest line is stored in body */
		fflush(stdin);

		if(body == NULL){	/* Double new line recognition */
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

	printf("Sending post request.\n");

	/* Pre-send block */
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	#ifdef PRINT_DEBUG_FINE
	printf("BEGIN%s\n%s(%d) is sending:\n%s\n%s\n%s\n%s\n%sEND\n", SEP, client_ui.username, client_ui.uid, SEP,
		subject, SEP, body, SEP);
	#endif

	/* #3: Send operation with message */
	if(send_operation_to(sockfd, client_ui.uid, OP_MSG, post_buffer) < 0){
		free(post_buffer);
		return -1;
	}

	/* After-send unlock */
	pthread_sigmask(SIG_SETMASK, &sigset_sigint_allowed, NULL);

	printf("Waiting for server response...\n");

	/* #4: Receive response */
	if(receive_operation_from_2(sockfd, &op) < 0)
		return -1;

	/* If (UID_SERVER, OP_OK, *) then action was successful and ends correctly */
	if(op.uid == UID_SERVER && op.code == OP_OK){
		printf("Post #%s sent. Done.\n\n", op.text);
			if(op.text != NULL)
		free(op.text);
		return 0;
	}

	/*	If (UID_SERVER, OP_NOT_ACCEPTED, text)
			then post was refused but further operations are possible,
			so action ends correctly */
	if(op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Post refused: %s. Done.\n\n", op.text);
		if(op.text != NULL)
			free(op.text);
		return 0;
	}
	/* If operation code is unexpected, then no further are possible,
		so action ends not correctly */
	else {
		fprintf(stderr, "Unknown error. Exiting.\n");
		print_operation(&op);
		if(op.text != NULL)
			free(op.text);
		return -1;
	}
}

int read_all_messages(){
	int read_mid;
	char *read_username, *subject, *body;
	size_t before_body_len;
	operation_t op;
	unsigned int messagge_counter = 0;

	printf("Sending read request.\n");

	/* Pre-send block */
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	/* #1: Send operation with the read all request */
	if(send_operation_to(sockfd, client_ui.uid, OP_READ_REQUEST, NULL) < 0)
		return -1;

	/* After-send unlock */
	pthread_sigmask(SIG_SETMASK, &sigset_sigint_allowed, NULL);

	/* #2: Read message response * until OK, and print messages 
		Server will send OP_READ_RESPONSE with messages,
		server will end with OP_OK */
	while(receive_operation_from_2(sockfd, &op) == 0){
		switch(op.code){
			case OP_READ_RESPONSE:

				if(messagge_counter == 0){
					printf("\nMessages:\n\n");
				}

				/* sscanf to obtain MID, username and subject */
				sscanf(op.text, "%d\n%ms\n%m[^\n]\n", &read_mid, &read_username, &subject);

				/* The body of the message is obtained by displacing by the # of bytes of MID + Username + subject */
				before_body_len = snprintf(NULL, 0, "%d\n%s\n%s\n", read_mid, read_username, subject);
				body = &op.text[before_body_len];

				/* Print received message */
				printf("BEGIN%s\nMessage #%d from %s:\n\n%s\n\n%s\n%s--END\n\n",
					SEP,
					read_mid, read_username, 
					subject,
					body,
					SEP);

				if(op.text != NULL)
					free(op.text);

				messagge_counter++;

				break;

			/* If last operation's code is OP_OK, then action ends correctly */
			case OP_OK:
				if(messagge_counter > 0){
					printf("%u messages read. Done.\n\n", messagge_counter);
				} else {
					printf("\nNo messages on server. Done.\n\n");
				}
				return 0;

			/* If last operation's code is OP_NOT_ACCEPTED, then the request was denied,
				but further actions are possible so action ends correctly */	
			case OP_NOT_ACCEPTED:
				printf("Read request refused: %s. Done\n\n", op.text);
				if(op.text != NULL)
					free(op.text);
				return 0;

			/* If unexpected code, then action ends not correctly */
			default:
				printf("Unknown error. Exiting.\n\n");
				print_operation(&op);
				return -1;
		}
	}

	/* If last operation's code is OP_READ_RESPONSE, then there is a fatal error
		and action ends not correctly */
	return -1;
}

int delete_message(){
	int target_mid;
	char target_mid_str[32];

	operation_t op;

	/* #1: Ask user which post to delete */
	printf("Which post do you want to delete?\n");
	while(scanf("%d", &target_mid) == 0)
		fflush(stdin);

	sprintf(target_mid_str, "%d", target_mid);

	printf("Sending delete request for post #%d.\n", target_mid);

	/* Pre-send block */
	pthread_sigmask(SIG_SETMASK, &sigset_all_blocked, NULL);

	/* #2: Send operation with delete request */
	if(send_operation_to(sockfd, client_ui.uid, OP_DELETE_REQUEST, target_mid_str) < 0)
		return -1;

	/* After-send unlock */
	pthread_sigmask(SIG_SETMASK, &sigset_sigint_allowed, NULL);

	printf("Waiting for server response...\n");

	/* #3: Receive response */
	if(receive_operation_from_2(sockfd, &op) < 0)
		return -1;

	/* If (UID_SERVER, OP_OK, *) then deletion successful and action ends correctly */
	if(op.uid == UID_SERVER && op.code == OP_OK){
		printf("Post #%d deleted successfully. Done\n\n", target_mid);
		return 0;
	}
	/* If (UID_SERVER, OP_NOT_ACCEPTED, text) then deletion was denied,
		but further actions are possible so action ends correctly */
	else if (op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Post #%d not deleted: %s. Done\n\n", target_mid, op.text);
		if(op.text != NULL)
			free(op.text);
		return 0;
	} 
	/* If unknown code, then action ends not correctly */
	else {
		fprintf(stderr, "Unknown Error. Exiting.\n\n");
		print_operation(&op);
		free(op.text);
		return -1;
	}
}