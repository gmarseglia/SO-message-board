#include "client-routines.h"

int save(int sockfd, user_info client_ui){
	operation op;
	char *subject, *body;

	if(send_message_to(sockfd, client_ui.uid, OP_READ_REQUEST, NULL) < 0)
		return -1;

	while(receive_operation_from(sockfd, &op) == 0){
		print_operation(&op);

		switch(op.code){
			case OP_READ_RESPONSE:
				// printf("UID=%d\ntext=%s\n",
				// 	op.uid, op.text);
				sscanf(op.text, "%m[^\n]", &subject);
				body = &op.text[strlen(subject) + 1];
				printf("%s\nUID=%d\nSubject=%s\n,Body=%s\n%s\n",
					SEP, op.uid, subject, body, SEP);
				free(op.text);
				break;

			case OP_OK:
			case OP_NOT_ACCEPTED:
				return 0;
		}

	}

	return 0;
}
