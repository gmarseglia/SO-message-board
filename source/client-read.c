#include "client.h"

int read_all(int sockfd, user_info client_ui){
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
